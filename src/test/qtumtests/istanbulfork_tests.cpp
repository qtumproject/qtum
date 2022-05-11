#include <boost/test/unit_test.hpp>
#include <qtumtests/test_utils.h>
#include <script/standard.h>
#include <chainparams.h>

namespace IstanbulTest{

const dev::u256 GASLIMIT = dev::u256(500000);
const dev::h256 HASHTX = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

// Codes used to check that the newer forks up to istanbul are present
const std::vector<valtype> CODE = {
    /*
    // Contract for Istanbul check
    pragma solidity 0.5.13;

    contract IstanbulContract {
        function getChainID() public view returns (uint256 id) {
            assembly {
                id := chainid()
            }
        }
    }
    */    valtype(ParseHex("6080604052348015600f57600080fd5b5060868061001e6000396000f3fe6080604052348015600f57600080fd5b506004361060285760003560e01c8063564b81ef14602d575b600080fd5b60336049565b6040518082815260200191505060405180910390f35b60004690509056fea265627a7a72315820a0b6703e30e0f059af077092e6e0a97a0ae52192355468108d9a24b0ebbe9a7464736f6c634300050d0032")),

    // getChainID()
    valtype(ParseHex("564b81ef"))
};

void genesisLoading(){
    const CChainParams& chainparams = Params();
    int forkHeight = Params().GetConsensus().CoinbaseMaturity(0) + 499;
    dev::eth::ChainParams cp(chainparams.EVMGenesisInfo(forkHeight));
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
BOOST_FIXTURE_TEST_SUITE(istanbulfork_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(checking_istanbul_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), hashTx, dev::Address()));
    executeBC(txs, *m_node.chainman);

    // Call is it istanbul
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txIsItIstanbul;
    txIsItIstanbul.push_back(createQtumTransaction(CODE[1], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    auto result = executeBC(txIsItIstanbul, *m_node.chainman);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(globalSealEngine->chainParams().chainID));
}

BOOST_AUTO_TEST_CASE(checking_istanbul_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), hashTx, dev::Address()));
    executeBC(txs, *m_node.chainman);

    // Call is it istanbul
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txIsItIstanbul;
    txIsItIstanbul.push_back(createQtumTransaction(CODE[1], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    auto result = executeBC(txIsItIstanbul, *m_node.chainman);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) != dev::h256(globalSealEngine->chainParams().chainID));
}

BOOST_AUTO_TEST_SUITE_END()

}
