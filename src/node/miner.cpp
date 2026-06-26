// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/miner.h>

#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <common/args.h>
#include <common/threadpriority.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <logging.h>
#include <node/context.h>
#include <node/kernel_notifications.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <pow.h>
#include <pos.h>
#include <primitives/transaction.h>
#include <util/moneystr.h>
#include <util/signalinterrupt.h>
#include <util/time.h>
#include <validation.h>
#include <util/threadnames.h>
#include <key_io.h>
#include <qtum/qtumledger.h>
#include <qtum/qtumdelegation.h>
#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#include <wallet/receive.h>
#include <wallet/stake.h>
#endif

#include <algorithm>
#include <utility>
#include <numeric>

namespace node {
unsigned int nMaxStakeLookahead = MAX_STAKE_LOOKAHEAD;
unsigned int nBytecodeTimeBuffer = BYTECODE_TIME_BUFFER;
unsigned int nStakeTimeBuffer = STAKE_TIME_BUFFER;
unsigned int nMinerSleep = STAKER_POLLING_PERIOD;
unsigned int nMinerWaitWalidBlock = STAKER_WAIT_FOR_WALID_BLOCK;
unsigned int nMinerWaitBestBlockHeader = STAKER_WAIT_FOR_BEST_BLOCK_HEADER;

void updateMinerParams(int nHeight, const Consensus::Params& consensusParams, bool minDifficulty)
{
    static unsigned int timeDownscale = 1;
    static unsigned int timeDefault = 1;
    unsigned int timeDownscaleTmp = consensusParams.TimestampDownscaleFactor(nHeight);
    if(timeDownscale != timeDownscaleTmp)
    {
        timeDownscale = timeDownscaleTmp;
        unsigned int targetSpacing =  consensusParams.TargetSpacing(nHeight);
        nMaxStakeLookahead = std::max(MAX_STAKE_LOOKAHEAD / timeDownscale, timeDefault);
        nMaxStakeLookahead = std::min(nMaxStakeLookahead, targetSpacing);
        nBytecodeTimeBuffer = std::max(BYTECODE_TIME_BUFFER / timeDownscale, timeDefault);
        nStakeTimeBuffer = std::max(STAKE_TIME_BUFFER / timeDownscale, timeDefault);
        nMinerSleep = std::max(STAKER_POLLING_PERIOD / timeDownscale, timeDefault);
        nMinerWaitWalidBlock = std::max(STAKER_WAIT_FOR_WALID_BLOCK / timeDownscale, timeDefault);
    }

    // Sleep for 20 seconds when mining with minimum difficulty to avoid creating blocks every 4 seconds
    if(minDifficulty && nMinerSleep != STAKER_POLLING_PERIOD_MIN_DIFFICULTY)
    {
        nMinerSleep = STAKER_POLLING_PERIOD_MIN_DIFFICULTY;
    }
}

int64_t GetMinimumTime(const CBlockIndex* pindexPrev, const int64_t difficulty_adjustment_interval)
{
    int64_t min_time{pindexPrev->GetMedianTimePast() + 1};
    // Height of block to be mined.
    const int height{pindexPrev->nHeight + 1};
    // Account for BIP94 timewarp rule on all networks. This makes future
    // activation safer.
    if (height % difficulty_adjustment_interval == 0) {
        min_time = std::max<int64_t>(min_time, pindexPrev->GetBlockTime() - MAX_TIMEWARP);
    }
    return min_time;
}

int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev)
{
    int64_t nOldTime = pblock->nTime;
    const int height{pindexPrev->nHeight + 1};
    int64_t nNewTime{std::max<int64_t>(GetMinimumTime(pindexPrev, consensusParams.DifficultyAdjustmentInterval(height)),
                                       TicksSinceEpoch<std::chrono::seconds>(NodeClock::now()))};

    if (nOldTime < nNewTime) {
        pblock->nTime = nNewTime;
    }

    // Updating time can change work required on testnet:
    if (consensusParams.fPowAllowMinDifficultyBlocks) {
        pblock->nBits = GetNextWorkRequired(pindexPrev, pblock, consensusParams, pblock->IsProofOfStake());
    }

    return nNewTime - nOldTime;
}

void RegenerateCommitments(CBlock& block, ChainstateManager& chainman)
{
    CMutableTransaction tx{*block.vtx.at(0)};
    tx.vout.erase(tx.vout.begin() + GetWitnessCommitmentIndex(block));
    block.vtx.at(0) = MakeTransactionRef(tx);

    const CBlockIndex* prev_block = WITH_LOCK(::cs_main, return chainman.m_blockman.LookupBlockIndex(block.hashPrevBlock));
    chainman.GenerateCoinbaseCommitment(block, prev_block);

    block.hashMerkleRoot = BlockMerkleRoot(block);
}

static BlockAssembler::Options ClampOptions(BlockAssembler::Options options)
{
    // Apply DEFAULT_BLOCK_RESERVED_WEIGHT when the caller left it unset.
    options.block_reserved_weight = std::clamp<size_t>(options.block_reserved_weight.value_or(DEFAULT_BLOCK_RESERVED_WEIGHT), MINIMUM_BLOCK_RESERVED_WEIGHT, dgpMaxBlockWeight - 4000);
    options.coinbase_output_max_additional_sigops = std::clamp<size_t>(options.coinbase_output_max_additional_sigops, 0, dgpMaxTxSigOps);
    // Limit weight to between block_reserved_weight and dgpMaxBlockWeight-4K for sanity:
    // block_reserved_weight can safely exceed -blockmaxweight, but the rest of the block template will be empty.
    options.nBlockMaxWeight = std::clamp<size_t>(options.nBlockMaxWeight, *options.block_reserved_weight, dgpMaxBlockWeight - 4000);
    return options;
}

BlockAssembler::BlockAssembler(Chainstate& chainstate, const CTxMemPool* mempool, const Options& options)
    : chainparams{chainstate.m_chainman.GetParams()},
      m_mempool{options.use_mempool ? mempool : nullptr},
      m_chainstate{chainstate},
      m_options{ClampOptions(options)}
{
}

void ApplyArgsManOptions(const ArgsManager& args, BlockAssembler::Options& options)
{
    // Block resource limits
    options.nBlockMaxWeight = args.GetIntArg("-blockmaxweight", options.nBlockMaxWeight);
    if (const auto blockmintxfee{args.GetArg("-blockmintxfee")}) {
        if (const auto parsed{ParseMoney(*blockmintxfee)}) options.blockMinFeeRate = CFeeRate{*parsed};
    }
    options.print_modified_fee = args.GetBoolArg("-printpriority", options.print_modified_fee);
    if (!options.block_reserved_weight) {
        options.block_reserved_weight = args.GetIntArg("-blockreservedweight");
    }
    options.disable_contract_staking = args.GetBoolArg("-disablecontractstaking", false);
}

static BlockAssembler::Options ConfiguredOptions()
{
    BlockAssembler::Options options;
    ApplyArgsManOptions(gArgs, options);
    return options;
}

#ifdef ENABLE_WALLET
BlockAssembler::BlockAssembler(Chainstate& chainstate, const CTxMemPool* mempool, wallet::CWallet* _pwallet, const Options& options)
    : BlockAssembler(chainstate, mempool, options)
{
    pwallet = _pwallet;
}
#endif

void BlockAssembler::resetBlock()
{
    // Reserve space for fixed-size block header, txs count, and coinbase tx.
    nBlockWeight = *Assert(m_options.block_reserved_weight);
    nBlockSigOpsCost = m_options.coinbase_output_max_additional_sigops;

    // These counters do not include coinbase tx
    nBlockTx = 0;
    nFees = 0;
}

void BlockAssembler::RebuildRefundTransaction(CBlock* pblock){
    int refundtx=0; //0 for coinbase in PoW
    if(pblock->IsProofOfStake()){
        refundtx=1; //1 for coinstake in PoS
    }
    CMutableTransaction contrTx(originalRewardTx);
    contrTx.vout[refundtx].nValue = nFees + GetBlockSubsidy(nHeight, chainparams.GetConsensus());
    contrTx.vout[refundtx].nValue -= bceResult.refundSender;
    //note, this will need changed for MPoS
    int i=contrTx.vout.size();
    contrTx.vout.resize(contrTx.vout.size()+bceResult.refundOutputs.size());
    for(CTxOut& vout : bceResult.refundOutputs){
        contrTx.vout[i]=vout;
        i++;
    }
    pblock->vtx[refundtx] = MakeTransactionRef(std::move(contrTx));
}

std::unique_ptr<CBlockTemplate> BlockAssembler::CreateNewBlock()
{
    const auto time_start{SteadyClock::now()};

    resetBlock();

    pblocktemplate.reset(new CBlockTemplate());
    CBlock* const pblock = &pblocktemplate->block; // pointer for convenience

    // Add dummy coinbase tx as first transaction. It is skipped by the
    // getblocktemplate RPC and mining interface consumers must not use it.
    pblock->vtx.emplace_back();
    // Add dummy coinstake tx as second transaction
    if(m_options.is_coinstake)
        pblock->vtx.emplace_back();

#ifdef ENABLE_WALLET
    if(pwallet && pwallet->IsStakeClosing())
        return nullptr;
#endif
    LOCK(::cs_main);
    CBlockIndex* pindexPrev = m_chainstate.m_chain.Tip();
    assert(pindexPrev != nullptr);
    nHeight = pindexPrev->nHeight + 1;

    pblock->nVersion = m_chainstate.m_chainman.m_versionbitscache.ComputeBlockVersion(pindexPrev, chainparams.GetConsensus());
    // -regtest only: allow overriding block.nVersion with
    // -blockversion=N to test forking scenarios
    if (chainparams.MineBlocksOnDemand()) {
        pblock->nVersion = gArgs.GetIntArg("-blockversion", pblock->nVersion);
    }

    int64_t txProofTime = m_options.proof_time;
    if(txProofTime == 0) {
        txProofTime = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now());
    }
    if(m_options.is_coinstake)
        txProofTime &= ~chainparams.GetConsensus().StakeTimestampMask(nHeight);
    pblock->nTime = txProofTime;
    if (!m_options.is_coinstake)
        UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev);
    pblock->nBits = GetNextWorkRequired(pindexPrev, pblock, chainparams.GetConsensus(),m_options.is_coinstake);

    m_lock_time_cutoff = pindexPrev->GetMedianTimePast();

    const auto time_1{SteadyClock::now()};

    m_last_block_num_txs = nBlockTx;
    m_last_block_weight = nBlockWeight;

    // Create coinbase transaction.
    CMutableTransaction coinbaseTx;

    // Construct coinbase transaction struct in parallel
    CoinbaseTx& coinbase_tx{pblocktemplate->m_coinbase_tx};
    coinbase_tx.version = coinbaseTx.version;

    coinbaseTx.vin.resize(1);
    coinbaseTx.vin[0].prevout.SetNull();
    coinbaseTx.vin[0].nSequence = CTxIn::MAX_SEQUENCE_NONFINAL; // Make sure timelock is enforced.
    coinbase_tx.sequence = coinbaseTx.vin[0].nSequence;

    // Add an output that spends the full coinbase reward.
    coinbaseTx.vout.resize(1);
    if (m_options.is_coinstake)
    {
        // Make the coinbase tx empty in case of proof of stake
        coinbaseTx.vout[0].SetEmpty();
        coinbase_tx.block_reward_remaining = 0;
    }
    else
    {
        coinbaseTx.vout[0].scriptPubKey = m_options.coinbase_output_script;
        // Block subsidy + fees
        const CAmount block_reward{nFees + GetBlockSubsidy(nHeight, chainparams.GetConsensus())};
        coinbaseTx.vout[0].nValue = block_reward;
        coinbase_tx.block_reward_remaining = block_reward;
    }

    // Start the coinbase scriptSig with the block height as required by BIP34.
    // Mining clients are expected to append extra data to this prefix, so
    // increasing its length would reduce the space they can use and may break
    // existing clients.
    coinbaseTx.vin[0].scriptSig = CScript() << nHeight;
    if (m_options.include_dummy_extranonce) {
        // For blocks at heights <= 16, the BIP34-encoded height alone is only
        // one byte. Consensus requires coinbase scriptSigs to be at least two
        // bytes long (bad-cb-length), so tests and regtest include a dummy
        // extraNonce (OP_0)
        coinbaseTx.vin[0].scriptSig << OP_0;
    }
    coinbase_tx.script_sig_prefix = coinbaseTx.vin[0].scriptSig;
    Assert(nHeight > 0);
    coinbaseTx.nLockTime = static_cast<uint32_t>(nHeight - 1);
    coinbase_tx.lock_time = coinbaseTx.nLockTime;
    originalRewardTx = coinbaseTx;
    pblock->vtx[0] = MakeTransactionRef(std::move(coinbaseTx));

    // Create coinstake transaction.
    if(m_options.is_coinstake)
    {
        CMutableTransaction coinstakeTx;
        coinstakeTx.vout.resize(2);
        coinstakeTx.vout[0].SetEmpty();
        coinstakeTx.vout[1].scriptPubKey = m_options.coinbase_output_script;
        originalRewardTx = coinstakeTx;
        pblock->vtx[1] = MakeTransactionRef(std::move(coinstakeTx));

        //this just makes CBlock::IsProofOfStake to return true
        //real prevoutstake info is filled in later in SignBlock
        pblock->prevoutStake.n=0;
    }

    //////////////////////////////////////////////////////// qtum
    QtumDGP qtumDGP(globalState.get(), m_chainstate, fGettingValuesDGP);
    globalSealEngine->setQtumSchedule(qtumDGP.getGasSchedule(nHeight));
    uint32_t blockSizeDGP = qtumDGP.getBlockSize(nHeight);
    minGasPrice = qtumDGP.getMinGasPrice(nHeight);
    if(gArgs.IsArgSet("-staker-min-tx-gas-price")) {
        std::optional<CAmount> stakerMinGasPrice = ParseMoney(gArgs.GetArg("-staker-min-tx-gas-price", ""));
        minGasPrice = std::max(minGasPrice, (uint64_t)(stakerMinGasPrice.value_or(0)));
    }
    hardBlockGasLimit = qtumDGP.getBlockGasLimit(nHeight);
    softBlockGasLimit = gArgs.GetIntArg("-staker-soft-block-gas-limit", hardBlockGasLimit);
    softBlockGasLimit = std::min(softBlockGasLimit, hardBlockGasLimit);
    txGasLimit = gArgs.GetIntArg("-staker-max-tx-gas-limit", softBlockGasLimit);

    m_options.nBlockMaxWeight = blockSizeDGP ? blockSizeDGP * WITNESS_SCALE_FACTOR : m_options.nBlockMaxWeight;
    
    dev::h256 oldHashStateRoot(globalState->rootHash());
    dev::h256 oldHashUTXORoot(globalState->rootHashUTXO());
    ////////////////////////////////////////////////// deploy offline staking contract
    if(nHeight == chainparams.GetConsensus().nOfflineStakeHeight){
        globalState->deployDelegationsContract();
    }
    /////////////////////////////////////////////////
    if (m_mempool) {
        LOCK(m_mempool->cs);
        m_mempool->StartBlockBuilding();
        addChunks();
        m_mempool->StopBlockBuilding();
    }
    pblock->hashStateRoot = uint256(h256Touint(dev::h256(globalState->rootHash())));
    pblock->hashUTXORoot = uint256(h256Touint(dev::h256(globalState->rootHashUTXO())));
    globalState->setRoot(oldHashStateRoot);
    globalState->setRootUTXO(oldHashUTXORoot);

    //this should already be populated by AddBlock in case of contracts, but if no contracts
    //then it won't get populated
    RebuildRefundTransaction(pblock);
    ////////////////////////////////////////////////////////

    m_chainstate.m_chainman.GenerateCoinbaseCommitment(*pblock, pindexPrev, m_options.is_coinstake);

    const CTransactionRef& final_coinbase{pblock->vtx[0]};
    if (final_coinbase->HasWitness()) {
        const auto& witness_stack{final_coinbase->vin[0].scriptWitness.stack};
        // Consensus requires the coinbase witness stack to have exactly one
        // element of 32 bytes.
        Assert(witness_stack.size() == 1 && witness_stack[0].size() == 32);
        coinbase_tx.witness = uint256(witness_stack[0]);
    }
    if (const int witness_index = GetWitnessCommitmentIndex(*pblock); witness_index != NO_WITNESS_COMMITMENT) {
        Assert(witness_index >= 0 && static_cast<size_t>(witness_index) < final_coinbase->vout.size());
        coinbase_tx.required_outputs.push_back(final_coinbase->vout[witness_index]);
    }

    LogInfo("CreateNewBlock(): block weight: %u txs: %u fees: %ld sigops %d\n", GetBlockWeight(*pblock), nBlockTx, nFees, nBlockSigOpsCost);

    // The total fee is the Fees minus the Refund
    pblocktemplate->nTotalFees = nFees - bceResult.refundSender;

    // Fill in header
    pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
    pblock->nNonce         = 0;

    if (!m_options.is_coinstake && m_options.test_block_validity) {
        // if nHeight <= 16, and include_dummy_extranonce=false this will fail due to bad-cb-length.
        if (BlockValidationState state{TestBlockValidity(m_chainstate, *pblock, /*check_pow=*/false, /*check_merkle_root=*/false)}; !state.IsValid()) {
            throw std::runtime_error(strprintf("TestBlockValidity failed: %s", state.ToString()));
        }
    }
    const auto time_2{SteadyClock::now()};

    LogDebug(BCLog::BENCH, "CreateNewBlock() chunks: %.2fms, validity: %.2fms (total %.2fms)\n",
             Ticks<MillisecondsDouble>(time_1 - time_start),
             Ticks<MillisecondsDouble>(time_2 - time_1),
             Ticks<MillisecondsDouble>(time_2 - time_start));

    return std::move(pblocktemplate);
}

std::unique_ptr<CBlockTemplate> BlockAssembler::CreateEmptyBlock()
{
    resetBlock();

    pblocktemplate.reset(new CBlockTemplate());
    CBlock* const pblock = &pblocktemplate->block; // pointer for convenience

    // Add dummy coinbase tx as first transaction
    pblock->vtx.emplace_back();
    // Add dummy coinstake tx as second transaction
    if(m_options.is_coinstake)
        pblock->vtx.emplace_back();

#ifdef ENABLE_WALLET
    if(pwallet && pwallet->IsStakeClosing())
        return nullptr;
#endif
    LOCK(::cs_main);
    CBlockIndex* pindexPrev = m_chainstate.m_chain.Tip();
    assert(pindexPrev != nullptr);
    nHeight = pindexPrev->nHeight + 1;

    pblock->nVersion = m_chainstate.m_chainman.m_versionbitscache.ComputeBlockVersion(pindexPrev, chainparams.GetConsensus());
    // -regtest only: allow overriding block.nVersion with
    // -blockversion=N to test forking scenarios
    if (chainparams.MineBlocksOnDemand()) {
        pblock->nVersion = gArgs.GetIntArg("-blockversion", pblock->nVersion);
    }

    int64_t txProofTime = m_options.proof_time;
    if(txProofTime == 0) {
        txProofTime = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now());
    }
    if(m_options.is_coinstake)
        txProofTime &= ~chainparams.GetConsensus().StakeTimestampMask(nHeight);
    pblock->nTime = txProofTime;

    m_lock_time_cutoff = pindexPrev->GetMedianTimePast();

    m_last_block_num_txs = nBlockTx;
    m_last_block_weight = nBlockWeight;

    // Create coinbase transaction.
    CMutableTransaction coinbaseTx;

    // Construct coinbase transaction struct in parallel
    CoinbaseTx& coinbase_tx{pblocktemplate->m_coinbase_tx};
    coinbase_tx.version = coinbaseTx.version;

    coinbaseTx.vin.resize(1);
    coinbaseTx.vin[0].prevout.SetNull();
    coinbaseTx.vin[0].nSequence = CTxIn::MAX_SEQUENCE_NONFINAL; // Make sure timelock is enforced.
    coinbase_tx.sequence = coinbaseTx.vin[0].nSequence;

    // Add an output that spends the full coinbase reward.
    coinbaseTx.vout.resize(1);
    if (m_options.is_coinstake)
    {
        // Make the coinbase tx empty in case of proof of stake
        coinbaseTx.vout[0].SetEmpty();
        coinbase_tx.block_reward_remaining = 0;
    }
    else
    {
        coinbaseTx.vout[0].scriptPubKey = m_options.coinbase_output_script;
        // Block subsidy + fees
        const CAmount block_reward{nFees + GetBlockSubsidy(nHeight, chainparams.GetConsensus())};
        coinbaseTx.vout[0].nValue = block_reward;
        coinbase_tx.block_reward_remaining = block_reward;
    }

    // Start the coinbase scriptSig with the block height as required by BIP34.
    // Mining clients are expected to append extra data to this prefix, so
    // increasing its length would reduce the space they can use and may break
    // existing clients.
    coinbaseTx.vin[0].scriptSig = CScript() << nHeight;
    if (m_options.include_dummy_extranonce) {
        // For blocks at heights <= 16, the BIP34-encoded height alone is only
        // one byte. Consensus requires coinbase scriptSigs to be at least two
        // bytes long (bad-cb-length), so tests and regtest include a dummy
        // extraNonce (OP_0)
        coinbaseTx.vin[0].scriptSig << OP_0;
    }
    coinbase_tx.script_sig_prefix = coinbaseTx.vin[0].scriptSig;
    Assert(nHeight > 0);
    coinbaseTx.nLockTime = static_cast<uint32_t>(nHeight - 1);
    coinbase_tx.lock_time = coinbaseTx.nLockTime;
    originalRewardTx = coinbaseTx;
    pblock->vtx[0] = MakeTransactionRef(std::move(coinbaseTx));

    // Create coinstake transaction.
    if(m_options.is_coinstake)
    {
        CMutableTransaction coinstakeTx;
        coinstakeTx.vout.resize(2);
        coinstakeTx.vout[0].SetEmpty();
        coinstakeTx.vout[1].scriptPubKey = m_options.coinbase_output_script;
        originalRewardTx = coinstakeTx;
        pblock->vtx[1] = MakeTransactionRef(std::move(coinstakeTx));

        //this just makes CBlock::IsProofOfStake to return true
        //real prevoutstake info is filled in later in SignBlock
        pblock->prevoutStake.n=0;
    }

    //////////////////////////////////////////////////////// qtum
    //state shouldn't change here for an empty block, but if it's not valid it'll fail in CheckBlock later
    pblock->hashStateRoot = uint256(h256Touint(dev::h256(globalState->rootHash())));
    pblock->hashUTXORoot = uint256(h256Touint(dev::h256(globalState->rootHashUTXO())));

    RebuildRefundTransaction(pblock);
    ////////////////////////////////////////////////////////

    m_chainstate.m_chainman.GenerateCoinbaseCommitment(*pblock, pindexPrev, m_options.is_coinstake);

    const CTransactionRef& final_coinbase{pblock->vtx[0]};
    if (final_coinbase->HasWitness()) {
        const auto& witness_stack{final_coinbase->vin[0].scriptWitness.stack};
        // Consensus requires the coinbase witness stack to have exactly one
        // element of 32 bytes.
        Assert(witness_stack.size() == 1 && witness_stack[0].size() == 32);
        coinbase_tx.witness = uint256(witness_stack[0]);
    }
    if (const int witness_index = GetWitnessCommitmentIndex(*pblock); witness_index != NO_WITNESS_COMMITMENT) {
        Assert(witness_index >= 0 && static_cast<size_t>(witness_index) < final_coinbase->vout.size());
        coinbase_tx.required_outputs.push_back(final_coinbase->vout[witness_index]);
    }

    // The total fee is the Fees minus the Refund
    pblocktemplate->nTotalFees = nFees - bceResult.refundSender;

    // Fill in header
    pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
    if (!m_options.is_coinstake)
        UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev);
    pblock->nBits          = GetNextWorkRequired(pindexPrev, pblock, chainparams.GetConsensus(),m_options.is_coinstake);
    pblock->nNonce         = 0;

    if (!m_options.is_coinstake && m_options.test_block_validity) {
        // if nHeight <= 16, and include_dummy_extranonce=false this will fail due to bad-cb-length.
        if (BlockValidationState state{TestBlockValidity(m_chainstate, *pblock, /*check_pow=*/false, /*check_merkle_root=*/false)}; !state.IsValid()) {
            throw std::runtime_error(strprintf("TestBlockValidity failed: %s", state.ToString()));
        }
    }

    return std::move(pblocktemplate);
}

bool BlockAssembler::TestChunkBlockLimits(FeePerWeight chunk_feerate, int64_t chunk_sigops_cost) const
{
    if (nBlockWeight + chunk_feerate.size >= m_options.nBlockMaxWeight) {
        return false;
    }
    if (nBlockSigOpsCost + chunk_sigops_cost >= (uint64_t)dgpMaxBlockSigOps) {
        return false;
    }
    return true;
}

// Perform transaction-level checks before adding to block:
// - transaction finality (locktime)
bool BlockAssembler::TestChunkTransactions(const std::vector<CTxMemPoolEntryRef>& txs) const
{
    for (const auto tx : txs) {
        if (!IsFinalTx(tx.get().GetTx(), nHeight, m_lock_time_cutoff)) {
            return false;
        }
    }
    return true;
}

bool BlockAssembler::AttemptToAddContractToBlock(const CTxMemPoolEntry& entry) {
    if (m_options.time_limit != 0 && TicksSinceEpoch<std::chrono::seconds>(NodeClock::now()) >= m_options.time_limit - nBytecodeTimeBuffer) {
        return false;
    }

    if (m_options.disable_contract_staking)
    {
        // Contract staking is disabled for the staker
        return false;
    }

    CBlock* const pblock = &pblocktemplate->block;
    dev::h256 oldHashStateRoot(globalState->rootHash());
    dev::h256 oldHashUTXORoot(globalState->rootHashUTXO());
    // operate on local vars first, then later apply to `this`
    uint64_t nBlockWeight = this->nBlockWeight;
    uint64_t nBlockSigOpsCost = this->nBlockSigOpsCost;

    script_verify_flags contractflags = GetContractScriptFlags(nHeight, chainparams.GetConsensus());
    QtumTxConverter convert(entry.GetTx(), m_chainstate, m_mempool, NULL, &pblock->vtx, contractflags);

    ExtractQtumTX resultConverter;
    if(!convert.extractionQtumTransactions(resultConverter)){
        //this check already happens when accepting txs into mempool
        //therefore, this can only be triggered by using raw transactions on the staker itself
        LogInfo("AttemptToAddContractToBlock(): Fail to extract contacts from tx %s\n", entry.GetTx().GetHash().ToString());
        return false;
    }
    std::vector<QtumTransaction> qtumTransactions = resultConverter.first;
    dev::u256 txGas = 0;
    for(QtumTransaction qtumTransaction : qtumTransactions){
        txGas += qtumTransaction.gas();
        if(txGas > txGasLimit) {
            // Limit the tx gas limit by the soft limit if such a limit has been specified.
            LogInfo("AttemptToAddContractToBlock(): The gas needed is bigger than -staker-max-tx-gas-limit for the contract tx %s\n", entry.GetTx().GetHash().ToString());
            return false;
        }

        if(bceResult.usedGas + qtumTransaction.gas() > softBlockGasLimit){
            // If this transaction's gasLimit could cause block gas limit to be exceeded, then don't add it
            // Log if the contract is the only contract tx
            if(bceResult.usedGas == 0)
                LogInfo("AttemptToAddContractToBlock(): The gas needed is bigger than -staker-soft-block-gas-limit for the contract tx %s\n", entry.GetTx().GetHash().ToString());
            return false;
        }
        if(qtumTransaction.gasPrice() < minGasPrice){
            //if this transaction's gasPrice is less than the current DGP minGasPrice don't add it
            LogInfo("AttemptToAddContractToBlock(): The gas price is less than -staker-min-tx-gas-price for the contract tx %s\n", entry.GetTx().GetHash().ToString());
            return false;
        }
    }
    // We need to pass the DGP's block gas limit (not the soft limit) since it is consensus critical.
    ByteCodeExec exec(*pblock, qtumTransactions, hardBlockGasLimit, m_chainstate.m_chain.Tip(), m_chainstate.m_chain);
    if(!exec.performByteCode()){
        //error, don't add contract
        globalState->setRoot(oldHashStateRoot);
        globalState->setRootUTXO(oldHashUTXORoot);
        LogInfo("AttemptToAddContractToBlock(): Perform byte code fails for the contract tx %s\n", entry.GetTx().GetHash().ToString());
        return false;
    }

    ByteCodeExecResult testExecResult;
    if(!exec.processingResults(testExecResult)){
        globalState->setRoot(oldHashStateRoot);
        globalState->setRootUTXO(oldHashUTXORoot);
        LogInfo("AttemptToAddContractToBlock(): Processing results fails for the contract tx %s\n", entry.GetTx().GetHash().ToString());
        return false;
    }

    if(bceResult.usedGas + testExecResult.usedGas > softBlockGasLimit){
        // If this transaction could cause block gas limit to be exceeded, then don't add it
        globalState->setRoot(oldHashStateRoot);
        globalState->setRootUTXO(oldHashUTXORoot);
        // Log if the contract is the only contract tx
        if(bceResult.usedGas == 0)
            LogInfo("AttemptToAddContractToBlock(): The gas used is bigger than -staker-soft-block-gas-limit for the contract tx %s\n", entry.GetTx().GetHash().ToString());
        return false;
    }

    //apply contractTx costs to local state
    nBlockWeight += entry.GetTxWeight();
    nBlockSigOpsCost += entry.GetSigOpCost();
    //apply value-transfer txs to local state
    for (CTransaction &t : testExecResult.valueTransfers) {
        nBlockWeight += GetTransactionWeight(t);
        nBlockSigOpsCost += GetLegacySigOpCount(t);
    }

    int proofTx = pblock->IsProofOfStake() ? 1 : 0;

    //calculate sigops from new refund/proof tx

    //first, subtract old proof tx
    nBlockSigOpsCost -= GetLegacySigOpCount(*pblock->vtx[proofTx]);

    // manually rebuild refundtx
    CMutableTransaction contrTx(*pblock->vtx[proofTx]);
    //note, this will need changed for MPoS
    int i=contrTx.vout.size();
    contrTx.vout.resize(contrTx.vout.size()+testExecResult.refundOutputs.size());
    for(CTxOut& vout : testExecResult.refundOutputs){
        contrTx.vout[i]=vout;
        i++;
    }
    nBlockSigOpsCost += GetLegacySigOpCount(contrTx);
    //all contract costs now applied to local state

    //Check if block will be too big or too expensive with this contract execution
    if (nBlockSigOpsCost * WITNESS_SCALE_FACTOR > (uint64_t)dgpMaxBlockSigOps ||
            nBlockWeight > dgpMaxBlockWeight) {
        //contract will not be added to block, so revert state to before we tried
        globalState->setRoot(oldHashStateRoot);
        globalState->setRootUTXO(oldHashUTXORoot);
        return false;
    }

    //block is not too big, so apply the contract execution and it's results to the actual block

    //apply local bytecode to global bytecode state
    bceResult.usedGas += testExecResult.usedGas;
    bceResult.refundSender += testExecResult.refundSender;
    bceResult.refundOutputs.insert(bceResult.refundOutputs.end(), testExecResult.refundOutputs.begin(), testExecResult.refundOutputs.end());
    bceResult.valueTransfers = std::move(testExecResult.valueTransfers);

    pblock->vtx.emplace_back(entry.GetSharedTx());
    pblocktemplate->vTxFees.push_back(entry.GetFee());
    pblocktemplate->vTxSigOpsCost.push_back(entry.GetSigOpCost());
    this->nBlockWeight += entry.GetTxWeight();
    ++nBlockTx;
    this->nBlockSigOpsCost += entry.GetSigOpCost();
    nFees += entry.GetFee();

    for (CTransaction &t : bceResult.valueTransfers) {
        pblock->vtx.emplace_back(MakeTransactionRef(std::move(t)));
        this->nBlockWeight += GetTransactionWeight(t);
        this->nBlockSigOpsCost += GetLegacySigOpCount(t);
        ++nBlockTx;
    }
    //calculate sigops from new refund/proof tx
    this->nBlockSigOpsCost -= GetLegacySigOpCount(*pblock->vtx[proofTx]);
    RebuildRefundTransaction(pblock);
    this->nBlockSigOpsCost += GetLegacySigOpCount(*pblock->vtx[proofTx]);

    bceResult.valueTransfers.clear();

    return true;
}

void BlockAssembler::AddToBlock(const CTxMemPoolEntry& entry)
{
    pblocktemplate->block.vtx.emplace_back(entry.GetSharedTx());
    pblocktemplate->vTxFees.push_back(entry.GetFee());
    pblocktemplate->vTxSigOpsCost.push_back(entry.GetSigOpCost());
    nBlockWeight += entry.GetTxWeight();
    ++nBlockTx;
    nBlockSigOpsCost += entry.GetSigOpCost();
    nFees += entry.GetFee();

    if (m_options.print_modified_fee) {
        LogInfo("fee rate %s txid %s\n",
                  CFeeRate(entry.GetModifiedFee(), entry.GetTxSize()).ToString(),
                  entry.GetTx().GetHash().ToString());
    }
}

void BlockAssembler::addChunks()
{
    // Limit the number of attempts to add transactions to the block when it is
    // close to full; this is just a simple heuristic to finish quickly if the
    // mempool has a lot of entries.
    const int64_t MAX_CONSECUTIVE_FAILURES = 1000;
    constexpr int32_t BLOCK_FULL_ENOUGH_WEIGHT_DELTA = 4000;
    int64_t nConsecutiveFailed = 0;

    std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef> selected_transactions;
    selected_transactions.reserve(MAX_CLUSTER_COUNT_LIMIT);
    FeePerWeight chunk_feerate;

    // This fills selected_transactions
    chunk_feerate = m_mempool->GetBlockBuilderChunk(selected_transactions);
    FeePerVSize chunk_feerate_vsize = ToFeePerVSize(chunk_feerate);

    while (selected_transactions.size() > 0) {
        if (m_options.time_limit != 0 && TicksSinceEpoch<std::chrono::seconds>(NodeClock::now()) >= m_options.time_limit) {
            // No more time to add transactions, just exit
            return;
        }

        // Check to see if min fee rate is still respected.
        if (chunk_feerate_vsize << m_options.blockMinFeeRate.GetFeePerVSize()) {
            // Everything else we might consider has a lower feerate
            return;
        }

        int64_t chunk_sig_ops = 0;
        for (const auto& tx : selected_transactions) {
            chunk_sig_ops += tx.get().GetSigOpCost();
        }

        // Check to see if this chunk will fit.
        if (!TestChunkBlockLimits(chunk_feerate, chunk_sig_ops) || !TestChunkTransactions(selected_transactions)) {
            // This chunk won't fit, so we skip it and will try the next best one.
            m_mempool->SkipBuilderChunk();
            ++nConsecutiveFailed;

            if (nConsecutiveFailed > MAX_CONSECUTIVE_FAILURES && nBlockWeight +
                    BLOCK_FULL_ENOUGH_WEIGHT_DELTA > m_options.nBlockMaxWeight) {
                // Give up if we're close to full and haven't succeeded in a while
                return;
            }
        } else {
            m_mempool->IncludeBuilderChunk();

            // This chunk will fit, so add it to the block.
            nConsecutiveFailed = 0;
            bool wasAdded=true;
            for (const auto& tx : selected_transactions) {
                if (!wasAdded || (m_options.time_limit != 0 && TicksSinceEpoch<std::chrono::seconds>(NodeClock::now()) >= m_options.time_limit)) {
                    // If out of time, or earlier ancestor failed, then skip the rest of the transactions
                    wasAdded=false;
                    break;
                }
                if (wasAdded) {
                    if (tx.get().GetTx().HasCreateOrCall()) {
                        wasAdded = AttemptToAddContractToBlock(tx);
                    } else {
                        AddToBlock(tx);
                    }
                }
            }

            // Skip update packages if a transaction failed to be added (match test package logic)
            if (wasAdded) {
                pblocktemplate->m_package_feerates.emplace_back(chunk_feerate_vsize);
            }
        }

        selected_transactions.clear();
        chunk_feerate = m_mempool->GetBlockBuilderChunk(selected_transactions);
        chunk_feerate_vsize = ToFeePerVSize(chunk_feerate);
    }
}

void AddMerkleRootAndCoinbase(CBlock& block, CTransactionRef coinbase, uint32_t version, uint32_t timestamp, uint32_t nonce)
{
    if (block.vtx.size() == 0) {
        block.vtx.emplace_back(coinbase);
    } else {
        block.vtx[0] = coinbase;
    }
    block.nVersion = version;
    block.nTime = timestamp;
    block.nNonce = nonce;
    block.hashMerkleRoot = BlockMerkleRoot(block);

    // Reset cached checks
    block.m_checked_witness_commitment = false;
    block.m_checked_merkle_root = false;
    block.fChecked = false;
}

void InterruptWait(KernelNotifications& kernel_notifications, bool& interrupt_wait)
{
    LOCK(kernel_notifications.m_tip_block_mutex);
    interrupt_wait = true;
    kernel_notifications.m_tip_block_cv.notify_all();
}

std::unique_ptr<CBlockTemplate> WaitAndCreateNewBlock(ChainstateManager& chainman,
                                                      KernelNotifications& kernel_notifications,
                                                      CTxMemPool* mempool,
                                                      const std::unique_ptr<CBlockTemplate>& block_template,
                                                      const BlockWaitOptions& options,
                                                      const BlockAssembler::Options& assemble_options,
                                                      bool& interrupt_wait)
{
    // Delay calculating the current template fees, just in case a new block
    // comes in before the next tick.
    CAmount current_fees = -1;

    // Alternate waiting for a new tip and checking if fees have risen.
    // The latter check is expensive so we only run it once per second.
    auto now{NodeClock::now()};
    const auto deadline = now + options.timeout;
    const MillisecondsDouble tick{1000};
    const bool allow_min_difficulty{chainman.GetParams().GetConsensus().fPowAllowMinDifficultyBlocks};

    do {
        bool tip_changed{false};
        {
            WAIT_LOCK(kernel_notifications.m_tip_block_mutex, lock);
            // Note that wait_until() checks the predicate before waiting
            kernel_notifications.m_tip_block_cv.wait_until(lock, std::min(now + tick, deadline), [&]() EXCLUSIVE_LOCKS_REQUIRED(kernel_notifications.m_tip_block_mutex) {
                AssertLockHeld(kernel_notifications.m_tip_block_mutex);
                const auto tip_block{kernel_notifications.TipBlock()};
                // We assume tip_block is set, because this is an instance
                // method on BlockTemplate and no template could have been
                // generated before a tip exists.
                tip_changed = Assume(tip_block) && tip_block != block_template->block.hashPrevBlock;
                return tip_changed || chainman.m_interrupt || interrupt_wait;
            });
            if (interrupt_wait) {
                interrupt_wait = false;
                return nullptr;
            }
        }

        if (chainman.m_interrupt) return nullptr;
        // At this point the tip changed, a full tick went by or we reached
        // the deadline.

        // Must release m_tip_block_mutex before locking cs_main, to avoid deadlocks.
        LOCK(::cs_main);

        // On test networks return a minimum difficulty block after 20 minutes
        if (!tip_changed && allow_min_difficulty) {
            const NodeClock::time_point tip_time{std::chrono::seconds{chainman.ActiveChain().Tip()->GetBlockTime()}};
            if (now > tip_time + 20min) {
                tip_changed = true;
            }
        }

        /**
         * We determine if fees increased compared to the previous template by generating
         * a fresh template. There may be more efficient ways to determine how much
         * (approximate) fees for the next block increased, perhaps more so after
         * Cluster Mempool.
         *
         * We'll also create a new template if the tip changed during this iteration.
         */
        if (options.fee_threshold < MAX_MONEY || tip_changed) {
            auto new_tmpl{BlockAssembler{
                chainman.ActiveChainstate(),
                mempool,
                assemble_options}
                              .CreateNewBlock()};

            // If the tip changed, return the new template regardless of its fees.
            if (tip_changed) return new_tmpl;

            // Calculate the original template total fees if we haven't already
            if (current_fees == -1) {
                current_fees = std::accumulate(block_template->vTxFees.begin(), block_template->vTxFees.end(), CAmount{0});
            }

            // Check if fees increased enough to return the new template
            const CAmount new_fees = std::accumulate(new_tmpl->vTxFees.begin(), new_tmpl->vTxFees.end(), CAmount{0});
            Assume(options.fee_threshold != MAX_MONEY);
            if (new_fees >= current_fees + options.fee_threshold) return new_tmpl;
        }

        now = NodeClock::now();
    } while (now < deadline);

    return nullptr;
}

std::optional<BlockRef> GetTip(ChainstateManager& chainman)
{
    LOCK(::cs_main);
    CBlockIndex* tip{chainman.ActiveChain().Tip()};
    if (!tip) return {};
    return BlockRef{tip->GetBlockHash(), tip->nHeight};
}

bool CooldownIfHeadersAhead(ChainstateManager& chainman, KernelNotifications& kernel_notifications, const BlockRef& last_tip, bool& interrupt_mining)
{
    uint256 last_tip_hash{last_tip.hash};

    while (const std::optional<int> remaining = chainman.BlocksAheadOfTip()) {
        const int cooldown_seconds = std::clamp(*remaining, 3, 20);
        const auto cooldown_deadline{MockableSteadyClock::now() + std::chrono::seconds{cooldown_seconds}};

        {
            WAIT_LOCK(kernel_notifications.m_tip_block_mutex, lock);
            kernel_notifications.m_tip_block_cv.wait_until(lock, cooldown_deadline, [&]() EXCLUSIVE_LOCKS_REQUIRED(kernel_notifications.m_tip_block_mutex) {
                const auto tip_block = kernel_notifications.TipBlock();
                return chainman.m_interrupt || interrupt_mining || (tip_block && *tip_block != last_tip_hash);
            });
            if (chainman.m_interrupt || interrupt_mining) {
                interrupt_mining = false;
                return false;
            }

            // If the tip changed during the wait, extend the deadline
            const auto tip_block = kernel_notifications.TipBlock();
            if (tip_block && *tip_block != last_tip_hash) {
                last_tip_hash = *tip_block;
                continue;
            }
        }

        // No tip change and the cooldown window has expired.
        if (MockableSteadyClock::now() >= cooldown_deadline) break;
    }

    return true;
}

std::optional<BlockRef> WaitTipChanged(ChainstateManager& chainman, KernelNotifications& kernel_notifications, const uint256& current_tip, MillisecondsDouble& timeout, bool& interrupt)
{
    Assume(timeout >= 0ms); // No internal callers should use a negative timeout
    if (timeout < 0ms) timeout = 0ms;
    if (timeout > std::chrono::years{100}) timeout = std::chrono::years{100}; // Upper bound to avoid UB in std::chrono
    auto deadline{std::chrono::steady_clock::now() + timeout};
    {
        WAIT_LOCK(kernel_notifications.m_tip_block_mutex, lock);
        // For callers convenience, wait longer than the provided timeout
        // during startup for the tip to be non-null. That way this function
        // always returns valid tip information when possible and only
        // returns null when shutting down, not when timing out.
        kernel_notifications.m_tip_block_cv.wait(lock, [&]() EXCLUSIVE_LOCKS_REQUIRED(kernel_notifications.m_tip_block_mutex) {
            return kernel_notifications.TipBlock() || chainman.m_interrupt || interrupt;
        });
        if (chainman.m_interrupt || interrupt) {
            interrupt = false;
            return {};
        }
        // At this point TipBlock is set, so continue to wait until it is
        // different then `current_tip` provided by caller.
        kernel_notifications.m_tip_block_cv.wait_until(lock, deadline, [&]() EXCLUSIVE_LOCKS_REQUIRED(kernel_notifications.m_tip_block_mutex) {
            return Assume(kernel_notifications.TipBlock()) != current_tip || chainman.m_interrupt || interrupt;
        });
        if (chainman.m_interrupt || interrupt) {
            interrupt = false;
            return {};
        }
    }

    // Must release m_tip_block_mutex before getTip() locks cs_main, to
    // avoid deadlocks.
    return GetTip(chainman);
}

bool CanStake()
{
    bool canStake = gArgs.GetBoolArg("-staking", DEFAULT_STAKE);

    if(canStake)
    {
        // Signet is for creating PoW blocks by an authorized signer
        canStake = !Params().GetConsensus().signet_blocks;
    }

    return canStake;
}

#ifdef ENABLE_WALLET
//////////////////////////////////////////////////////////////////////////////
//
// Proof of Stake miner
//

//
// Looking for suitable coins for creating new block.
//

class DelegationFilterBase : public IDelegationFilter
{
public:
    bool GetKey(const std::string& strAddress, uint160& keyId)
    {
        CTxDestination destination = DecodeDestination(strAddress);
        if (!IsValidDestination(destination)) {
            return false;
        }

        if (!std::holds_alternative<PKHash>(destination)) {
            return false;
        }

        keyId = uint160(std::get<PKHash>(destination));

        return true;
    }
};

class DelegationsStaker : public DelegationFilterBase
{
public:
    enum StakerType
    {
        STAKER_NORMAL    = 0,
        STAKER_ALLOWLIST = 1,
        STAKER_EXCLUDELIST = 2,
    };

    DelegationsStaker(wallet::CWallet *_pwallet):
        pwallet(_pwallet),
        cacheHeight(0),
        type(StakerType::STAKER_NORMAL)
    {
        // Get allow list
        for (const std::string& strAddress : gArgs.GetArgs("-stakingallowlist"))
        {
            uint160 keyId;
            if(GetKey(strAddress, keyId))
            {
                if(std::find(allowList.begin(), allowList.end(), keyId) == allowList.end())
                    allowList.push_back(keyId);
            }
            else
            {
                LogDebug(BCLog::COINSTAKE, "Fail to add %s to stake allow list\n", strAddress);
            }
        }

        // Get exclude list
        for (const std::string& strAddress : gArgs.GetArgs("-stakingexcludelist"))
        {
            uint160 keyId;
            if(GetKey(strAddress, keyId))
            {
                if(std::find(excludeList.begin(), excludeList.end(), keyId) == excludeList.end())
                    excludeList.push_back(keyId);
            }
            else
            {
                LogDebug(BCLog::COINSTAKE, "Fail to add %s to stake exclude list\n", strAddress);
            }
        }

        // Set staker type
        if(allowList.size() > 0)
        {
            type = StakerType::STAKER_ALLOWLIST;
        }
        else if(excludeList.size() > 0)
        {
            type = StakerType::STAKER_EXCLUDELIST;
        }
    }

    bool Match(const DelegationEvent& event) const override
    {
        bool mine = pwallet->HasPrivateKey(PKHash(event.item.staker));
        if(!mine)
            return false;

        wallet::CSuperStakerInfo info;
        if(pwallet->GetSuperStaker(info, event.item.staker) && info.fCustomConfig)
        {
            return CheckAddressList(info.nDelegateAddressType, info.delegateAddressList, info.delegateAddressList, event);
        }

        return CheckAddressList(type, allowList, excludeList, event);
    }

    bool CheckAddressList(const int& _type, const std::vector<uint160>& _allowList, const std::vector<uint160>& _excludeList, const DelegationEvent& event) const
    {
        switch (_type) {
        case STAKER_NORMAL:
            return true;
        case STAKER_ALLOWLIST:
            return std::count(_allowList.begin(), _allowList.end(), event.item.delegate);
        case STAKER_EXCLUDELIST:
            return std::count(_excludeList.begin(), _excludeList.end(), event.item.delegate) == 0;
        default:
            break;
        }

        return false;
    }

    void Update(int32_t nHeight)
    {
        if(pwallet->fUpdatedSuperStaker)
        {
            // Clear cache if updated
            cacheHeight = 0;
            cacheDelegationsStaker.clear();
            pwallet->fUpdatedSuperStaker = false;
        }

        std::map<uint160, Delegation> delegations_staker;
        int checkpointSpan = Params().GetConsensus().CheckpointSpan(nHeight);
        if(nHeight <= checkpointSpan)
        {
            // Get delegations from events
            std::vector<DelegationEvent> events;
            qtumDelegations.FilterDelegationEvents(events, *this, pwallet->chain().chainman());
            delegations_staker = qtumDelegations.DelegationsFromEvents(events);
        }
        else
        {
            // Update the cached delegations for the staker, older then the sync checkpoint (500 blocks)
            int cpsHeight = nHeight - checkpointSpan;
            if(cacheHeight < cpsHeight)
            {
                std::vector<DelegationEvent> events;
                qtumDelegations.FilterDelegationEvents(events, *this, pwallet->chain().chainman(), cacheHeight, cpsHeight);
                qtumDelegations.UpdateDelegationsFromEvents(events, cacheDelegationsStaker);
                cacheHeight = cpsHeight;
            }

            // Update the wallet delegations
            std::vector<DelegationEvent> events;
            qtumDelegations.FilterDelegationEvents(events, *this, pwallet->chain().chainman(), cacheHeight + 1);
            delegations_staker = cacheDelegationsStaker;
            qtumDelegations.UpdateDelegationsFromEvents(events, delegations_staker);
        }
        pwallet->updateDelegationsStaker(delegations_staker);
    }

private:
    wallet::CWallet *pwallet;
    QtumDelegation qtumDelegations;
    int32_t cacheHeight;
    std::map<uint160, Delegation> cacheDelegationsStaker;
    std::vector<uint160> allowList;
    std::vector<uint160> excludeList;
    int type;
};

class MyDelegations : public DelegationFilterBase
{
public:
    MyDelegations(wallet::CWallet *_pwallet):
        pwallet(_pwallet),
        cacheHeight(0),
        cacheAddressHeight(0)
    {}

    bool Match(const DelegationEvent& event) const override
    {
        return pwallet->HasPrivateKey(PKHash(event.item.delegate));
    }

    void Update(int32_t nHeight)
    {
        if(fLogEvents)
        {
            // When log events are enabled, search the log events to get complete list of my delegations
            int checkpointSpan = Params().GetConsensus().CheckpointSpan(nHeight);
            if(nHeight <= checkpointSpan)
            {
                // Get delegations from events
                std::vector<DelegationEvent> events;
                qtumDelegations.FilterDelegationEvents(events, *this, pwallet->chain().chainman());
                pwallet->m_my_delegations = qtumDelegations.DelegationsFromEvents(events);
            }
            else
            {
                // Update the cached delegations for the staker, older then the sync checkpoint (500 blocks)
                int cpsHeight = nHeight - checkpointSpan;
                if(cacheHeight < cpsHeight)
                {
                    std::vector<DelegationEvent> events;
                    qtumDelegations.FilterDelegationEvents(events, *this, pwallet->chain().chainman(), cacheHeight, cpsHeight);
                    qtumDelegations.UpdateDelegationsFromEvents(events, cacheMyDelegations);
                    cacheHeight = cpsHeight;
                }

                // Update the wallet delegations
                std::vector<DelegationEvent> events;
                qtumDelegations.FilterDelegationEvents(events, *this, pwallet->chain().chainman(), cacheHeight + 1);
                pwallet->m_my_delegations = cacheMyDelegations;
                qtumDelegations.UpdateDelegationsFromEvents(events, pwallet->m_my_delegations);
            }
        }
        else
        {
            // Log events are not enabled, search the available addresses for list of my delegations
            if(cacheHeight != nHeight)
            {
                cacheMyDelegations.clear();

                // Address map
                std::map<uint160, bool> mapAddress;

                // Get all addresses with coins
                SelectAddress(mapAddress, nHeight);

                // Get all addresses for delegations in the GUI
                for(auto item : pwallet->mapDelegation)
                {
                    uint160 address = item.second.delegateAddress;
                    if(pwallet->HasPrivateKey(PKHash(address)))
                    {
                        if (mapAddress.find(address) == mapAddress.end())
                        {
                            mapAddress[address] = false;
                        }
                    }
                }

                // Search my delegations in the addresses
                for(auto item: mapAddress)
                {
                    Delegation delegation;
                    uint160 address = item.first;
                    if(qtumDelegations.GetDelegation(address, delegation, pwallet->chain().chainman().ActiveChainstate()) && QtumDelegation::VerifyDelegation(address, delegation))
                    {
                        cacheMyDelegations[address] = delegation;
                    }
                }

                // Update my delegations list
                pwallet->m_my_delegations = cacheMyDelegations;
                cacheHeight = nHeight;
            }
        }
    }

    void SelectAddress(std::map<uint160, bool>& mapAddress, int32_t nHeight)
    {
        if(cacheAddressHeight < nHeight)
        {
            wallet::SelectAddress(*pwallet, mapAddress);
            pwallet->mapAddressUnspentCache = mapAddress;
            if(pwallet->fUpdateAddressUnspentCache == false)
                pwallet->fUpdateAddressUnspentCache = true;
            cacheAddressHeight = nHeight + 100;
        }
        else
        {
            mapAddress = pwallet->mapAddressUnspentCache;
        }
    }

private:

    wallet::CWallet *pwallet;
    QtumDelegation qtumDelegations;
    int32_t cacheHeight;
    int32_t cacheAddressHeight;
    std::map<uint160, Delegation> cacheMyDelegations;
};

bool CheckStake(const std::shared_ptr<const CBlock> pblock, wallet::CWallet& wallet)
{
    uint256 proofHash, hashTarget;
    uint256 hashBlock = pblock->GetHash();

    if(!pblock->IsProofOfStake()) {
        LogError("CheckStake() : %s is not a proof-of-stake block", hashBlock.GetHex());
        return false;
    }

    // verify hash target and signature of coinstake tx
    {
        LOCK(cs_main);
        BlockValidationState state;
        CBlockIndex* pindexPrev = &(wallet.chain().chainman().BlockIndex()[pblock->hashPrevBlock]);
        if (!CheckProofOfStake(pindexPrev, state, *pblock->vtx[1], pblock->nBits, pblock->nTime, pblock->GetProofOfDelegation(), pblock->prevoutStake, proofHash, hashTarget, wallet.chain().getCoinsTip(), wallet.chain().chainman().ActiveChainstate())) {
            LogError("CheckStake() : proof-of-stake checking failed %s",state.GetRejectReason());
            return false;
        }
    }

    //// debug print
    LogDebug(BCLog::COINSTAKE, "CheckStake() : new proof-of-stake block found  \n  hash: %s \nproofhash: %s  \ntarget: %s\n", hashBlock.GetHex(), proofHash.GetHex(), hashTarget.GetHex());
    LogDebug(BCLog::COINSTAKE, "%s\n", pblock->ToString());
    LogDebug(BCLog::COINSTAKE, "out %s\n", FormatMoney(pblock->vtx[1]->GetValueOut()));

    // Found a solution
    {
        LOCK(cs_main);
        if (pblock->hashPrevBlock != wallet.chain().getTip()->GetBlockHash()) {
            LogError("CheckStake() : generated block is stale");
            return false;
        }
    }
    {
        LOCK(wallet.cs_wallet);
        for(const CTxIn& vin : pblock->vtx[1]->vin) {
            COutPoint prevout(vin.prevout.hash, vin.prevout.n);
            if (wallet.IsSpent(prevout)) {
                LogError("CheckStake() : generated block became invalid due to stake UTXO being spent");
                return false;
            }
        }
    }

    // Process this block the same as if we had received it from another node
    bool fNewBlock = false;
    if (!wallet.chain().chainman().ProcessNewBlock(pblock, true, true, &fNewBlock)) {
        LogError("CheckStake() : ProcessBlock, block not accepted");
        return false;
    }

    return true;
}

bool SleepStaker(wallet::CWallet *pwallet, uint64_t milliseconds)
{
    uint64_t seconds = milliseconds / 1000;
    milliseconds %= 1000;

    for(unsigned int i = 0; i < seconds; i++)
    {
        if(!pwallet->IsStakeClosing())
            UninterruptibleSleep(std::chrono::seconds{1});
        else
            return false;
    }

    if(milliseconds)
    {
        if(!pwallet->IsStakeClosing())
            UninterruptibleSleep(std::chrono::milliseconds{milliseconds});
        else
            return false;
    }

    return !pwallet->IsStakeClosing();
}

bool SignBlockHWI(std::shared_ptr<CBlock> pblock, wallet::CWallet& wallet, std::vector<unsigned char>& vchSig)
{
    // Check ledger ID
    if(wallet.m_ledger_id == "") {
        return false;
    }
    QtumLedger &device = QtumLedger::instance();

    // Make a blank psbt
    PartiallySignedTransaction psbtx_in;
    CMutableTransaction rawTx = CMutableTransaction(*pblock->vtx[1]);
    psbtx_in.tx = rawTx;
    for (unsigned int i = 0; i < rawTx.vin.size(); ++i) {
        psbtx_in.inputs.push_back(PSBTInput());
    }
    for (unsigned int i = 0; i < rawTx.vout.size(); ++i) {
        psbtx_in.outputs.push_back(PSBTOutput());
    }

    // Get staker path
    CScript stakerPubKey = rawTx.vout[1].scriptPubKey;
    CTxDestination txStakerDest = ExtractPublicKeyHash(stakerPubKey);
    std::string strStaker;
    if(!wallet.GetHDKeyPath(txStakerDest, strStaker)) {
        return false;
    }

    // Fill transaction with out data but don't sign
    bool bip32derivs = true;
    bool complete = true;
    const auto err{wallet.FillPSBT(psbtx_in, complete, 1, false, bip32derivs)};
    if (err) {
        return false;
    }

    // Serialize the PSBT
    if(wallet.IsStakeClosing()) return false;
    DataStream ssTx;
    ssTx << TX_WITH_WITNESS(psbtx_in);
    std::string psbt = EncodeBase64(ssTx.str());
    if(!device.signCoinStake(wallet.m_ledger_id, psbt)) {
        return false;
    }

    // Unserialize the transactions
    PartiallySignedTransaction psbtx_out;
    std::string error;
    if (!DecodeBase64PSBT(psbtx_out, psbt, error)) {
        return false;
    }

    // Update block proof
    CMutableTransaction txCoinStake;
    complete = FinalizeAndExtractPSBT(psbtx_out, txCoinStake);
    if(!complete) {
        return false;
    }
    pblock->vtx[1] = MakeTransactionRef(std::move(txCoinStake));
    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);

    // Sign block header
    if(wallet.IsStakeClosing()) return false;
    std::string header = pblock->GetWithoutSign();
    if(!device.signBlockHeader(wallet.m_ledger_id, header, strStaker, vchSig)) {
        return false;
    }

    return true;
}

bool SignBlockLedger(std::shared_ptr<CBlock> pblock, wallet::CWallet& wallet)
{
    LOCK(cs_ledger);
    std::vector<unsigned char> vchSig;
    bool ret = SignBlockHWI(pblock, wallet, vchSig);
    if(ret) pblock->SetBlockSignature(vchSig);
    if(!ret && !wallet.IsStakeClosing())
    {
        std::string errorMessage = QtumLedger::instance().errorMessage();
        LogInfo("WARN: %s: fail to sign block (%s)\n", __func__, errorMessage);
    }
    return ret;
}

// novacoin: attempt to generate suitable proof-of-stake
bool SignBlock(std::shared_ptr<CBlock> pblock, wallet::CWallet& wallet, const CAmount& nTotalFees, uint32_t nTime, std::set<std::pair<const wallet::CWalletTx*,unsigned int> >& setCoins, std::vector<COutPoint>& setSelectedCoins, std::vector<COutPoint>& setDelegateCoins, bool selectedOnly = false, bool tryOnly = false)
{
    // if we are trying to sign
    //    something except proof-of-stake block template
    if (!CheckFirstCoinstakeOutput(*pblock))
        return false;

    // if we are trying to sign
    //    a complete proof-of-stake block
    if (pblock->IsProofOfStake() && !pblock->vchBlockSigDlgt.empty())
        return true;

    PKHash pkhash;
    CMutableTransaction txCoinStake(*pblock->vtx[1]);
    uint32_t nTimeBlock = nTime;
    std::vector<unsigned char> vchPoD;
    COutPoint headerPrevout;
    //original line:
    //int64_t nSearchInterval = IsProtocolV2(nBestHeight+1) ? 1 : nSearchTime - nLastCoinStakeSearchTime;
    //IsProtocolV2 mean POS 2 or higher, so the modified line is:
    if(wallet.IsStakeClosing()) return false;
    LOCK(wallet.cs_wallet);
    uint32_t nHeight = wallet.chain().getHeight().value_or(0) + 1;
    const Consensus::Params& consensusParams = Params().GetConsensus();
    nTimeBlock &= ~consensusParams.StakeTimestampMask(nHeight);
    bool privateKeysDisabled = wallet.IsWalletFlagSet(wallet::WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    bool found = false;
    {
        LOCK(cs_main);
        found = wallet::CreateCoinStake(wallet, pblock->nBits, nTotalFees, nTimeBlock, txCoinStake, pkhash, setCoins, setSelectedCoins, setDelegateCoins, selectedOnly, !privateKeysDisabled, vchPoD, headerPrevout);
    }
    if (found)
    {
        if (nTimeBlock >= wallet.chain().getTip()->GetMedianTimePast()+1)
        {
            // make sure coinstake would meet timestamp protocol
            //    as it would be the same as the block timestamp
            pblock->nTime = nTimeBlock;
            pblock->vtx[1] = MakeTransactionRef(std::move(txCoinStake));
            pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
            pblock->prevoutStake = headerPrevout;

            if(tryOnly)
                return true;

            // Check timestamp against prev
            if(pblock->GetBlockTime() <= wallet.chain().getTip()->GetBlockTime() || FutureDrift(pblock->GetBlockTime(), nHeight, consensusParams) < wallet.chain().getTip()->GetBlockTime())
            {
                return false;
            }

            // Sign block
            if (wallet.chain().getHeight().value_or(0) + 1 >= consensusParams.nOfflineStakeHeight)
            {
                // append PoD to the end of the block header
                if(vchPoD.size() > 0)
                    pblock->SetProofOfDelegation(vchPoD);

                // append a signature to our block, ensure that is compact and check block header
                bool isSigned = privateKeysDisabled ? SignBlockLedger(pblock, wallet) : wallet.SignBlockStake(*pblock, pkhash, true);
                return isSigned && CheckHeaderProof(*pblock, consensusParams, wallet.chain().chainman().ActiveChainstate());
            }
            else
            {
                // append a signature to our block and ensure that is LowS
                return wallet.SignBlockStake(*pblock, pkhash, false) &&
                           EnsureLowS(pblock->vchBlockSigDlgt) &&
                           CheckHeaderProof(*pblock, consensusParams, wallet.chain().chainman().ActiveChainstate());
            }
        }
    }

    return false;
}

/**
 * @brief The IStakeMiner class Miner interface
 */
class IStakeMiner
{
public:
    /**
     * @brief init Initialize the miner
     * @param pwallet Wallet to use
     */
    virtual void Init(wallet::CWallet *pwallet) = 0;

    /**
     * @brief run Run the miner
     */
    virtual void Run() = 0;

    /**
     * @brief ~IStakeMiner Destructor
     */
    virtual ~IStakeMiner() {};
};

class SolveItem
{
public:
    SolveItem(const COutPoint& _prevoutStake, const uint32_t& _blockTime, const bool& _delegate):
        prevoutStake(_prevoutStake),
        blockTime(_blockTime),
        delegate(_delegate)
    {}

    COutPoint prevoutStake;
    uint32_t blockTime = 0;
    bool delegate = false;
};

class StakeMinerPriv
{
public:
    wallet::CWallet *pwallet = 0;
    bool fTryToSync = true;
    bool regtestMode = false;
    bool minDifficulty = false;
    bool fSuperStake = false;
    const Consensus::Params& consensusParams;
    int nOfflineStakeHeight = 0;
    bool fDelegationsContract = false;
    bool fEmergencyStaking = false;
    bool fAggressiveStaking = false;
    bool fError = false;
    int numThreads = 1;
    boost::thread_group threads;
    mutable RecursiveMutex cs_worker;
    bool privateKeysDisabled = false;

public:
    DelegationsStaker delegationsStaker;
    MyDelegations myDelegations;

public:
    int32_t nHeight = 0;
    uint32_t stakeTimestampMask = 1;
    int64_t nTotalFees = 0;
    bool haveCoinsForStake = false;
    bool forceUpdate = false;

    CBlockIndex* pindexPrev = 0;
    CAmount nTargetValue = 0;
    std::set<std::pair<const wallet::CWalletTx*,unsigned int> > setCoins;
    std::vector<COutPoint> setSelectedCoins;
    std::vector<COutPoint> setDelegateCoins;
    std::vector<COutPoint> prevouts;
    std::map<uint32_t, bool> mapSolveBlockTime;
    std::multimap<uint256, SolveItem> mapSolvedBlock;
    std::map<uint32_t, std::vector<COutPoint>> mapSolveSelectedCoins;
    std::map<uint32_t, std::vector<COutPoint>> mapSolveDelegateCoins;
    uint32_t beginningTime = 0;
    uint32_t endingTime = 0;
    uint32_t waitBestHeaderAttempts = 0;

    std::shared_ptr<CBlock> pblock;
    std::unique_ptr<CBlockTemplate> pblocktemplate;
    std::shared_ptr<CBlock> pblockfilled;
    std::unique_ptr<CBlockTemplate> pblocktemplatefilled;

public:
    StakeMinerPriv(wallet::CWallet *_pwallet):
        pwallet(_pwallet),
        consensusParams(Params().GetConsensus()),
        delegationsStaker(_pwallet),
        myDelegations(_pwallet)

    {
        // Make this thread recognisable as the mining thread
        std::string threadName = "qtumstake";
        if(pwallet && pwallet->GetName() != "")
        {
            threadName = threadName + "-" + pwallet->GetName();
        }
        util::ThreadRename(threadName.c_str());

        regtestMode = Params().MineBlocksOnDemand();
        minDifficulty = consensusParams.fPowAllowMinDifficultyBlocks;
        fSuperStake = gArgs.GetBoolArg("-superstaking", DEFAULT_SUPER_STAKE);
        nOfflineStakeHeight = consensusParams.nOfflineStakeHeight;
        fDelegationsContract = !consensusParams.delegationsAddress.IsNull();
        fEmergencyStaking = gArgs.GetBoolArg("-emergencystaking", false);
        fAggressiveStaking = gArgs.IsArgSet("-aggressive-staking");
        int maxWaitForBestHeader = gArgs.GetIntArg("-maxstakerwaitforbestheader", node::DEFAULT_MAX_STAKER_WAIT_FOR_BEST_BLOCK_HEADER);
        if(maxWaitForBestHeader > 0)
        {
            waitBestHeaderAttempts = maxWaitForBestHeader / nMinerWaitBestBlockHeader;
        }
        if(pwallet) numThreads = pwallet->m_num_threads;
        if(pwallet) privateKeysDisabled = pwallet->IsWalletFlagSet(wallet::WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    }

    void clearCache()
    {
        nHeight = 0;
        stakeTimestampMask = 1;
        nTotalFees = 0;
        haveCoinsForStake = false;
        forceUpdate = false;

        pindexPrev = 0;
        nTargetValue = 0;
        setCoins.clear();
        setSelectedCoins.clear();
        setDelegateCoins.clear();
        prevouts.clear();
        mapSolveBlockTime.clear();
        mapSolvedBlock.clear();
        mapSolveSelectedCoins.clear();
        mapSolveDelegateCoins.clear();
        beginningTime = 0;
        endingTime = 0;

        pblock.reset();
        pblocktemplate.reset();
        pblockfilled.reset();
        pblocktemplatefilled.reset();
    }
};

class StakeMiner : public IStakeMiner
{
private:
    StakeMinerPriv *d = 0;

public:
    void Init(wallet::CWallet *pwallet) override
    {
        d = new StakeMinerPriv(pwallet);
    }

    void Run() override
    {
        SetThreadPriority(THREAD_PRIORITY_LOWEST);

        while (Next()) {
            // Is ready for mining
            if(!IsReady()) continue;

            // Cache mining data
            if(!CacheData()) continue;

            // Check if ledger is connected
            if(d->privateKeysDisabled)
            {
                if(!isLedgerConnected()) continue;
            }

            // Check if miner have coins for staking
            if(HaveCoinsForStake())
            {
                // Look for possibility to create a block
                d->beginningTime = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now());
                d->beginningTime &= ~d->stakeTimestampMask;
                d->endingTime = d->beginningTime + nMaxStakeLookahead;

                for(uint32_t blockTime = d->beginningTime; blockTime < d->endingTime; blockTime += d->stakeTimestampMask+1)
                {
                    // Update status bar
                    UpdateStatusBar(blockTime);

                    // Check cached data
                    if(IsCachedDataOld())
                        break;

                    // Check if block can be created
                    if(CanCreateBlock(blockTime))
                    {
                        // Create new block
                        if(!CreateNewBlock(blockTime)) break;

                        // Sign new block
                        if(SignNewBlock(blockTime)) break;
                    }
                }
            }

            // Miner sleep before the next try
            Sleep(nMinerSleep);
        }
    }

    ~StakeMiner()
    {
        if(d)
        {
            delete d;
            d = 0;
        }
    }

protected:
    bool Next()
    {
        return d && d->pwallet && !d->pwallet->IsStakeClosing() && !d->fError;
    }

    bool Sleep(uint64_t milliseconds)
    {
        return SleepStaker(d->pwallet, milliseconds);
    }

    bool IsStale(std::shared_ptr<CBlock> pblock)
    {
        if(d->pwallet->IsStakeClosing())
            return false;

        LOCK(cs_main);
        CBlockIndex* tip = d->pwallet->chain().getTip();
        return tip != d->pindexPrev || tip->GetBlockHash() != pblock->hashPrevBlock;
    }

    bool IsReady()
    {
        // Check if wallet is ready
        while (d->pwallet->IsLocked() || !d->pwallet->m_enabled_staking || 
               d->pwallet->chain().chainman().m_blockman.LoadingBlocks())
        {
            d->pwallet->m_last_coin_stake_search_interval = 0;
            if(!Sleep(10000))
                return false;
        }

        // Wait for node connections
        // Don't disable PoS mining for no connections if in regtest mode
        if(!d->minDifficulty && !d->fEmergencyStaking) {
            while (d->pwallet->chain().getNodeCount(ConnectionDirection::Both) == 0 || d->pwallet->chain().isInitialBlockDownload()) {
                d->pwallet->m_last_coin_stake_search_interval = 0;
                d->fTryToSync = true;
                if(!Sleep(1000))
                    return false;
            }
            if (d->fTryToSync) {
                d->fTryToSync = false;
                if (d->pwallet->chain().getNodeCount(ConnectionDirection::Both) < 3 ||
                    d->pwallet->chain().getTip()->GetBlockTime() < GetTime() - 10 * 60) {
                    Sleep(60000);
                    return false;
                }
            }
        }

        // Check if cached data is old
        uint32_t blokTime = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now());
        blokTime &= ~d->stakeTimestampMask;
        if(!IsCachedDataOld() && d->endingTime >= blokTime)
        {
            Sleep(100);
            return false;
        }

        // Wait for PoW block time in regtest mode
        if(d->regtestMode) {
            bool waitForBlockTime = false;
            {
                if(d->pwallet->IsStakeClosing()) return false;
                LOCK(cs_main);
                CBlockIndex* tip = d->pwallet->chain().getTip();
                if(tip && tip->IsProofOfWork() && tip->GetBlockTime() > GetTime()) {
                    waitForBlockTime = true;
                }
            }
            // Wait for generated PoW block time
            if(waitForBlockTime) {
                Sleep(10000);
                return false;
            }
        }

        if(d->pwallet->fCleanCoinStake)
            return false;

        return true;
    }

    bool IsCachedDataOld()
    {
        if(d->pwallet->IsStakeClosing()) return false;
        if(d->pindexPrev == 0 || d->forceUpdate) return true;
        LOCK(cs_main);
        return d->pwallet->chain().getTip() != d->pindexPrev;
    }

    bool WaitBestHeader()
    {
        if(d->pwallet->IsStakeClosing()) return false;
        if(d->fEmergencyStaking || d->fAggressiveStaking) return false;
        LOCK(cs_main);
        CBlockIndex* tip = d->pwallet->chain().getTip();
        CBlockIndex* bestHeader = d->pwallet->chain().chainman().m_best_header;
        if(bestHeader!= 0 &&
                tip != 0 &&
                tip != bestHeader &&
                tip->nHeight < bestHeader->nHeight)
        {
            return true;
        }

        return false;
    }

    bool SyncWithMiners()
    {
        // Try sync with mines
        for(size_t i = 0; i < d->waitBestHeaderAttempts; i++)
        {
            if(WaitBestHeader())
            {
                if(!Sleep(nMinerWaitBestBlockHeader))
                    return false;
            }
            else
            {
                break;
            }
        }

        return true;
    }
    bool UpdateData()
    {
        if(d->pwallet->IsStakeClosing()) return false;
        LOCK(d->pwallet->cs_wallet);

        d->clearCache();
        const auto bal = wallet::GetBalance(*d->pwallet);
        CAmount nBalance = bal.m_mine_trusted;
        d->nTargetValue = nBalance - d->pwallet->m_reserve_balance;
        CAmount nValueIn = 0;
        int32_t nHeightTip = 0;
        {
            LOCK(cs_main);
            d->pindexPrev = d->pwallet->chain().getTip();
            nHeightTip = d->pwallet->chain().getHeight().value_or(0);
        }
        d->nHeight = nHeightTip + 1;
        updateMinerParams(d->nHeight, d->consensusParams, d->minDifficulty);
        bool fOfflineStakeEnabled = (d->nHeight > d->nOfflineStakeHeight) && d->fDelegationsContract;
        if(fOfflineStakeEnabled)
        {
            d->myDelegations.Update(nHeightTip);
        }
        wallet::SelectCoinsForStaking(*d->pwallet, d->nTargetValue, d->setCoins, nValueIn);
        if(d->fSuperStake && fOfflineStakeEnabled)
        {
            d->delegationsStaker.Update(nHeightTip);
            std::map<uint160, CAmount> mDelegateWeight;
            wallet::SelectDelegateCoinsForStaking(*d->pwallet, d->setDelegateCoins, mDelegateWeight);
            d->pwallet->updateDelegationsWeight(mDelegateWeight);
            d->pwallet->updateHaveCoinSuperStaker(d->setCoins);
        }
        d->stakeTimestampMask = d->consensusParams.StakeTimestampMask(d->nHeight);

        d->haveCoinsForStake = d->setCoins.size() > 0 || d->pwallet->CanSuperStake(d->setCoins, d->setDelegateCoins);
        if(d->haveCoinsForStake)
        {
            // Create an empty block. No need to process transactions until we know we can create a block
            d->nTotalFees = 0;
            BlockAssembler::Options options = ConfiguredOptions();
            options.coinbase_output_script = CScript();
            options.is_coinstake = true;
            d->pblocktemplate = std::unique_ptr<CBlockTemplate>(BlockAssembler(d->pwallet->chain().chainman().ActiveChainstate(), &(d->pwallet->chain().mempool()), d->pwallet, options).CreateEmptyBlock());
            if (!d->pblocktemplate.get()) {
                d->fError = true;
                return false;
            }
            d->nTotalFees = d->pblocktemplate->nTotalFees;
            d->pblock = std::make_shared<CBlock>(d->pblocktemplate->block);

            d->prevouts.insert(d->prevouts.end(), d->setDelegateCoins.begin(), d->setDelegateCoins.end());
            for(const std::pair<const wallet::CWalletTx*,unsigned int> &pcoin : d->setCoins)
            {
                d->prevouts.push_back(COutPoint(pcoin.first->GetHash(), pcoin.second));
            }

            LOCK(cs_main);
            UpdateMinerStakeCache(*d->pwallet, true, d->prevouts, d->pindexPrev);
        }

        d->beginningTime = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now());
        d->beginningTime &= ~d->stakeTimestampMask;
        d->endingTime = d->beginningTime + nMaxStakeLookahead;

        return true;
    }

    bool CacheData()
    {
        if(IsCachedDataOld())
        {
            if(!UpdateData())
                return false;
        }

        return !d->pwallet->IsStakeClosing();
    }

    bool HaveCoinsForStake()
    {
        return d->haveCoinsForStake;
    }

    void UpdateStatusBar(const uint32_t& blockTime)
    {
        // The information is needed for status bar to determine if the staker is trying to create block and when it will be created approximately,
        if(d->pwallet->m_last_coin_stake_search_time == 0) d->pwallet->m_last_coin_stake_search_time = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now()); // startup timestamp
        // nLastCoinStakeSearchInterval > 0 mean that the staker is running
        int64_t searchInterval = blockTime - d->pwallet->m_last_coin_stake_search_time;
        if(searchInterval > 0) d->pwallet->m_last_coin_stake_search_interval = searchInterval;
    }

    void SloveBlock(uint32_t blockTime, size_t delegateSize, size_t from, size_t to)
    {
        std::multimap<uint256, SolveItem> tmpSolvedBlock;
        for(size_t i = from; i < to; i++)
        {
            const COutPoint &prevoutStake = d->prevouts[i];
            uint256 hashProofOfStake;
            if (CheckKernelCache(d->pindexPrev, d->pblock->nBits, blockTime, prevoutStake, d->pwallet->minerStakeCache, hashProofOfStake))
            {
                bool delegate = i < delegateSize;
                tmpSolvedBlock.insert(std::make_pair(hashProofOfStake, SolveItem(prevoutStake, blockTime, delegate)));
            }
        }

        if(tmpSolvedBlock.size() > 0)
        {
            LOCK(d->cs_worker);
            d->mapSolveBlockTime[blockTime] = true;
            d->mapSolvedBlock.insert(tmpSolvedBlock.begin(), tmpSolvedBlock.end());
        }
    }

    void SloveBlock(const uint32_t& blockTime)
    {
        // Init variables
        size_t listSize = d->prevouts.size();
        size_t delegateSize = d->setDelegateCoins.size();

        // Solve block
        int numThreads = std::min(d->numThreads, (int)listSize);
        if(listSize < 1000 || numThreads < 2)
        {
            SloveBlock(blockTime, delegateSize, 0, listSize);
        }
        else
        {
            size_t chunk = listSize / numThreads;
            for(int i = 0; i < numThreads; i++)
            {
                size_t from = i * chunk;
                size_t to = i == (numThreads -1) ? listSize : from + chunk;
                d->threads.create_thread([this, blockTime, delegateSize, from, to]{SloveBlock(blockTime, delegateSize, from, to);});
            }
            d->threads.join_all();
        }

        // Populate the list with the potential solwed blocks
        for (auto it = d->mapSolvedBlock.begin(); it != d->mapSolvedBlock.end(); ++it)
        {
            const SolveItem& item = (*it).second;
            if(item.delegate)
            {
                d->mapSolveDelegateCoins[item.blockTime].push_back(item.prevoutStake);
            }
            else
            {
                d->mapSolveSelectedCoins[item.blockTime].push_back(item.prevoutStake);
            }
        }
    }

    bool CanCreateBlock(const uint32_t& blockTime)
    {
        d->pblock->nTime = blockTime;
        if(d->mapSolveBlockTime.find(blockTime) == d->mapSolveBlockTime.end())
        {
            d->mapSolveBlockTime[blockTime] = false;
            SloveBlock(blockTime);
        }

        return d->mapSolveBlockTime[blockTime];
    }

    bool CreateNewBlock(const uint32_t& blockTime)
    {
        // increase priority so we can build the full PoS block ASAP to ensure the timestamp doesn't expire
        SetThreadPriority(THREAD_PRIORITY_ABOVE_NORMAL);

        if (IsStale(d->pblock)) {
            //another block was received while building ours, scrap progress
            LogInfo("ThreadStakeMiner(): Valid future PoS block was orphaned before becoming valid\n");
            return false;
        }

        // Try to create an empty PoS block to get the address of the block creator for contracts
        if (!SignBlock(d->pblock, *(d->pwallet), d->nTotalFees, blockTime, d->setCoins, d->mapSolveSelectedCoins[blockTime], d->mapSolveDelegateCoins[blockTime], true, true))
            return false;

        // Create a block that's properly populated with transactions
        BlockAssembler::Options options = ConfiguredOptions();
        options.coinbase_output_script = d->pblock->vtx[1]->vout[1].scriptPubKey;
        options.is_coinstake = true;
        options.proof_time = blockTime;
        options.time_limit = FutureDrift(TicksSinceEpoch<std::chrono::seconds>(NodeClock::now()), d->nHeight, d->consensusParams) - nStakeTimeBuffer;
        d->pblocktemplatefilled = std::unique_ptr<CBlockTemplate>(
                BlockAssembler(d->pwallet->chain().chainman().ActiveChainstate(), &(d->pwallet->chain().mempool()), d->pwallet, options).CreateNewBlock());
        if (!d->pblocktemplatefilled.get()) {
            d->fError = true;
            return false;
        }
        d->nTotalFees = d->pblocktemplatefilled->nTotalFees;

        if (IsStale(d->pblock)) {
            //another block was received while building ours, scrap progress
            LogInfo("ThreadStakeMiner(): Valid future PoS block was orphaned before becoming valid\n");
            return false;
        }

        // Sign the full block and use the timestamp from earlier for a valid stake
        d->pblockfilled = std::make_shared<CBlock>(d->pblocktemplatefilled->block);

        return true;
    }

    bool SignNewBlock(const uint32_t& blockTime)
    {
        // Try to sign the block once at specific time with the same cached data
        d->mapSolveBlockTime[blockTime] = false;

        if (SignBlock(d->pblockfilled, *(d->pwallet), d->nTotalFees, blockTime, d->setCoins, d->mapSolveSelectedCoins[blockTime], d->mapSolveDelegateCoins[blockTime], true)) {
            // Should always reach here unless we spent too much time processing transactions and the timestamp is now invalid
            // CheckStake also does CheckBlock and AcceptBlock to propagate it to the network
            bool validBlock = false;
            while(!validBlock) {
                if (IsStale(d->pblockfilled)) {
                    //another block was received while building ours, scrap progress
                    LogInfo("ThreadStakeMiner(): Valid future PoS block was orphaned before becoming valid\n");
                    break;
                }
                //check timestamps
                if (d->pblockfilled->GetBlockTime() <= d->pindexPrev->GetBlockTime() ||
                    FutureDrift(d->pblockfilled->GetBlockTime(), d->nHeight, d->consensusParams) < d->pindexPrev->GetBlockTime()) {
                    LogInfo("ThreadStakeMiner(): Valid PoS block took too long to create and has expired\n");
                    break; //timestamp too late, so ignore
                }
                if (d->pblockfilled->GetBlockTime() > FutureDrift(TicksSinceEpoch<std::chrono::seconds>(NodeClock::now()), d->nHeight, d->consensusParams)) {
                    if (d->fAggressiveStaking) {
                        //if being aggressive, then check more often to publish immediately when valid. This might allow you to find more blocks,
                        //but also increases the chance of broadcasting invalid blocks and getting DoS banned by nodes,
                        //or receiving more stale/orphan blocks than normal. Use at your own risk.
                        if(!Sleep(100)) break;
                    }else{
                        //too early, so wait 3 seconds and try again
                        if(!Sleep(nMinerWaitWalidBlock)) break;
                    }
                    continue;
                }
                //if there is mined block by other staker wait for it to download
                if(!SyncWithMiners()) break;
                validBlock=true;
            }
            if(validBlock) {
                if(!CheckStake(d->pblockfilled, *(d->pwallet)))
                    d->forceUpdate = true;
                // Update the search time when new valid block is created, needed for status bar icon
                d->pwallet->m_last_coin_stake_search_time = d->pblockfilled->GetBlockTime();
            }
            return true;
        }

        //return back to low priority
        SetThreadPriority(THREAD_PRIORITY_LOWEST);
        return false;
    }

    bool isLedgerConnected()
    {
        if(d->pwallet->IsStakeClosing())
            return false;

        std::string ledgerId;
        {
            LOCK(d->pwallet->cs_wallet);
            ledgerId = d->pwallet->m_ledger_id;
        }

        if(ledgerId.empty())
            return false;

        QtumLedger &device = QtumLedger::instance();
        bool fConnected = device.isConnected(ledgerId, true);
        if(!fConnected)
        {
            d->pwallet->m_last_coin_stake_search_interval = 0;
            LogInfo("ThreadStakeMiner(): Ledger not connected with fingerprint %s\n", d->pwallet->m_ledger_id);
            Sleep(10000);
        }

        return fConnected;
    }
};

IStakeMiner *createMiner()
{
    return new StakeMiner();
}

void ThreadStakeMiner(wallet::CWallet *pwallet)
{
    IStakeMiner* miner = createMiner();
    miner->Init(pwallet);
    miner->Run();
    delete miner;
    miner = 0;
}

void StakeQtums(bool fStake, wallet::CWallet *pwallet)
{
    if (pwallet->stakeThread != nullptr)
    {
        pwallet->stakeThread->join_all();
        delete pwallet->stakeThread;
        pwallet->stakeThread = nullptr;
    }

    if(fStake)
    {
        pwallet->stakeThread = new boost::thread_group();
        pwallet->stakeThread->create_thread(boost::bind(&ThreadStakeMiner, pwallet));
    }
}

void RefreshDelegates(wallet::CWallet *pwallet, bool refreshMyDelegates, bool refreshStakerDelegates)
{
    if(pwallet && (refreshMyDelegates || refreshStakerDelegates))
    {
        LOCK(pwallet->cs_wallet);

        DelegationsStaker delegationsStaker(pwallet);
        MyDelegations myDelegations(pwallet);

        int nOfflineStakeHeight = Params().GetConsensus().nOfflineStakeHeight;
        bool fDelegationsContract = !Params().GetConsensus().delegationsAddress.IsNull();
        int32_t nHeight = 0;
        {
            LOCK(cs_main);
            nHeight = pwallet->chain().getHeight().value_or(0);
        }
        bool fOfflineStakeEnabled = ((nHeight + 1) > nOfflineStakeHeight) && fDelegationsContract;
        if(fOfflineStakeEnabled)
        {
            if(refreshMyDelegates)
            {
                myDelegations.Update(nHeight);
            }

            if(refreshStakerDelegates)
            {
                bool fUpdatedSuperStaker = pwallet->fUpdatedSuperStaker;
                delegationsStaker.Update(nHeight);
                pwallet->fUpdatedSuperStaker = fUpdatedSuperStaker;
            }
        }
    }
}
#endif
} // namespace node
