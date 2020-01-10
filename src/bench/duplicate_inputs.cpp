// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <chainparams.h>
#include <coins.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <pow.h>
#include <txmempool.h>
#include <validation.h>
#include <util/convert.h>

#include <list>
#include <vector>


static void DuplicateInputs(benchmark::State& state)
{
    const CScript SCRIPT_PUB{CScript(OP_TRUE)};

    const CChainParams& chainparams = Params();
    {
        LOCK(cs_main);
        ::pstorageresult.reset();
        ::globalState.reset();
        ::globalSealEngine.reset();

        ::fRequireStandard=false;
        fs::path qtumStateDir = GetDataDir() / "stateQtum";
        bool fStatus = fs::exists(qtumStateDir);
        const std::string dirQtum(qtumStateDir.string());
        const dev::h256 hashDB(dev::sha3(dev::rlp("")));
        dev::eth::BaseState existsQtumstate = fStatus ? dev::eth::BaseState::PreExisting : dev::eth::BaseState::Empty;
        ::globalState = std::unique_ptr<QtumState>(new QtumState(dev::u256(0), QtumState::openDB(dirQtum, hashDB, dev::WithExisting::Trust), dirQtum, existsQtumstate));
        dev::eth::ChainParams cp((chainparams.EVMGenesisInfo(dev::eth::Network::qtumMainNetwork)));
        ::globalSealEngine = std::unique_ptr<dev::eth::SealEngineFace>(cp.createSealEngine());

        ::pstorageresult.reset(new StorageResults(qtumStateDir.string()));

        if(::ChainActive().Tip() != nullptr){
            ::globalState->setRoot(uintToh256(::ChainActive().Tip()->hashStateRoot));
            ::globalState->setRootUTXO(uintToh256(::ChainActive().Tip()->hashUTXORoot));
        } else {
            ::globalState->setRoot(dev::sha3(dev::rlp("")));
            ::globalState->setRootUTXO(uintToh256(chainparams.GenesisBlock().hashUTXORoot));
            ::globalState->populateFrom(cp.genesisState);
        }
        ::globalState->db().commit();
        ::globalState->dbUtxo().commit();
    }
    CBlock block{};
    CMutableTransaction coinbaseTx{};
    CMutableTransaction naughtyTx{};

    LOCK(cs_main);
    CBlockIndex* pindexPrev = ::ChainActive().Tip();
    assert(pindexPrev != nullptr);
    block.nBits = GetNextWorkRequired(pindexPrev, &block, chainparams.GetConsensus());
    block.nNonce = 0;
    auto nHeight = pindexPrev->nHeight + 1;

    // Make a coinbase TX
    coinbaseTx.vin.resize(1);
    coinbaseTx.vin[0].prevout.SetNull();
    coinbaseTx.vout.resize(1);
    coinbaseTx.vout[0].scriptPubKey = SCRIPT_PUB;
    coinbaseTx.vout[0].nValue = GetBlockSubsidy(nHeight, chainparams.GetConsensus());
    coinbaseTx.vin[0].scriptSig = CScript() << nHeight << OP_0;


    naughtyTx.vout.resize(1);
    naughtyTx.vout[0].nValue = 0;
    naughtyTx.vout[0].scriptPubKey = SCRIPT_PUB;

    uint64_t n_inputs = ((std::min((uint64_t)(dgpMaxBlockSerSize / WITNESS_SCALE_FACTOR), (uint64_t)MAX_TRANSACTION_BASE_SIZE) - (CTransaction(coinbaseTx).GetTotalSize() + CTransaction(naughtyTx).GetTotalSize())) / 41) - 100;
    for (uint64_t x = 0; x < (n_inputs - 1); ++x) {
        naughtyTx.vin.emplace_back(GetRandHash(), 0, CScript(), 0);
    }
    naughtyTx.vin.emplace_back(naughtyTx.vin.back());

    block.vtx.push_back(MakeTransactionRef(std::move(coinbaseTx)));
    block.vtx.push_back(MakeTransactionRef(std::move(naughtyTx)));

    block.hashMerkleRoot = BlockMerkleRoot(block);

    while (state.KeepRunning()) {
        CValidationState cvstate{};
        assert(!CheckBlock(block, cvstate, chainparams.GetConsensus(), false, false));
        assert(cvstate.GetRejectReason() == "bad-txns-inputs-duplicate");
    }

    if (g_chainstate && g_chainstate->CanFlushToDisk()) {
        g_chainstate->ForceFlushStateToDisk();
        g_chainstate->ResetCoinsViews();
    }
    pblocktree.reset();
    pstorageresult.reset();
    globalState.reset();
    globalSealEngine.reset();
}

BENCHMARK(DuplicateInputs, 10);
