#include <boost/test/unit_test.hpp>
#include <qtumtests/test_utils.h>
#include <script/standard.h>
#include <chainparams.h>
#include <qtumtests/precompiled_utils.h>
#include <test/qtumtests/data/ecrecover.json.h>
#include <test/qtumtests/data/sha256.json.h>
#include <test/qtumtests/data/ripemd160.json.h>
#include <test/qtumtests/data/identity.json.h>
#include <test/qtumtests/data/modexp.json.h>
#include <test/qtumtests/data/modexp_eip2565.json.h>

namespace LondonTest{

const dev::u256 GASLIMIT = dev::u256(500000);
const dev::h256 HASHTX = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

// Codes used to check that london fork is present
const std::vector<valtype> CODE = {
    /*
    pragma solidity >=0.7.0;

    contract LondonContract {
        address private owner;
        constructor() {
            owner = msg.sender;
        }
        function getBaseFee() public view returns (uint256 fee) {
            assembly {
                fee := basefee()
            }
        }
        function close() public { 
            address payable addr = payable(owner);
            selfdestruct(addr); 
        }
    }
    */
    valtype(ParseHex("608060405234801561001057600080fd5b50336000806101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550610106806100606000396000f3fe6080604052348015600f57600080fd5b506004361060325760003560e01c806315e812ad14603757806343d726d6146051575b600080fd5b603d6059565b6040516048919060ad565b60405180910390f35b60576061565b005b600048905090565b60008060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690508073ffffffffffffffffffffffffffffffffffffffff16ff5b60a78160c6565b82525050565b600060208201905060c0600083018460a0565b92915050565b600081905091905056fea2646970667358221220533ac75fbef0edfb29e840b19d4f3c851ec9e4dbf5a00840d68785006f370fe264736f6c63430008070033")),

    // getBaseFee()
    valtype(ParseHex("15e812ad")),

    // close()
    valtype(ParseHex("43d726d6")),

    // deploy one byte ef
    valtype(ParseHex("60ef60005360016000f3")),

    // deploy two bytes ef00
    valtype(ParseHex("60ef60005360026000f3")),

    // deploy three bytes ef0000
    valtype(ParseHex("60ef60005360036000f3")),

    // deploy 32 bytes ef00...00
    valtype(ParseHex("60ef60005360206000f3")),

    // deploy one byte fe
    valtype(ParseHex("60fe60005360016000f3")),

    // call with first byte ef
    valtype(ParseHex("ef653660")),

    // call with second byte ef
    valtype(ParseHex("60ef6536"))
};

void genesisLoading(){
    const CChainParams& chainparams = Params();
    int coinbaseMaturity = Params().GetConsensus().CoinbaseMaturity(0);
    int forkHeight = coinbaseMaturity + 499;
    dev::eth::EVMConsensus evmConsensus;
    evmConsensus.QIP6Height = coinbaseMaturity;
    evmConsensus.QIP7Height = coinbaseMaturity;
    evmConsensus.nMuirGlacierHeight = coinbaseMaturity;
    evmConsensus.nLondonHeight = forkHeight;
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
BOOST_FIXTURE_TEST_SUITE(londonfork_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(checking_london_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    //------------------------------------
    // Contract tests
    //------------------------------------

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), hashTx, dev::Address()));
    executeBC(txs);

    {
        // Call basefee
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(CODE[1], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon);
        BOOST_CHECK(result.first[0].execRes.output.size() == 32);
        BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0));
        BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    }

    {
        // Call selfdestruct
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(CODE[2], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon);
        BOOST_CHECK(result.first[0].execRes.gasRefunded == 0);
        BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    }

    {
        // Call invalidcode
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(CODE[3], 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(CODE[4], 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(CODE[5], 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(CODE[6], 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(CODE[7], 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(CODE[8], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        txIsItLondon.push_back(createQtumTransaction(CODE[9], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon);
        BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::InvalidCode);
        BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::InvalidCode);
        BOOST_CHECK(result.first[2].execRes.excepted == dev::eth::TransactionException::InvalidCode);
        BOOST_CHECK(result.first[3].execRes.excepted == dev::eth::TransactionException::InvalidCode);
        BOOST_CHECK(result.first[4].execRes.excepted != dev::eth::TransactionException::InvalidCode);
        BOOST_CHECK(result.first[5].execRes.excepted != dev::eth::TransactionException::InvalidCode);
        BOOST_CHECK(result.first[6].execRes.excepted != dev::eth::TransactionException::InvalidCode);
    }

    //------------------------------------
    // Precompiled contract tests
    //------------------------------------

    dev::eth::ChainOperationParams const& params = globalSealEngine->chainParams();
    dev::u256 blockNumber = ChainActive().Tip()->nHeight;

    {
        // Call ecrecover 01
        std::string name = "ecrecover";
        PrecompiledTester tester(name, params, blockNumber);
        std::string jsondata = std::string(json_tests::ecrecover, json_tests::ecrecover + sizeof(json_tests::ecrecover));
        tester.performTests(jsondata);
    }

    {
        // Call sha256 02
        std::string name = "sha256";
        PrecompiledTester tester(name, params, blockNumber);
        std::string jsondata = std::string(json_tests::sha256, json_tests::sha256 + sizeof(json_tests::sha256));
        tester.performTests(jsondata);
    }

    {
        // Call ripemd160 03
        std::string name = "ripemd160";
        PrecompiledTester tester(name, params, blockNumber);
        std::string jsondata = std::string(json_tests::ripemd160, json_tests::ripemd160 + sizeof(json_tests::ripemd160));
        tester.performTests(jsondata);
    }

    {
        // Call identity 04
        std::string name = "identity";
        PrecompiledTester tester(name, params, blockNumber);
        std::string jsondata = std::string(json_tests::identity, json_tests::identity + sizeof(json_tests::identity));
        tester.performTests(jsondata);
    }

    {
        // Call modexp 05
        std::string name = "modexp";
        PrecompiledTester tester(name, params, blockNumber);
        std::string jsondata = std::string(json_tests::modexp_eip2565, json_tests::modexp_eip2565 + sizeof(json_tests::modexp_eip2565));
        tester.performTests(jsondata);
    }
}

BOOST_AUTO_TEST_CASE(checking_london_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    //------------------------------------
    // Contract tests
    //------------------------------------

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), hashTx, dev::Address()));
    executeBC(txs);

    {
        // Call basefee
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(CODE[1], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon);
        BOOST_CHECK(result.first[0].execRes.output.size() == 0);
        BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::BadInstruction);
    }

    {
        // Call selfdestruct
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(CODE[2], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon);
        BOOST_CHECK(result.first[0].execRes.gasRefunded == 24000);
        BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    }

    {
        // Call invalidcode
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(CODE[3], 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(CODE[4], 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(CODE[5], 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(CODE[6], 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(CODE[7], 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(CODE[8], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        txIsItLondon.push_back(createQtumTransaction(CODE[9], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon);
        BOOST_CHECK(result.first[0].execRes.excepted != dev::eth::TransactionException::InvalidCode);
        BOOST_CHECK(result.first[1].execRes.excepted != dev::eth::TransactionException::InvalidCode);
        BOOST_CHECK(result.first[2].execRes.excepted != dev::eth::TransactionException::InvalidCode);
        BOOST_CHECK(result.first[3].execRes.excepted != dev::eth::TransactionException::InvalidCode);
        BOOST_CHECK(result.first[4].execRes.excepted != dev::eth::TransactionException::InvalidCode);
        BOOST_CHECK(result.first[5].execRes.excepted != dev::eth::TransactionException::InvalidCode);
        BOOST_CHECK(result.first[6].execRes.excepted != dev::eth::TransactionException::InvalidCode);
    }

    //------------------------------------
    // Precompiled contract tests
    //------------------------------------

    dev::eth::ChainOperationParams const& params = globalSealEngine->chainParams();
    dev::u256 blockNumber = ChainActive().Tip()->nHeight;

    {
        // Call ecrecover 01
        std::string name = "ecrecover";
        PrecompiledTester tester(name, params, blockNumber);
        std::string jsondata = std::string(json_tests::ecrecover, json_tests::ecrecover + sizeof(json_tests::ecrecover));
        tester.performTests(jsondata);
    }

    {
        // Call sha256 02
        std::string name = "sha256";
        PrecompiledTester tester(name, params, blockNumber);
        std::string jsondata = std::string(json_tests::sha256, json_tests::sha256 + sizeof(json_tests::sha256));
        tester.performTests(jsondata);
    }

    {
        // Call ripemd160 03
        std::string name = "ripemd160";
        PrecompiledTester tester(name, params, blockNumber);
        std::string jsondata = std::string(json_tests::ripemd160, json_tests::ripemd160 + sizeof(json_tests::ripemd160));
        tester.performTests(jsondata);
    }

    {
        // Call identity 04
        std::string name = "identity";
        PrecompiledTester tester(name, params, blockNumber);
        std::string jsondata = std::string(json_tests::identity, json_tests::identity + sizeof(json_tests::identity));
        tester.performTests(jsondata);
    }

    {
        // Call modexp 05
        std::string name = "modexp";
        PrecompiledTester tester(name, params, blockNumber);
        std::string jsondata = std::string(json_tests::modexp, json_tests::modexp + sizeof(json_tests::modexp));
        tester.performTests(jsondata);
    }
}

BOOST_AUTO_TEST_SUITE_END()

}
