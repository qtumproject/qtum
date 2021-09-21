#include <boost/test/unit_test.hpp>
#include <qtumtests/test_utils.h>
#include <script/standard.h>
#include <chainparams.h>

namespace LondonTest{

const dev::u256 GASLIMIT = dev::u256(500000);
const dev::h256 HASHTX = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

// Codes used to check that london fork is present
const std::vector<valtype> CODE = {
    /*
    // Contract for London check
    pragma solidity >=0.7.0;

    contract LondonContract {
        function getBaseFee() public view returns (uint256 fee) {
            assembly {
                fee := basefee()
            }
        }
    }
    */    valtype(ParseHex("608060405234801561001057600080fd5b5060b58061001f6000396000f3fe6080604052348015600f57600080fd5b506004361060285760003560e01c806315e812ad14602d575b600080fd5b60336047565b604051603e9190605c565b60405180910390f35b600048905090565b6056816075565b82525050565b6000602082019050606f6000830184604f565b92915050565b600081905091905056fea264697066735822122041aef153c9034c2310acd56afc522027477c2f23c1c535279623b59c9598c36164736f6c63430008070033")),

    // getChainID()
    valtype(ParseHex("15e812ad"))
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
BOOST_FIXTURE_TEST_SUITE(londonfork_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(checking_london_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), hashTx, dev::Address()));
    executeBC(txs);

    // Call is it london
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txIsItLondon;
    txIsItLondon.push_back(createQtumTransaction(CODE[1], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    auto result = executeBC(txIsItLondon);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0));
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
}

BOOST_AUTO_TEST_CASE(checking_london_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), hashTx, dev::Address()));
    executeBC(txs);

    // Call is it london
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txIsItLondon;
    txIsItLondon.push_back(createQtumTransaction(CODE[1], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    auto result = executeBC(txIsItLondon);
    BOOST_CHECK(result.first[0].execRes.output.size() == 0);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::BadInstruction);
}

BOOST_AUTO_TEST_SUITE_END()

}
