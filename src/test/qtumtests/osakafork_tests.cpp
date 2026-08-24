#include <boost/test/unit_test.hpp>
#include <test/qtumtests/test_utils.h>
#include <qtum/qtumutils.h>
#include <chainparams.h>
#include <test/qtumtests/precompiled_utils.h>
#include <test/qtumtests/data/modexp_eip7883.json.h>

namespace OsakaTest{

void genesisLoading(){
    const CChainParams& chainparams = Params();
    int coinbaseMaturity = Params().GetConsensus().CoinbaseMaturity(0);
    int forkHeight = coinbaseMaturity + 499;
    dev::eth::EVMConsensus evmConsensus;
    evmConsensus.QIP6Height = coinbaseMaturity;
    evmConsensus.QIP7Height = coinbaseMaturity;
    evmConsensus.nMuirGlacierHeight = coinbaseMaturity;
    evmConsensus.nLondonHeight = coinbaseMaturity;
    evmConsensus.nShanghaiHeight = coinbaseMaturity;
    evmConsensus.nCancunHeight = coinbaseMaturity;
    evmConsensus.nPectraHeight = coinbaseMaturity;
    evmConsensus.nOsakaHeight = forkHeight;
    UpdateOsakaHeight(forkHeight);
    dev::eth::ChainParams cp(chainparams.EVMGenesisInfo(evmConsensus));
    globalState->populateFrom(cp.genesisState);
    globalSealEngine = std::unique_ptr<dev::eth::SealEngineFace>(cp.createSealEngine());
    globalState->db().commit();
}

void createNewBlocks(TestChain100Setup* testChain100Setup, size_t n){
    std::function<void(size_t n)> generateBlocks = [&](size_t n){
        dev::h256 oldHashStateRoot = globalState->rootHash();
        dev::h256 oldHashUTXORoot = globalState->rootHashUTXO();
        for(size_t i = 0; i < n; i++){
            testChain100Setup->CreateAndProcessBlock({}, GetScriptForRawPubKey(testChain100Setup->coinbaseKey.GetPubKey()));
        }
        globalState->setRoot(oldHashStateRoot);
        globalState->setRootUTXO(oldHashUTXORoot);
    };

    generateBlocks(n);
}
BOOST_FIXTURE_TEST_SUITE(osakafork_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(checking_modexp_after_fork){
    genesisLoading();
    createNewBlocks(this, 500);

    dev::eth::ChainOperationParams const& params = globalSealEngine->chainParams();
    dev::u256 blockNumber = 0;
    {
        LOCK(::cs_main);
        blockNumber = m_node.chainman->ActiveChain().Tip()->nHeight;
    }

    // Call modexp 0x5
    RunNewPrecompiledTests(modexp, modexp_eip7883, params, blockNumber);
}

BOOST_AUTO_TEST_CASE(checking_modexp_before_fork){
    genesisLoading();
    createNewBlocks(this, 499);

    dev::eth::ChainOperationParams const& params = globalSealEngine->chainParams();
    dev::u256 blockNumber = 0;
    {
        LOCK(::cs_main);
        blockNumber = m_node.chainman->ActiveChain().Tip()->nHeight;
    }

    // Call modexp 0x5
    RunOldPrecompiledTests(modexp, modexp_eip7883, params, blockNumber);
}

BOOST_AUTO_TEST_SUITE_END()

}
