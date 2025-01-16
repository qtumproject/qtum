// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_MINER_H
#define BITCOIN_NODE_MINER_H

#include <node/types.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <txmempool.h>
#include <validation.h>

#include <memory>
#include <optional>
#include <stdint.h>

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

namespace node {
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
    std::vector<CAmount> vTxFees;
    std::vector<int64_t> vTxSigOpsCost;
    std::vector<unsigned char> vchCoinbaseCommitment;
};

// Container for tracking updates to ancestor feerate as we include (parent)
// transactions in a block
struct CTxMemPoolModifiedEntry {
    explicit CTxMemPoolModifiedEntry(CTxMemPool::txiter entry)
    {
        iter = entry;
        nSizeWithAncestors = entry->GetSizeWithAncestors();
        nModFeesWithAncestors = entry->GetModFeesWithAncestors();
        nSigOpCostWithAncestors = entry->GetSigOpCostWithAncestors();
    }

    CAmount GetModifiedFee() const { return iter->GetModifiedFee(); }
    uint64_t GetSizeWithAncestors() const { return nSizeWithAncestors; }
    CAmount GetModFeesWithAncestors() const { return nModFeesWithAncestors; }
    size_t GetTxSize() const { return iter->GetTxSize(); }
    const CTransaction& GetTx() const { return iter->GetTx(); }

    CTxMemPool::txiter iter;
    uint64_t nSizeWithAncestors;
    CAmount nModFeesWithAncestors;
    int64_t nSigOpCostWithAncestors;
};

/** Comparator for CTxMemPool::txiter objects.
 *  It simply compares the internal memory address of the CTxMemPoolEntry object
 *  pointed to. This means it has no meaning, and is only useful for using them
 *  as key in other indexes.
 */
struct CompareCTxMemPoolIter {
    bool operator()(const CTxMemPool::txiter& a, const CTxMemPool::txiter& b) const
    {
        return &(*a) < &(*b);
    }
};

struct modifiedentry_iter {
    typedef CTxMemPool::txiter result_type;
    result_type operator() (const CTxMemPoolModifiedEntry &entry) const
    {
        return entry.iter;
    }
};

// This related to the calculation in CompareTxMemPoolEntryByAncestorFeeOrGasPrice,
// except operating on CTxMemPoolModifiedEntry.
// TODO: refactor to avoid duplication of this logic.
struct CompareModifiedEntry {
    bool operator()(const CTxMemPoolModifiedEntry &a, const CTxMemPoolModifiedEntry &b) const
    {
        int fAHasCreateOrCall = a.iter->GetTx().GetCreateOrCall();
        int fBHasCreateOrCall = b.iter->GetTx().GetCreateOrCall();

        // If either of the two entries that we are comparing has a contract scriptPubKey, the comparison here takes precedence
        if(fAHasCreateOrCall || fBHasCreateOrCall) {

            // Prioritze non-contract txs
            if((fAHasCreateOrCall > CTransaction::OpNone) != (fBHasCreateOrCall > CTransaction::OpNone)) {
                return fAHasCreateOrCall > CTransaction::OpNone ? false : true;
            }

            // Prioritze create contract txs over send to contract txs
            if((fAHasCreateOrCall > CTransaction::OpNone) && (fBHasCreateOrCall > CTransaction::OpNone) &&
                    (fAHasCreateOrCall != fBHasCreateOrCall) && (fAHasCreateOrCall == CTransaction::OpCall || fBHasCreateOrCall == CTransaction::OpCall)){
                return fAHasCreateOrCall == CTransaction::OpCall ? false : true;
            }

            // Prioritize the contract txs that have the least number of ancestors
            // The reason for this is that otherwise it is possible to send one tx with a
            // high gas limit but a low gas price which has a child with a low gas limit but a high gas price
            // Without this condition that transaction chain would get priority in being included into the block.
            // The two next checks are to see if all our ancestors have been added.
            if((int64_t) a.nSizeWithAncestors == a.iter->GetTxSize() && (int64_t) b.nSizeWithAncestors != b.iter->GetTxSize()) {
                return true;
            }

            if((int64_t) b.nSizeWithAncestors == b.iter->GetTxSize() && (int64_t) a.nSizeWithAncestors != a.iter->GetTxSize()) {
                return false;
            }

            // Otherwise, prioritize the contract tx with the highest (minimum among its outputs) gas price
            // The reason for using the gas price of the output that sets the minimum gas price is that
            // otherwise it may be possible to game the prioritization by setting a large gas price in one output
            // that does no execution, while the real execution has a very low gas price
            if(a.iter->GetMinGasPrice() != b.iter->GetMinGasPrice()) {
                return a.iter->GetMinGasPrice() > b.iter->GetMinGasPrice();
            }

            // Otherwise, prioritize the tx with the min size
            if(a.iter->GetTxSize() != b.iter->GetTxSize()) {
                return a.iter->GetTxSize() < b.iter->GetTxSize();
            }

            // If the txs are identical in their minimum gas prices and tx size
            // order based on the tx hash for consistency.
            return CompareIteratorByHash()(a.iter, b.iter);
        }

        // If neither of the txs we are comparing are contract txs, use the standard comparison based on ancestor fees / ancestor size
        return CompareTxMemPoolEntryByAncestorFee()(a, b);
    }
};

// A comparator that sorts transactions based on number of ancestors.
// This is sufficient to sort an ancestor package in an order that is valid
// to appear in a block.
struct CompareTxIterByAncestorCount {
    bool operator()(const CTxMemPool::txiter& a, const CTxMemPool::txiter& b) const
    {
        if (a->GetCountWithAncestors() != b->GetCountWithAncestors()) {
            return a->GetCountWithAncestors() < b->GetCountWithAncestors();
        }
        return CompareIteratorByHash()(a, b);
    }
};


struct CTxMemPoolModifiedEntry_Indices final : boost::multi_index::indexed_by<
    boost::multi_index::ordered_unique<
        modifiedentry_iter,
        CompareCTxMemPoolIter
    >,
    // sorted by modified ancestor fee rate
    boost::multi_index::ordered_non_unique<
        // Reuse same tag from CTxMemPool's similar index
        boost::multi_index::tag<ancestor_score_or_gas_price>,
        boost::multi_index::identity<CTxMemPoolModifiedEntry>,
        CompareModifiedEntry
    >
>
{};

typedef boost::multi_index_container<
    CTxMemPoolModifiedEntry,
    CTxMemPoolModifiedEntry_Indices
> indexed_modified_transaction_set;

typedef indexed_modified_transaction_set::nth_index<0>::type::iterator modtxiter;
typedef indexed_modified_transaction_set::index<ancestor_score_or_gas_price>::type::iterator modtxscoreiter;

struct update_for_parent_inclusion
{
    explicit update_for_parent_inclusion(CTxMemPool::txiter it) : iter(it) {}

    void operator() (CTxMemPoolModifiedEntry &e)
    {
        e.nModFeesWithAncestors -= iter->GetModifiedFee();
        e.nSizeWithAncestors -= iter->GetTxSize();
        e.nSigOpCostWithAncestors -= iter->GetSigOpCost();
    }

    CTxMemPool::txiter iter;
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
    std::unordered_set<Txid, SaltedTxidHasher> inBlock;

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
#ifdef ENABLE_WALLET
    explicit BlockAssembler(Chainstate& chainstate, const CTxMemPool* mempool, wallet::CWallet *pwallet);
#endif

///////////////////////////////////////////// // qtum
    ByteCodeExecResult bceResult;
    uint64_t minGasPrice = 1;
    uint64_t hardBlockGasLimit;
    uint64_t softBlockGasLimit;
    uint64_t txGasLimit;
/////////////////////////////////////////////

    // The original constructed reward tx (either coinbase or coinstake) without gas refund adjustments
    CMutableTransaction originalRewardTx; // qtum

    //When GetAdjustedTime() exceeds this, no more transactions will attempt to be added
    int32_t nTimeLimit;

    /** Construct a new block template with coinbase to scriptPubKeyIn */
    std::unique_ptr<CBlockTemplate> CreateNewBlock(const CScript& scriptPubKeyIn, bool fProofOfStake=false, int64_t* pTotalFees = 0, int32_t nTime=0, int32_t nTimeLimit=0);
    std::unique_ptr<CBlockTemplate> CreateEmptyBlock(const CScript& scriptPubKeyIn, bool fProofOfStake=false, int64_t* pTotalFees = 0, int32_t nTime=0);

    inline static std::optional<int64_t> m_last_block_num_txs{};
    inline static std::optional<int64_t> m_last_block_weight{};

private:
    const Options m_options;

    // utility functions
    /** Clear the block's state and prepare for assembling a new block */
    void resetBlock();
    /** Add a tx to the block */
    void AddToBlock(CTxMemPool::txiter iter);

    bool AttemptToAddContractToBlock(CTxMemPool::txiter iter, uint64_t minGasPrice, CBlock* pblock);

    // Methods for how to add transactions to a block.
    /** Add transactions based on feerate including unconfirmed ancestors
      * Increments nPackagesSelected / nDescendantsUpdated with corresponding
      * statistics from the package selection (for logging statistics). */
    void addPackageTxs(const CTxMemPool& mempool, int& nPackagesSelected, int& nDescendantsUpdated, uint64_t minGasPrice, CBlock* pblock) EXCLUSIVE_LOCKS_REQUIRED(mempool.cs);

    /** Rebuild the coinbase/coinstake transaction to account for new gas refunds **/
    void RebuildRefundTransaction(CBlock* pblock);
    // helper functions for addPackageTxs()
    /** Remove confirmed (inBlock) entries from given set */
    void onlyUnconfirmed(CTxMemPool::setEntries& testSet);
    /** Test if a new package would "fit" in the block */
    bool TestPackage(uint64_t packageSize, int64_t packageSigOpsCost) const;
    /** Perform checks on each transaction in a package:
      * locktime, premature-witness, serialized size (if necessary)
      * These checks should always succeed, and they're here
      * only as an extra check in case of suboptimal node configuration */
    bool TestPackageTransactions(const CTxMemPool::setEntries& package) const;
    /** Sort the package in an order that is valid to appear in a block */
    void SortForBlock(const CTxMemPool::setEntries& package, std::vector<CTxMemPool::txiter>& sortedEntries);
};

#ifdef ENABLE_WALLET
/** Generate a new block, without valid proof-of-work */
void StakeQtums(bool fStake, wallet::CWallet *pwallet);
void RefreshDelegates(wallet::CWallet *pwallet, bool myDelegates, bool stakerDelegates);
#endif

int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev);

/** Update an old GenerateCoinbaseCommitment from CreateNewBlock after the block txs have changed */
void RegenerateCommitments(CBlock& block, ChainstateManager& chainman);

/** Apply -blockmintxfee and -blockmaxweight options from ArgsManager to BlockAssembler options. */
void ApplyArgsManOptions(const ArgsManager& gArgs, BlockAssembler::Options& options);

/** Check if staking is enabled */
bool CanStake();
} // namespace node

#endif // BITCOIN_NODE_MINER_H
