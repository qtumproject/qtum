// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
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
#include <policy/feerate.h>
#include <policy/policy.h>
#include <pow.h>
#include <pos.h>
#include <primitives/transaction.h>
#include <util/moneystr.h>
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

int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev)
{
    int64_t nOldTime = pblock->nTime;
    int64_t nNewTime{std::max<int64_t>(pindexPrev->GetMedianTimePast() + 1, TicksSinceEpoch<std::chrono::seconds>(NodeClock::now()))};

    if (consensusParams.enforce_BIP94) {
        // Height of block to be mined.
        const int height{pindexPrev->nHeight + 1};
        if (height % consensusParams.DifficultyAdjustmentInterval(height) == 0) {
            nNewTime = std::max<int64_t>(nNewTime, pindexPrev->GetBlockTime() - MAX_TIMEWARP);
        }
    }

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
    Assert(options.coinbase_max_additional_weight <= (dgpMaxBlockWeight - 4000));
    Assert(options.coinbase_output_max_additional_sigops <= dgpMaxBlockSigOps);
    // Limit weight to between coinbase_max_additional_weight and dgpMaxBlockWeight-4K for sanity:
    // Coinbase (reserved) outputs can safely exceed -blockmaxweight, but the rest of the block template will be empty.
    options.nBlockMaxWeight = std::clamp<size_t>(options.nBlockMaxWeight, options.coinbase_max_additional_weight, dgpMaxBlockWeight - 4000);
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
}

static BlockAssembler::Options ConfiguredOptions()
{
    BlockAssembler::Options options;
    ApplyArgsManOptions(gArgs, options);
    return options;
}

#ifdef ENABLE_WALLET
BlockAssembler::BlockAssembler(Chainstate& chainstate, const CTxMemPool* mempool, wallet::CWallet *_pwallet)
    : BlockAssembler(chainstate, mempool, ConfiguredOptions())
{
    pwallet = _pwallet;
}
#endif

void BlockAssembler::resetBlock()
{
    inBlock.clear();

    // Reserve space for coinbase tx
    nBlockWeight = m_options.coinbase_max_additional_weight;
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

std::unique_ptr<CBlockTemplate> BlockAssembler::CreateNewBlock(const CScript& scriptPubKeyIn, bool fProofOfStake, int64_t* pTotalFees, int32_t txProofTime, int32_t nTimeLimit)
{
    const auto time_start{SteadyClock::now()};

    resetBlock();

    pblocktemplate.reset(new CBlockTemplate());

    if (!pblocktemplate.get()) {
        return nullptr;
    }
    CBlock* const pblock = &pblocktemplate->block; // pointer for convenience

    this->nTimeLimit = nTimeLimit;

    // Add dummy coinbase tx as first transaction
    pblock->vtx.emplace_back();
    // Add dummy coinstake tx as second transaction
    if(fProofOfStake)
        pblock->vtx.emplace_back();
    pblocktemplate->vTxFees.push_back(-1); // updated at end
    pblocktemplate->vTxSigOpsCost.push_back(-1); // updated at end

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

    if(txProofTime == 0) {
        txProofTime = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now());
    }
    if(fProofOfStake)
        txProofTime &= ~chainparams.GetConsensus().StakeTimestampMask(nHeight);
    pblock->nTime = txProofTime;
    if (!fProofOfStake)
        UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev);
    pblock->nBits = GetNextWorkRequired(pindexPrev, pblock, chainparams.GetConsensus(),fProofOfStake);

    m_lock_time_cutoff = pindexPrev->GetMedianTimePast();

    const auto time_1{SteadyClock::now()};

    m_last_block_num_txs = nBlockTx;
    m_last_block_weight = nBlockWeight;

    // Create coinbase transaction.
    CMutableTransaction coinbaseTx;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vin[0].prevout.SetNull();
    coinbaseTx.vout.resize(1);
    if (fProofOfStake)
    {
        // Make the coinbase tx empty in case of proof of stake
        coinbaseTx.vout[0].SetEmpty();
    }
    else
    {
        coinbaseTx.vout[0].scriptPubKey = scriptPubKeyIn;
        coinbaseTx.vout[0].nValue = nFees + GetBlockSubsidy(nHeight, chainparams.GetConsensus());
    }
    coinbaseTx.vin[0].scriptSig = CScript() << nHeight << OP_0;
    originalRewardTx = coinbaseTx;
    pblock->vtx[0] = MakeTransactionRef(std::move(coinbaseTx));

    // Create coinstake transaction.
    if(fProofOfStake)
    {
        CMutableTransaction coinstakeTx;
        coinstakeTx.vout.resize(2);
        coinstakeTx.vout[0].SetEmpty();
        coinstakeTx.vout[1].scriptPubKey = scriptPubKeyIn;
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
    int nPackagesSelected = 0;
    int nDescendantsUpdated = 0;
    if (m_mempool) {
        LOCK(m_mempool->cs);
        addPackageTxs(*m_mempool, nPackagesSelected, nDescendantsUpdated, minGasPrice, pblock);
    }
    pblock->hashStateRoot = uint256(h256Touint(dev::h256(globalState->rootHash())));
    pblock->hashUTXORoot = uint256(h256Touint(dev::h256(globalState->rootHashUTXO())));
    globalState->setRoot(oldHashStateRoot);
    globalState->setRootUTXO(oldHashUTXORoot);

    //this should already be populated by AddBlock in case of contracts, but if no contracts
    //then it won't get populated
    RebuildRefundTransaction(pblock);
    ////////////////////////////////////////////////////////

    pblocktemplate->vchCoinbaseCommitment = m_chainstate.m_chainman.GenerateCoinbaseCommitment(*pblock, pindexPrev, fProofOfStake);
    pblocktemplate->vTxFees[0] = -nFees;

    LogPrintf("CreateNewBlock(): block weight: %u txs: %u fees: %ld sigops %d\n", GetBlockWeight(*pblock), nBlockTx, nFees, nBlockSigOpsCost);

    // The total fee is the Fees minus the Refund
    if (pTotalFees)
        *pTotalFees = nFees - bceResult.refundSender;

    // Fill in header
    pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
    pblock->nNonce         = 0;
    pblocktemplate->vTxSigOpsCost[0] = WITNESS_SCALE_FACTOR * GetLegacySigOpCount(*pblock->vtx[0]);

    BlockValidationState state;
    if (!fProofOfStake && m_options.test_block_validity && !TestBlockValidity(state, chainparams, m_chainstate, *pblock, pindexPrev,
                                                            /*fCheckPOW=*/false, /*fCheckMerkleRoot=*/false)) {
        throw std::runtime_error(strprintf("%s: TestBlockValidity failed: %s", __func__, state.ToString()));
    }
    const auto time_2{SteadyClock::now()};

    LogPrint(BCLog::BENCH, "CreateNewBlock() packages: %.2fms (%d packages, %d updated descendants), validity: %.2fms (total %.2fms)\n",
             Ticks<MillisecondsDouble>(time_1 - time_start), nPackagesSelected, nDescendantsUpdated,
             Ticks<MillisecondsDouble>(time_2 - time_1),
             Ticks<MillisecondsDouble>(time_2 - time_start));

    return std::move(pblocktemplate);
}

std::unique_ptr<CBlockTemplate> BlockAssembler::CreateEmptyBlock(const CScript& scriptPubKeyIn, bool fProofOfStake, int64_t* pTotalFees, int32_t nTime)
{
    resetBlock();

    pblocktemplate.reset(new CBlockTemplate());

    if(!pblocktemplate.get())
        return nullptr;
    CBlock* const pblock = &pblocktemplate->block; // pointer for convenience

    // Add dummy coinbase tx as first transaction
    pblock->vtx.emplace_back();
    // Add dummy coinstake tx as second transaction
    if(fProofOfStake)
        pblock->vtx.emplace_back();
    pblocktemplate->vTxFees.push_back(-1); // updated at end
    pblocktemplate->vTxSigOpsCost.push_back(-1); // updated at end

#ifdef ENABLE_WALLET
    if(pwallet && pwallet->IsStakeClosing())
        return nullptr;
#endif
    LOCK(cs_main);
    CBlockIndex* pindexPrev = m_chainstate.m_chain.Tip();
    assert(pindexPrev != nullptr);
    nHeight = pindexPrev->nHeight + 1;

    pblock->nVersion = m_chainstate.m_chainman.m_versionbitscache.ComputeBlockVersion(pindexPrev, chainparams.GetConsensus());
    // -regtest only: allow overriding block.nVersion with
    // -blockversion=N to test forking scenarios
    if (chainparams.MineBlocksOnDemand())
        pblock->nVersion = gArgs.GetIntArg("-blockversion", pblock->nVersion);

    uint32_t txProofTime = nTime == 0 ? TicksSinceEpoch<std::chrono::seconds>(NodeClock::now()) : nTime;
    if(fProofOfStake)
        txProofTime &= ~chainparams.GetConsensus().StakeTimestampMask(nHeight);
    pblock->nTime = txProofTime;

    m_lock_time_cutoff = pindexPrev->GetMedianTimePast();

    m_last_block_num_txs = nBlockTx;
    m_last_block_weight = nBlockWeight;

    // Create coinbase transaction.
    CMutableTransaction coinbaseTx;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vin[0].prevout.SetNull();
    coinbaseTx.vout.resize(1);
    if (fProofOfStake)
    {
        // Make the coinbase tx empty in case of proof of stake
        coinbaseTx.vout[0].SetEmpty();
    }
    else
    {
        coinbaseTx.vout[0].scriptPubKey = scriptPubKeyIn;
        coinbaseTx.vout[0].nValue = nFees + GetBlockSubsidy(nHeight, chainparams.GetConsensus());
    }
    coinbaseTx.vin[0].scriptSig = CScript() << nHeight << OP_0;
    originalRewardTx = coinbaseTx;
    pblock->vtx[0] = MakeTransactionRef(std::move(coinbaseTx));

    // Create coinstake transaction.
    if(fProofOfStake)
    {
        CMutableTransaction coinstakeTx;
        coinstakeTx.vout.resize(2);
        coinstakeTx.vout[0].SetEmpty();
        coinstakeTx.vout[1].scriptPubKey = scriptPubKeyIn;
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

    pblocktemplate->vchCoinbaseCommitment = m_chainstate.m_chainman.GenerateCoinbaseCommitment(*pblock, pindexPrev, fProofOfStake);
    pblocktemplate->vTxFees[0] = -nFees;

    // The total fee is the Fees minus the Refund
    if (pTotalFees)
        *pTotalFees = nFees - bceResult.refundSender;

    // Fill in header
    pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
    if (!fProofOfStake)
        UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev);
    pblock->nBits          = GetNextWorkRequired(pindexPrev, pblock, chainparams.GetConsensus(),fProofOfStake);
    pblock->nNonce         = 0;
    pblocktemplate->vTxSigOpsCost[0] = WITNESS_SCALE_FACTOR * GetLegacySigOpCount(*pblock->vtx[0]);

    BlockValidationState state;
    if (!fProofOfStake && m_options.test_block_validity && !TestBlockValidity(state, chainparams, m_chainstate, *pblock, pindexPrev, false, false)) {
        throw std::runtime_error(strprintf("%s: TestBlockValidity failed: %s", __func__, state.ToString()));
    }

    return std::move(pblocktemplate);
}

void BlockAssembler::onlyUnconfirmed(CTxMemPool::setEntries& testSet)
{
    for (CTxMemPool::setEntries::iterator iit = testSet.begin(); iit != testSet.end(); ) {
        // Only test txs not already in the block
        if (inBlock.count((*iit)->GetSharedTx()->GetHash())) {
            testSet.erase(iit++);
        } else {
            iit++;
        }
    }
}

bool BlockAssembler::TestPackage(uint64_t packageSize, int64_t packageSigOpsCost) const
{
    // TODO: switch to weight-based accounting for packages instead of vsize-based accounting.
    if (nBlockWeight + WITNESS_SCALE_FACTOR * packageSize >= m_options.nBlockMaxWeight) {
        return false;
    }
    if (nBlockSigOpsCost + packageSigOpsCost >= (uint64_t)dgpMaxBlockSigOps) {
        return false;
    }
    return true;
}

// Perform transaction-level checks before adding to block:
// - transaction finality (locktime)
bool BlockAssembler::TestPackageTransactions(const CTxMemPool::setEntries& package) const
{
    for (CTxMemPool::txiter it : package) {
        if (!IsFinalTx(it->GetTx(), nHeight, m_lock_time_cutoff)) {
            return false;
        }
    }
    return true;
}

bool BlockAssembler::AttemptToAddContractToBlock(CTxMemPool::txiter iter, uint64_t minGasPrice, CBlock* pblock) {
    if (nTimeLimit != 0 && TicksSinceEpoch<std::chrono::seconds>(NodeClock::now()) >= nTimeLimit - nBytecodeTimeBuffer) {
        return false;
    }
    if (gArgs.GetBoolArg("-disablecontractstaking", false))
    {
        // Contract staking is disabled for the staker
        return false;
    }
    
    dev::h256 oldHashStateRoot(globalState->rootHash());
    dev::h256 oldHashUTXORoot(globalState->rootHashUTXO());
    // operate on local vars first, then later apply to `this`
    uint64_t nBlockWeight = this->nBlockWeight;
    uint64_t nBlockSigOpsCost = this->nBlockSigOpsCost;

    unsigned int contractflags = GetContractScriptFlags(nHeight, chainparams.GetConsensus());
    QtumTxConverter convert(iter->GetTx(), m_chainstate, m_mempool, NULL, &pblock->vtx, contractflags);

    ExtractQtumTX resultConverter;
    if(!convert.extractionQtumTransactions(resultConverter)){
        //this check already happens when accepting txs into mempool
        //therefore, this can only be triggered by using raw transactions on the staker itself
        LogPrintf("AttemptToAddContractToBlock(): Fail to extract contacts from tx %s\n", iter->GetTx().GetHash().ToString());
        return false;
    }
    std::vector<QtumTransaction> qtumTransactions = resultConverter.first;
    dev::u256 txGas = 0;
    for(QtumTransaction qtumTransaction : qtumTransactions){
        txGas += qtumTransaction.gas();
        if(txGas > txGasLimit) {
            // Limit the tx gas limit by the soft limit if such a limit has been specified.
            LogPrintf("AttemptToAddContractToBlock(): The gas needed is bigger than -staker-max-tx-gas-limit for the contract tx %s\n", iter->GetTx().GetHash().ToString());
            return false;
        }

        if(bceResult.usedGas + qtumTransaction.gas() > softBlockGasLimit){
            // If this transaction's gasLimit could cause block gas limit to be exceeded, then don't add it
            // Log if the contract is the only contract tx
            if(bceResult.usedGas == 0)
                LogPrintf("AttemptToAddContractToBlock(): The gas needed is bigger than -staker-soft-block-gas-limit for the contract tx %s\n", iter->GetTx().GetHash().ToString());
            return false;
        }
        if(qtumTransaction.gasPrice() < minGasPrice){
            //if this transaction's gasPrice is less than the current DGP minGasPrice don't add it
            LogPrintf("AttemptToAddContractToBlock(): The gas price is less than -staker-min-tx-gas-price for the contract tx %s\n", iter->GetTx().GetHash().ToString());
            return false;
        }
    }
    // We need to pass the DGP's block gas limit (not the soft limit) since it is consensus critical.
    ByteCodeExec exec(*pblock, qtumTransactions, hardBlockGasLimit, m_chainstate.m_chain.Tip(), m_chainstate.m_chain);
    if(!exec.performByteCode()){
        //error, don't add contract
        globalState->setRoot(oldHashStateRoot);
        globalState->setRootUTXO(oldHashUTXORoot);
        LogPrintf("AttemptToAddContractToBlock(): Perform byte code fails for the contract tx %s\n", iter->GetTx().GetHash().ToString());
        return false;
    }

    ByteCodeExecResult testExecResult;
    if(!exec.processingResults(testExecResult)){
        globalState->setRoot(oldHashStateRoot);
        globalState->setRootUTXO(oldHashUTXORoot);
        LogPrintf("AttemptToAddContractToBlock(): Processing results fails for the contract tx %s\n", iter->GetTx().GetHash().ToString());
        return false;
    }

    if(bceResult.usedGas + testExecResult.usedGas > softBlockGasLimit){
        // If this transaction could cause block gas limit to be exceeded, then don't add it
        globalState->setRoot(oldHashStateRoot);
        globalState->setRootUTXO(oldHashUTXORoot);
        // Log if the contract is the only contract tx
        if(bceResult.usedGas == 0)
            LogPrintf("AttemptToAddContractToBlock(): The gas used is bigger than -staker-soft-block-gas-limit for the contract tx %s\n", iter->GetTx().GetHash().ToString());
        return false;
    }

    //apply contractTx costs to local state
    nBlockWeight += iter->GetTxWeight();
    nBlockSigOpsCost += iter->GetSigOpCost();
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

    pblock->vtx.emplace_back(iter->GetSharedTx());
    pblocktemplate->vTxFees.push_back(iter->GetFee());
    pblocktemplate->vTxSigOpsCost.push_back(iter->GetSigOpCost());
    this->nBlockWeight += iter->GetTxWeight();
    ++nBlockTx;
    this->nBlockSigOpsCost += iter->GetSigOpCost();
    nFees += iter->GetFee();
    inBlock.insert(iter->GetSharedTx()->GetHash());

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

void BlockAssembler::AddToBlock(CTxMemPool::txiter iter)
{
    pblocktemplate->block.vtx.emplace_back(iter->GetSharedTx());
    pblocktemplate->vTxFees.push_back(iter->GetFee());
    pblocktemplate->vTxSigOpsCost.push_back(iter->GetSigOpCost());
    nBlockWeight += iter->GetTxWeight();
    ++nBlockTx;
    nBlockSigOpsCost += iter->GetSigOpCost();
    nFees += iter->GetFee();
    inBlock.insert(iter->GetSharedTx()->GetHash());

    if (m_options.print_modified_fee) {
        LogPrintf("fee rate %s txid %s\n",
                  CFeeRate(iter->GetModifiedFee(), iter->GetTxSize()).ToString(),
                  iter->GetTx().GetHash().ToString());
    }
}

/** Add descendants of given transactions to mapModifiedTx with ancestor
 * state updated assuming given transactions are inBlock. Returns number
 * of updated descendants. */
static int UpdatePackagesForAdded(const CTxMemPool& mempool,
                                  const CTxMemPool::setEntries& alreadyAdded,
                                  indexed_modified_transaction_set& mapModifiedTx) EXCLUSIVE_LOCKS_REQUIRED(mempool.cs)
{
    AssertLockHeld(mempool.cs);

    int nDescendantsUpdated = 0;
    for (CTxMemPool::txiter it : alreadyAdded) {
        CTxMemPool::setEntries descendants;
        mempool.CalculateDescendants(it, descendants);
        // Insert all descendants (not yet in block) into the modified set
        for (CTxMemPool::txiter desc : descendants) {
            if (alreadyAdded.count(desc)) {
                continue;
            }
            ++nDescendantsUpdated;
            modtxiter mit = mapModifiedTx.find(desc);
            if (mit == mapModifiedTx.end()) {
                CTxMemPoolModifiedEntry modEntry(desc);
                mit = mapModifiedTx.insert(modEntry).first;
            }
            mapModifiedTx.modify(mit, update_for_parent_inclusion(it));
        }
    }
    return nDescendantsUpdated;
}

void BlockAssembler::SortForBlock(const CTxMemPool::setEntries& package, std::vector<CTxMemPool::txiter>& sortedEntries)
{
    // Sort package by ancestor count
    // If a transaction A depends on transaction B, then A's ancestor count
    // must be greater than B's.  So this is sufficient to validly order the
    // transactions for block inclusion.
    sortedEntries.clear();
    sortedEntries.insert(sortedEntries.begin(), package.begin(), package.end());
    std::sort(sortedEntries.begin(), sortedEntries.end(), CompareTxIterByAncestorCount());
}

// This transaction selection algorithm orders the mempool based
// on feerate of a transaction including all unconfirmed ancestors.
// Since we don't remove transactions from the mempool as we select them
// for block inclusion, we need an alternate method of updating the feerate
// of a transaction with its not-yet-selected ancestors as we go.
// This is accomplished by walking the in-mempool descendants of selected
// transactions and storing a temporary modified state in mapModifiedTxs.
// Each time through the loop, we compare the best transaction in
// mapModifiedTxs with the next transaction in the mempool to decide what
// transaction package to work on next.
void BlockAssembler::addPackageTxs(const CTxMemPool& mempool, int& nPackagesSelected, int& nDescendantsUpdated, uint64_t minGasPrice, CBlock* pblock)
{
    AssertLockHeld(mempool.cs);

    // mapModifiedTx will store sorted packages after they are modified
    // because some of their txs are already in the block
    indexed_modified_transaction_set mapModifiedTx;
    // Keep track of entries that failed inclusion, to avoid duplicate work
    std::set<Txid> failedTx;

    CTxMemPool::indexed_transaction_set::index<ancestor_score_or_gas_price>::type::iterator mi = mempool.mapTx.get<ancestor_score_or_gas_price>().begin();
    CTxMemPool::txiter iter;

    // Limit the number of attempts to add transactions to the block when it is
    // close to full; this is just a simple heuristic to finish quickly if the
    // mempool has a lot of entries.
    const int64_t MAX_CONSECUTIVE_FAILURES = 1000;
    int64_t nConsecutiveFailed = 0;

    while (mi != mempool.mapTx.get<ancestor_score_or_gas_price>().end() || !mapModifiedTx.empty()) {
        if(nTimeLimit != 0 && TicksSinceEpoch<std::chrono::seconds>(NodeClock::now()) >= nTimeLimit){
            //no more time to add transactions, just exit
            return;
        }
        // First try to find a new transaction in mapTx to evaluate.
        //
        // Skip entries in mapTx that are already in a block or are present
        // in mapModifiedTx (which implies that the mapTx ancestor state is
        // stale due to ancestor inclusion in the block)
        // Also skip transactions that we've already failed to add. This can happen if
        // we consider a transaction in mapModifiedTx and it fails: we can then
        // potentially consider it again while walking mapTx.  It's currently
        // guaranteed to fail again, but as a belt-and-suspenders check we put it in
        // failedTx and avoid re-evaluation, since the re-evaluation would be using
        // cached size/sigops/fee values that are not actually correct.
        /** Return true if given transaction from mapTx has already been evaluated,
         * or if the transaction's cached data in mapTx is incorrect. */
        if (mi != mempool.mapTx.get<ancestor_score_or_gas_price>().end()) {
            auto it = mempool.mapTx.project<0>(mi);
            assert(it != mempool.mapTx.end());
            if (mapModifiedTx.count(it) || inBlock.count(it->GetSharedTx()->GetHash()) || failedTx.count(it->GetSharedTx()->GetHash())) {
                ++mi;
                continue;
            }
        }

        // Now that mi is not stale, determine which transaction to evaluate:
        // the next entry from mapTx, or the best from mapModifiedTx?
        bool fUsingModified = false;

        modtxscoreiter modit = mapModifiedTx.get<ancestor_score_or_gas_price>().begin();
        if (mi == mempool.mapTx.get<ancestor_score_or_gas_price>().end()) {
            // We're out of entries in mapTx; use the entry from mapModifiedTx
            iter = modit->iter;
            fUsingModified = true;
        } else {
            // Try to compare the mapTx entry to the mapModifiedTx entry
            iter = mempool.mapTx.project<0>(mi);
            if (modit != mapModifiedTx.get<ancestor_score_or_gas_price>().end() &&
                    CompareModifiedEntry()(*modit, CTxMemPoolModifiedEntry(iter))) {
                // The best entry in mapModifiedTx has higher score
                // than the one from mapTx.
                // Switch which transaction (package) to consider
                iter = modit->iter;
                fUsingModified = true;
            } else {
                // Either no entry in mapModifiedTx, or it's worse than mapTx.
                // Increment mi for the next loop iteration.
                ++mi;
            }
        }

        // We skip mapTx entries that are inBlock, and mapModifiedTx shouldn't
        // contain anything that is inBlock.
        assert(!inBlock.count(iter->GetSharedTx()->GetHash()));

        uint64_t packageSize = iter->GetSizeWithAncestors();
        CAmount packageFees = iter->GetModFeesWithAncestors();
        int64_t packageSigOpsCost = iter->GetSigOpCostWithAncestors();
        if (fUsingModified) {
            packageSize = modit->nSizeWithAncestors;
            packageFees = modit->nModFeesWithAncestors;
            packageSigOpsCost = modit->nSigOpCostWithAncestors;
        }

        if (packageFees < m_options.blockMinFeeRate.GetFee(packageSize)) {
            // Everything else we might consider has a lower fee rate
            return;
        }

        if (!TestPackage(packageSize, packageSigOpsCost)) {
            if (fUsingModified) {
                // Since we always look at the best entry in mapModifiedTx,
                // we must erase failed entries so that we can consider the
                // next best entry on the next loop iteration
                mapModifiedTx.get<ancestor_score_or_gas_price>().erase(modit);
                failedTx.insert(iter->GetSharedTx()->GetHash());
            }

            ++nConsecutiveFailed;

            if (nConsecutiveFailed > MAX_CONSECUTIVE_FAILURES && nBlockWeight >
                    m_options.nBlockMaxWeight - m_options.coinbase_max_additional_weight) {
                // Give up if we're close to full and haven't succeeded in a while
                break;
            }
            continue;
        }

        auto ancestors{mempool.AssumeCalculateMemPoolAncestors(__func__, *iter, CTxMemPool::Limits::NoLimits(), /*fSearchForParents=*/false)};

        onlyUnconfirmed(ancestors);
        ancestors.insert(iter);

        // Test if all tx's are Final
        if (!TestPackageTransactions(ancestors)) {
            if (fUsingModified) {
                mapModifiedTx.get<ancestor_score_or_gas_price>().erase(modit);
                failedTx.insert(iter->GetSharedTx()->GetHash());
            }
            continue;
        }

        // This transaction will make it in; reset the failed counter.
        nConsecutiveFailed = 0;

        // Package can be added. Sort the entries in a valid order.
        std::vector<CTxMemPool::txiter> sortedEntries;
        SortForBlock(ancestors, sortedEntries);

        bool wasAdded=true;
        for (size_t i = 0; i < sortedEntries.size(); ++i) {
            if(!wasAdded || (nTimeLimit != 0 && TicksSinceEpoch<std::chrono::seconds>(NodeClock::now()) >= nTimeLimit))
            {
                //if out of time, or earlier ancestor failed, then skip the rest of the transactions
                mapModifiedTx.erase(sortedEntries[i]);
                wasAdded=false;
                continue;
            }
            const CTransaction& tx = sortedEntries[i]->GetTx();
            if(wasAdded) {
                if (tx.HasCreateOrCall()) {
                    wasAdded = AttemptToAddContractToBlock(sortedEntries[i], minGasPrice, pblock);
                    if(!wasAdded){
                        if(fUsingModified) {
                            //this only needs to be done once to mark the whole package (everything in sortedEntries) as failed
                            mapModifiedTx.get<ancestor_score_or_gas_price>().erase(modit);
                            failedTx.insert(iter->GetSharedTx()->GetHash());
                        }
                    }
                } else {
                    AddToBlock(sortedEntries[i]);
                }
            }
            // Erase from the modified set, if present
            mapModifiedTx.erase(sortedEntries[i]);
        }

        if(!wasAdded){
            //skip UpdatePackages if a transaction failed to be added (match TestPackage logic)
            continue;
        }

        ++nPackagesSelected;

        // Update transactions that depend on each of these
        nDescendantsUpdated += UpdatePackagesForAdded(mempool, ancestors, mapModifiedTx);
    }
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
        type(StakerType::STAKER_NORMAL),
        fAllowWatchOnly(false)
    {
        fAllowWatchOnly = _pwallet->IsWalletFlagSet(wallet::WALLET_FLAG_DISABLE_PRIVATE_KEYS);

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
                LogPrint(BCLog::COINSTAKE, "Fail to add %s to stake allow list\n", strAddress);
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
                LogPrint(BCLog::COINSTAKE, "Fail to add %s to stake exclude list\n", strAddress);
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
        bool mine = pwallet->HasPrivateKey(PKHash(event.item.staker), fAllowWatchOnly);
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
    bool fAllowWatchOnly;
};

class MyDelegations : public DelegationFilterBase
{
public:
    MyDelegations(wallet::CWallet *_pwallet):
        pwallet(_pwallet),
        cacheHeight(0),
        cacheAddressHeight(0),
        fAllowWatchOnly(false)
    {
        fAllowWatchOnly = _pwallet->IsWalletFlagSet(wallet::WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    }

    bool Match(const DelegationEvent& event) const override
    {
        return pwallet->HasPrivateKey(PKHash(event.item.delegate), fAllowWatchOnly);
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
                    if(pwallet->HasPrivateKey(PKHash(address), fAllowWatchOnly))
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
    bool fAllowWatchOnly;
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
    LogPrint(BCLog::COINSTAKE, "CheckStake() : new proof-of-stake block found  \n  hash: %s \nproofhash: %s  \ntarget: %s\n", hashBlock.GetHex(), proofHash.GetHex(), hashTarget.GetHex());
    LogPrint(BCLog::COINSTAKE, "%s\n", pblock->ToString());
    LogPrint(BCLog::COINSTAKE, "out %s\n", FormatMoney(pblock->vtx[1]->GetValueOut()));

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
        LogPrintf("WARN: %s: fail to sign block (%s)\n", __func__, errorMessage);
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
    bool privateKeysDisabled = false;;

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
        if(d->privateKeysDisabled) nBalance += bal.m_watchonly_trusted;
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
            d->pblocktemplate = std::unique_ptr<CBlockTemplate>(BlockAssembler(d->pwallet->chain().chainman().ActiveChainstate(), &(d->pwallet->chain().mempool()), d->pwallet).CreateEmptyBlock(CScript(), true, &d->nTotalFees));
            if (!d->pblocktemplate.get()) {
                d->fError = true;
                return false;
            }
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
            LogPrintf("ThreadStakeMiner(): Valid future PoS block was orphaned before becoming valid\n");
            return false;
        }

        // Try to create an empty PoS block to get the address of the block creator for contracts
        if (!SignBlock(d->pblock, *(d->pwallet), d->nTotalFees, blockTime, d->setCoins, d->mapSolveSelectedCoins[blockTime], d->mapSolveDelegateCoins[blockTime], true, true))
            return false;

        // Create a block that's properly populated with transactions
        d->pblocktemplatefilled = std::unique_ptr<CBlockTemplate>(
                BlockAssembler(d->pwallet->chain().chainman().ActiveChainstate(), &(d->pwallet->chain().mempool()), d->pwallet).CreateNewBlock(d->pblock->vtx[1]->vout[1].scriptPubKey, true, &(d->nTotalFees),
                                                        blockTime, FutureDrift(TicksSinceEpoch<std::chrono::seconds>(NodeClock::now()), d->nHeight, d->consensusParams) - nStakeTimeBuffer));
        if (!d->pblocktemplatefilled.get()) {
            d->fError = true;
            return false;
        }

        if (IsStale(d->pblock)) {
            //another block was received while building ours, scrap progress
            LogPrintf("ThreadStakeMiner(): Valid future PoS block was orphaned before becoming valid\n");
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
                    LogPrintf("ThreadStakeMiner(): Valid future PoS block was orphaned before becoming valid\n");
                    break;
                }
                //check timestamps
                if (d->pblockfilled->GetBlockTime() <= d->pindexPrev->GetBlockTime() ||
                    FutureDrift(d->pblockfilled->GetBlockTime(), d->nHeight, d->consensusParams) < d->pindexPrev->GetBlockTime()) {
                    LogPrintf("ThreadStakeMiner(): Valid PoS block took too long to create and has expired\n");
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
            LogPrintf("ThreadStakeMiner(): Ledger not connected with fingerprint %s\n", d->pwallet->m_ledger_id);
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
