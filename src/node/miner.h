// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_MINER_H
#define BITCOIN_NODE_MINER_H

#include <interfaces/types.h>
#include <node/types.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <txmempool.h>
#include <util/feefrac.h>
#include <validation.h>

#include <cstdint>
#include <memory>
#include <optional>

#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/indexed_by.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index_container.hpp>

class ArgsManager;
class CBlockIndex;
class CChainParams;
class CScript;
class Chainstate;
class ChainstateManager;
#ifdef ENABLE_WALLET
namespace wallet { class CWallet; };
#endif

namespace Consensus { struct Params; };

using interfaces::BlockRef;

namespace node {
class KernelNotifications;

static const bool DEFAULT_PRINT_MODIFIED_FEE = false;

static const bool DEFAULT_STAKE = true;

static const bool DEFAULT_STAKE_CACHE = true;

static const bool DEFAULT_SUPER_STAKE = false;

static const bool ENABLE_HARDWARE_STAKE = false;

//How many seconds to look ahead and prepare a block for staking
//Look ahead up to 3 "timeslots" in the future, 48 seconds
//Reduce this to reduce computational waste for stakers, increase this to increase the amount of time available to construct full blocks
static const int32_t MAX_STAKE_LOOKAHEAD = 16 * 3;

//Will not add any more contracts when GetAdjustedTime() >= nTimeLimit-BYTECODE_TIME_BUFFER
//This does not affect non-contract transactions
static const int32_t BYTECODE_TIME_BUFFER = 6;

//Will not attempt to add more transactions when GetAdjustedTime() >= nTimeLimit
//And nTimeLimit = StakeExpirationTime - STAKE_TIME_BUFFER
static const int32_t STAKE_TIME_BUFFER = 2;

//How often to try to stake blocks in milliseconds
static const int32_t STAKER_POLLING_PERIOD = 5000;

//How often to try to stake blocks in milliseconds for minimum difficulty
static const int32_t STAKER_POLLING_PERIOD_MIN_DIFFICULTY = 20000;

//How often to try to check for future walid block
static const int32_t STAKER_WAIT_FOR_WALID_BLOCK = 3000;

//How much time to wait for best block header to be downloaded to the blockchain
static const int32_t STAKER_WAIT_FOR_BEST_BLOCK_HEADER = 250;

//How much max time to wait for best block header to be downloaded to the blockchain
static const int32_t DEFAULT_MAX_STAKER_WAIT_FOR_BEST_BLOCK_HEADER = 4000;

//How much time to spend trying to process transactions when using the generate RPC call
static const int32_t POW_MINER_MAX_TIME = 60;

struct CBlockTemplate
{
    CBlock block;
    // Fees per transaction, not including coinbase transaction (unlike CBlock::vtx).
    std::vector<CAmount> vTxFees;
    // Sigops per transaction, not including coinbase transaction (unlike CBlock::vtx).
    std::vector<int64_t> vTxSigOpsCost;
    /* A vector of package fee rates, ordered by the sequence in which
     * packages are selected for inclusion in the block template.*/
    std::vector<FeePerVSize> m_package_feerates;
    /*
     * Template containing all coinbase transaction fields that are set by our
     * miner code.
     */
    CoinbaseTx m_coinbase_tx;
    // The total fee is the Fees minus the Refund
    int64_t nTotalFees = 0;
};

/** Generate a new block, without valid proof-of-work */
class BlockAssembler
{
private:
    // The constructed block template
    std::unique_ptr<CBlockTemplate> pblocktemplate;

    // Information on the current status of the block
    uint64_t nBlockWeight;
    uint64_t nBlockTx;
    uint64_t nBlockSigOpsCost;
    CAmount nFees;

    // Chain context for the block
    int nHeight;
    int64_t m_lock_time_cutoff;

    const CChainParams& chainparams;
    const CTxMemPool* const m_mempool;
    Chainstate& m_chainstate;
#ifdef ENABLE_WALLET
    wallet::CWallet *pwallet = 0;
#endif

public:
    struct Options : BlockCreateOptions {
        // Configuration parameters for the block size
        mutable size_t nBlockMaxWeight{DEFAULT_BLOCK_MAX_WEIGHT};
        CFeeRate blockMinFeeRate{DEFAULT_BLOCK_MIN_TX_FEE};
        // Whether to call TestBlockValidity() at the end of CreateNewBlock().
        bool test_block_validity{true};
        bool print_modified_fee{DEFAULT_PRINT_MODIFIED_FEE};
    };

    explicit BlockAssembler(Chainstate& chainstate, const CTxMemPool* mempool, const Options& options);

///////////////////////////////////////////// // qtum
    ByteCodeExecResult bceResult;
    uint64_t minGasPrice = 1;
    uint64_t hardBlockGasLimit;
    uint64_t softBlockGasLimit;
    uint64_t txGasLimit;
/////////////////////////////////////////////

    // The original constructed reward tx (either coinbase or coinstake) without gas refund adjustments
    CMutableTransaction originalRewardTx; // qtum

    /** Construct a new block template */
    std::unique_ptr<CBlockTemplate> CreateNewBlock();

    /** The number of transactions in the last assembled block (excluding coinbase transaction) */
    inline static std::optional<int64_t> m_last_block_num_txs{};
    /** The weight of the last assembled block (including reserved weight for block header, txs count and coinbase tx) */
    inline static std::optional<int64_t> m_last_block_weight{};

private:
    const Options m_options;

    // utility functions
    /** Clear the block's state and prepare for assembling a new block */
    void resetBlock();
    /** Add a tx to the block */
    void AddToBlock(const CTxMemPoolEntry& entry);

    // Methods for how to add transactions to a block.
    /** Add transactions based on chunk feerate
      *
      * @pre BlockAssembler::m_mempool must not be nullptr
    */
    void addChunks() EXCLUSIVE_LOCKS_REQUIRED(m_mempool->cs);

    // helper functions for addChunks()
    /** Test if a new chunk would "fit" in the block */
    bool TestChunkBlockLimits(FeePerWeight chunk_feerate, int64_t chunk_sigops_cost) const;
    /** Perform locktime checks on each transaction in a chunk:
      * This check should always succeed, and is here
      * only as an extra check in case of a bug */
    bool TestChunkTransactions(const std::vector<CTxMemPoolEntryRef>& txs) const;
};

#ifdef ENABLE_WALLET
/** Generate a new block, without valid proof-of-work */
void StakeQtums(bool fStake, wallet::CWallet *pwallet);
void RefreshDelegates(wallet::CWallet *pwallet, bool myDelegates, bool stakerDelegates);
#endif

/**
 * Get the minimum time a miner should use in the next block. This always
 * accounts for the BIP94 timewarp rule, so does not necessarily reflect the
 * consensus limit.
 */
int64_t GetMinimumTime(const CBlockIndex* pindexPrev, int64_t difficulty_adjustment_interval);

int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev);

/** Update an old GenerateCoinbaseCommitment from CreateNewBlock after the block txs have changed */
void RegenerateCommitments(CBlock& block, ChainstateManager& chainman);

/** Apply -blockmintxfee and -blockmaxweight options from ArgsManager to BlockAssembler options. */
void ApplyArgsManOptions(const ArgsManager& gArgs, BlockAssembler::Options& options);

/* Compute the block's merkle root, insert or replace the coinbase transaction and the merkle root into the block */
void AddMerkleRootAndCoinbase(CBlock& block, CTransactionRef coinbase, uint32_t version, uint32_t timestamp, uint32_t nonce);


/* Interrupt a blocking call. */
void InterruptWait(KernelNotifications& kernel_notifications, bool& interrupt_wait);
/**
 * Return a new block template when fees rise to a certain threshold or after a
 * new tip; return nullopt if timeout is reached.
 */
std::unique_ptr<CBlockTemplate> WaitAndCreateNewBlock(ChainstateManager& chainman,
                                                      KernelNotifications& kernel_notifications,
                                                      CTxMemPool* mempool,
                                                      const std::unique_ptr<CBlockTemplate>& block_template,
                                                      const BlockWaitOptions& options,
                                                      const BlockAssembler::Options& assemble_options,
                                                      bool& interrupt_wait);

/* Locks cs_main and returns the block hash and block height of the active chain if it exists; otherwise, returns nullopt.*/
std::optional<BlockRef> GetTip(ChainstateManager& chainman);

/* Waits for the connected tip to change until timeout has elapsed. During node initialization, this will wait until the tip is connected (regardless of `timeout`).
 * Returns the current tip, or nullopt if the node is shutting down or interrupt()
 * is called.
 */
std::optional<BlockRef> WaitTipChanged(ChainstateManager& chainman, KernelNotifications& kernel_notifications, const uint256& current_tip, MillisecondsDouble& timeout, bool& interrupt);

/**
 * Wait while the best known header extends the current chain tip AND at least
 * one block is being added to the tip every 3 seconds. If the tip is
 * sufficiently far behind, allow up to 20 seconds for the next tip update.
 *
 * It’s not safe to keep waiting, because a malicious miner could announce a
 * header and delay revealing the block, causing all other miners using this
 * software to stall. At the same time, we need to balance between the default
 * waiting time being brief, but not ending the cooldown prematurely when a
 * random block is slow to download (or process).
 *
 * The cooldown only applies to createNewBlock(), which is typically called
 * once per connected client. Subsequent templates are provided by waitNext().
 *
 * @param last_tip tip at the start of the cooldown window.
 * @param interrupt_mining set to true to interrupt the cooldown.
 *
 * @returns false if interrupted.
 */
bool CooldownIfHeadersAhead(ChainstateManager& chainman, KernelNotifications& kernel_notifications, const BlockRef& last_tip, bool& interrupt_mining);

/** Check if staking is enabled */
bool CanStake();
} // namespace node

#endif // BITCOIN_NODE_MINER_H
