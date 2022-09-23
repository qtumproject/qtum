// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/chainstate.h>

#include <consensus/params.h>
#include <node/blockstorage.h>
#include <validation.h>
#include <chainparams.h>

namespace node {
std::optional<ChainstateLoadingError> LoadChainstate(bool fReset,
                                                     ChainstateManager& chainman,
                                                     CTxMemPool* mempool,
                                                     bool fPruneMode,
                                                     const Consensus::Params& consensus_params,
                                                     bool fReindexChainState,
                                                     int64_t nBlockTreeDBCache,
                                                     int64_t nCoinDBCache,
                                                     int64_t nCoinCacheUsage,
                                                     bool block_tree_db_in_memory,
                                                     bool coins_db_in_memory,
                                                     const ArgsManager& args,
                                                     std::function<bool()> shutdown_requested,
                                                     std::function<void()> coins_error_cb)
{
    auto is_coinsview_empty = [&](CChainState* chainstate) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        return fReset || fReindexChainState || chainstate->CoinsTip().GetBestBlock().IsNull();
    };

    LOCK(cs_main);
    chainman.Reset();
    chainman.InitializeChainstate(mempool);
    chainman.m_total_coinstip_cache = nCoinCacheUsage;
    chainman.m_total_coinsdb_cache = nCoinDBCache;

    UnloadBlockIndex(mempool, chainman);

    auto& pblocktree{chainman.m_blockman.m_block_tree_db};
    // new CBlockTreeDB tries to delete the existing file, which
    // fails if it's still open from the previous loop. Close it first:
    pblocktree.reset();
    pstorageresult.reset();
    globalState.reset();
    globalSealEngine.reset();
    pblocktree.reset(new CBlockTreeDB(nBlockTreeDBCache, block_tree_db_in_memory, fReset));

    if (fReset) {
        pblocktree->WriteReindexing(true);
        //If we're reindexing in prune mode, wipe away unusable block files and all undo data files
        if (fPruneMode)
            CleanupBlockRevFiles();
    }

    if (shutdown_requested && shutdown_requested()) return ChainstateLoadingError::SHUTDOWN_PROBED;

    // LoadBlockIndex will load fHavePruned if we've ever removed a
    // block file from disk.
    // Note that it also sets fReindex based on the disk flag!
    // From here on out fReindex and fReset mean something different!
    if (!chainman.LoadBlockIndex()) {
        if (shutdown_requested && shutdown_requested()) return ChainstateLoadingError::SHUTDOWN_PROBED;
        return ChainstateLoadingError::ERROR_LOADING_BLOCK_DB;
    }

    if (!chainman.BlockIndex().empty() &&
            !chainman.m_blockman.LookupBlockIndex(consensus_params.hashGenesisBlock)) {
        return ChainstateLoadingError::ERROR_BAD_GENESIS_BLOCK;
    }

    // Check for changed -prune state.  What we are concerned about is a user who has pruned blocks
    // in the past, but is now trying to run unpruned.
    if (fHavePruned && !fPruneMode) {
        return ChainstateLoadingError::ERROR_PRUNED_NEEDS_REINDEX;
    }

    // At this point blocktree args are consistent with what's on disk.
    // If we're not mid-reindex (based on disk + args), add a genesis block on disk
    // (otherwise we use the one already on disk).
    // This is called again in ThreadImport after the reindex completes.
    if (!fReindex && !chainman.ActiveChainstate().LoadGenesisBlock()) {
        return ChainstateLoadingError::ERROR_LOAD_GENESIS_BLOCK_FAILED;
    }

    // At this point we're either in reindex or we've loaded a useful
    // block tree into BlockIndex()!

    for (CChainState* chainstate : chainman.GetAll()) {
        chainstate->InitCoinsDB(
            /* cache_size_bytes */ nCoinDBCache,
            /* in_memory */ coins_db_in_memory,
            /* should_wipe */ fReset || fReindexChainState);

        if (coins_error_cb) {
            chainstate->CoinsErrorCatcher().AddReadErrCallback(coins_error_cb);
        }

        // If necessary, upgrade from older database format.
        // This is a no-op if we cleared the coinsviewdb with -reindex or -reindex-chainstate
        if (!chainstate->CoinsDB().Upgrade()) {
            return ChainstateLoadingError::ERROR_CHAINSTATE_UPGRADE_FAILED;
        }

        // ReplayBlocks is a no-op if we cleared the coinsviewdb with -reindex or -reindex-chainstate
        if (!chainstate->ReplayBlocks()) {
            return ChainstateLoadingError::ERROR_REPLAYBLOCKS_FAILED;
        }

        // The on-disk coinsdb is now in a good state, create the cache
        chainstate->InitCoinsCache(nCoinCacheUsage);
        assert(chainstate->CanFlushToDisk());

        if (!is_coinsview_empty(chainstate)) {
            // LoadChainTip initializes the chain based on CoinsTip()'s best block
            if (!chainstate->LoadChainTip()) {
                return ChainstateLoadingError::ERROR_LOADCHAINTIP_FAILED;
            }
            assert(chainstate->m_chain.Tip() != nullptr);
        }
    }

    /////////////////////////////////////////////////////////// qtum
    if((args.IsArgSet("-dgpstorage") && args.IsArgSet("-dgpevm")) || (!args.IsArgSet("-dgpstorage") && args.IsArgSet("-dgpevm")) ||
      (!args.IsArgSet("-dgpstorage") && !args.IsArgSet("-dgpevm"))){
        fGettingValuesDGP = true;
    } else {
        fGettingValuesDGP = false;
    }

    dev::eth::NoProof::init();
    fs::path qtumStateDir = gArgs.GetDataDirNet() / "stateQtum";
    bool fStatus = fs::exists(qtumStateDir);
    const std::string dirQtum = PathToString(qtumStateDir);
    const dev::h256 hashDB(dev::sha3(dev::rlp("")));
    dev::eth::BaseState existsQtumstate = fStatus ? dev::eth::BaseState::PreExisting : dev::eth::BaseState::Empty;
    globalState = std::unique_ptr<QtumState>(new QtumState(dev::u256(0), QtumState::openDB(dirQtum, hashDB, dev::WithExisting::Trust), dirQtum, existsQtumstate));
    const CChainParams& chainparams = Params();
    dev::eth::ChainParams cp(chainparams.EVMGenesisInfo());
    globalSealEngine = std::unique_ptr<dev::eth::SealEngineFace>(cp.createSealEngine());

    pstorageresult.reset(new StorageResults(PathToString(qtumStateDir)));
    if (fReset) {
        pstorageresult->wipeResults();
    }

    {
        LOCK(cs_main);
        CChain& active_chain = chainman.ActiveChain();
        if(active_chain.Tip() != nullptr){
        globalState->setRoot(uintToh256(active_chain.Tip()->hashStateRoot));
        globalState->setRootUTXO(uintToh256(active_chain.Tip()->hashUTXORoot));
        } else {
            globalState->setRoot(dev::sha3(dev::rlp("")));
            globalState->setRootUTXO(uintToh256(chainparams.GenesisBlock().hashUTXORoot));
            globalState->populateFrom(cp.genesisState);
        }
        globalState->db().commit();
        globalState->dbUtxo().commit();
    }

    fRecordLogOpcodes = args.IsArgSet("-record-log-opcodes");
    fIsVMlogFile = fs::exists(gArgs.GetDataDirNet() / "vmExecLogs.json");
    ///////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////// // qtum
    if (fAddressIndex != args.GetBoolArg("-addrindex", DEFAULT_ADDRINDEX)) {
        return ChainstateLoadingError::ERROR_ADDRINDEX_NEEDS_REINDEX;
    }
    ///////////////////////////////////////////////////////////////
    // Check for changed -logevents state
    if (fLogEvents != args.GetBoolArg("-logevents", DEFAULT_LOGEVENTS) && !fLogEvents) {
        return ChainstateLoadingError::ERROR_LOGEVENTS_NEEDS_REINDEX;
    }

    if (!args.GetBoolArg("-logevents", DEFAULT_LOGEVENTS))
    {
        pstorageresult->wipeResults();
        pblocktree->WipeHeightIndex();
        fLogEvents = false;
        pblocktree->WriteFlag("logevents", fLogEvents);
    }

    if (!fReset) {
        auto chainstates{chainman.GetAll()};
        if (std::any_of(chainstates.begin(), chainstates.end(),
                        [](const CChainState* cs) EXCLUSIVE_LOCKS_REQUIRED(cs_main) { return cs->NeedsRedownload(); })) {
            return ChainstateLoadingError::ERROR_BLOCKS_WITNESS_INSUFFICIENTLY_VALIDATED;
        }
    }

    return std::nullopt;
}

std::optional<ChainstateLoadVerifyError> VerifyLoadedChainstate(ChainstateManager& chainman,
                                                                bool fReset,
                                                                bool fReindexChainState,
                                                                const Consensus::Params& consensus_params,
                                                                int check_blocks,
                                                                int check_level,
                                                                std::function<int64_t()> get_unix_time_seconds)
{
    auto is_coinsview_empty = [&](CChainState* chainstate) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        return fReset || fReindexChainState || chainstate->CoinsTip().GetBestBlock().IsNull();
    };

    LOCK(cs_main);

    CChain& active_chain = chainman.ActiveChain();
    QtumDGP qtumDGP(globalState.get(), chainman.ActiveChainstate(), fGettingValuesDGP);
    globalSealEngine->setQtumSchedule(qtumDGP.getGasSchedule(active_chain.Height() + (active_chain.Height()+1 >= consensus_params.QIP7Height ? 0 : 1) ));

    for (CChainState* chainstate : chainman.GetAll()) {
        if (!is_coinsview_empty(chainstate)) {
            const CBlockIndex* tip = chainstate->m_chain.Tip();
            if (tip && tip->nTime > get_unix_time_seconds() + MAX_FUTURE_BLOCK_TIME) {
                return ChainstateLoadVerifyError::ERROR_BLOCK_FROM_FUTURE;
            }

            if (!CVerifyDB().VerifyDB(
                    *chainstate, consensus_params, chainstate->CoinsDB(),
                    check_level,
                    check_blocks)) {
                return ChainstateLoadVerifyError::ERROR_CORRUPTED_BLOCK_DB;
            }
        }
    }

    return std::nullopt;
}
} // namespace node
