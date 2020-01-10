// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <crypto/sha256.h>
#include <test/util.h>
#include <txmempool.h>
#include <validation.h>
#include <util/convert.h>

#include <list>
#include <vector>

static void AssembleBlock(benchmark::State& state)
{
    const std::vector<unsigned char> op_true{OP_TRUE};
    CScriptWitness witness;
    witness.stack.push_back(op_true);

    uint256 witness_program;
    CSHA256().Write(&op_true[0], op_true.size()).Finalize(witness_program.begin());

    const CScript SCRIPT_PUB{CScript(OP_0) << std::vector<unsigned char>{witness_program.begin(), witness_program.end()}};

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
    // Collect some loose transactions that spend the coinbases of our mined blocks
    constexpr size_t NUM_BLOCKS{600};
    std::array<CTransactionRef, NUM_BLOCKS - COINBASE_MATURITY + 1> txs;
    for (size_t b{0}; b < NUM_BLOCKS; ++b) {
        CMutableTransaction tx;
        tx.vin.push_back(MineBlock(SCRIPT_PUB));
        tx.vin.back().scriptWitness = witness;
        tx.vout.emplace_back(1337, SCRIPT_PUB);
        if (NUM_BLOCKS - b >= COINBASE_MATURITY)
            txs.at(b) = MakeTransactionRef(tx);
    }
    {
        LOCK(::cs_main); // Required for ::AcceptToMemoryPool.

        for (const auto& txr : txs) {
            CValidationState state;
            bool ret{::AcceptToMemoryPool(::mempool, state, txr, nullptr /* pfMissingInputs */, nullptr /* plTxnReplaced */, false /* bypass_limits */, /* nAbsurdFee */ 0)};
            assert(ret);
        }
    }

    while (state.KeepRunning()) {
        PrepareBlock(SCRIPT_PUB);
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

BENCHMARK(AssembleBlock, 700);
