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
#include <test/qtumtests/data/alt_bn128_G1_add.json.h>
#include <test/qtumtests/data/alt_bn128_G1_mul.json.h>
#include <test/qtumtests/data/alt_bn128_pairing_product.json.h>
#include <test/qtumtests/data/blake2_compression.json.h>

namespace LondonTest{

const dev::u256 GASLIMIT = dev::u256(500000);
const dev::h256 HASHTX = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

// Codes used to check that london fork is present
const std::vector<valtype> CODE = {
    /*
    pragma solidity >=0.7.0;

    contract LondonContract {
        address private owner;
        uint256 num_store;
        constructor() {
            owner = msg.sender;
            num_store = 1;
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
        function getStore() public {
            num_store = 2;
            num_store = 3;
            num_store = 4;
        }
        function getLoad() public view returns (uint256 num_load) {
            num_load = num_store + 1;
            num_load = num_store + 2;
            num_load = num_store + 3;
        }
    }
    */
    valtype(ParseHex("608060405234801561001057600080fd5b50336000806101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555060018081905550610228806100676000396000f3fe608060405234801561001057600080fd5b506004361061004c5760003560e01c806315e812ad1461005157806343d726d61461006f578063c2722ecc14610079578063dfa2062e14610083575b600080fd5b6100596100a1565b6040516100669190610148565b60405180910390f35b6100776100a9565b005b6100816100e8565b005b61008b610102565b6040516100989190610148565b60405180910390f35b600048905090565b60008060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690508073ffffffffffffffffffffffffffffffffffffffff16ff5b600260018190555060036001819055506004600181905550565b6000600180546101129190610163565b905060026001546101239190610163565b905060036001546101349190610163565b905090565b610142816101b9565b82525050565b600060208201905061015d6000830184610139565b92915050565b600061016e826101b9565b9150610179836101b9565b9250827fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff038211156101ae576101ad6101c3565b5b828201905092915050565b6000819050919050565b7f4e487b7100000000000000000000000000000000000000000000000000000000600052601160045260246000fdfea264697066735822122080d6ca6e9f654afc37c68f04ea441822e177844deebcfea7dbbd3012169a2ca164736f6c63430008070033")),

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
    valtype(ParseHex("60ef6536")),

    // getStore()
    valtype(ParseHex("c2722ecc")),

    // getLoad()
    valtype(ParseHex("dfa2062e"))
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
        assert(txs.size() > 0);
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(CODE[1], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon);
        BOOST_CHECK(result.first[0].execRes.output.size() == 32);
        BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0));
        BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    }

    {
        // Call invalidcode
        assert(txs.size() > 0);
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

    {
        // Call store and load three times (cold, warm and warm)
        assert(txs.size() > 0);
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(CODE[10], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        txIsItLondon.push_back(createQtumTransaction(CODE[11], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon);
        BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
        BOOST_CHECK(result.first[0].execRes.gasUsed == 26472);
        BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::None);
        BOOST_CHECK(dev::h256(result.first[1].execRes.output) == dev::h256(7));
        BOOST_CHECK(result.first[1].execRes.gasUsed == 24343);
    }

    //------------------------------------
    // Precompiled contract tests
    //------------------------------------

    dev::eth::ChainOperationParams const& params = globalSealEngine->chainParams();
    dev::u256 blockNumber = ChainActive().Tip()->nHeight;

    // Call ecrecover 01
    RunPrecompiledTests(ecrecover, ecrecover, params, blockNumber);

    // Call sha256 02
    RunPrecompiledTests(sha256, sha256, params, blockNumber);

    // Call ripemd160 03
    RunPrecompiledTests(ripemd160, ripemd160, params, blockNumber);

    // Call identity 04
    RunPrecompiledTests(identity, identity, params, blockNumber);

    // Call modexp 05
    RunPrecompiledTests(modexp, modexp_eip2565, params, blockNumber);

    // Call alt_bn128_G1_add 06
    RunPrecompiledTests(alt_bn128_G1_add, alt_bn128_G1_add, params, blockNumber);

    // Call alt_bn128_G1_mul 07
    RunPrecompiledTests(alt_bn128_G1_mul, alt_bn128_G1_mul, params, blockNumber);

    // Call alt_bn128_pairing_product 08
    RunPrecompiledTests(alt_bn128_pairing_product, alt_bn128_pairing_product, params, blockNumber);

    // Call blake2_compression 09
    RunPrecompiledTests(blake2_compression, blake2_compression, params, blockNumber);

    //------------------------------------
    // Close contract test
    //------------------------------------

    {
        // Call selfdestruct
        assert(txs.size() > 0);
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(CODE[2], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon);
        BOOST_CHECK(result.first[0].execRes.gasRefunded == 0);
        BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
        txs.clear();
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
        assert(txs.size() > 0);
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(CODE[1], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon);
        BOOST_CHECK(result.first[0].execRes.output.size() == 0);
        BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::BadInstruction);
    }

    {
        // Call invalidcode
        assert(txs.size() > 0);
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

    {
        // Call store and load three times
        assert(txs.size() > 0);
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(CODE[10], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        txIsItLondon.push_back(createQtumTransaction(CODE[11], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon);
        BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
        BOOST_CHECK(result.first[0].execRes.gasUsed == 27872);
        BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::None);
        BOOST_CHECK(dev::h256(result.first[1].execRes.output) == dev::h256(7));
        BOOST_CHECK(result.first[1].execRes.gasUsed == 24443);
    }

    //------------------------------------
    // Precompiled contract tests
    //------------------------------------

    dev::eth::ChainOperationParams const& params = globalSealEngine->chainParams();
    dev::u256 blockNumber = ChainActive().Tip()->nHeight;

    // Call ecrecover 01
    RunPrecompiledTests(ecrecover, ecrecover, params, blockNumber);

    // Call sha256 02
    RunPrecompiledTests(sha256, sha256, params, blockNumber);

    // Call ripemd160 03
    RunPrecompiledTests(ripemd160, ripemd160, params, blockNumber);

    // Call identity 04
    RunPrecompiledTests(identity, identity, params, blockNumber);

    // Call modexp 05
    RunPrecompiledTests(modexp, modexp, params, blockNumber);

    // Call alt_bn128_G1_add 06
    RunPrecompiledTests(alt_bn128_G1_add, alt_bn128_G1_add, params, blockNumber);

    // Call alt_bn128_G1_mul 07
    RunPrecompiledTests(alt_bn128_G1_mul, alt_bn128_G1_mul, params, blockNumber);

    // Call alt_bn128_pairing_product 08
    RunPrecompiledTests(alt_bn128_pairing_product, alt_bn128_pairing_product, params, blockNumber);

    // Call blake2_compression 09
    RunPrecompiledTests(blake2_compression, blake2_compression, params, blockNumber);

    //------------------------------------
    // Close contract test
    //------------------------------------

    {
        // Call selfdestruct
        assert(txs.size() > 0);
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(CODE[2], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon);
        BOOST_CHECK(result.first[0].execRes.gasRefunded == 24000);
        BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
        txs.clear();
    }
}

BOOST_AUTO_TEST_SUITE_END()

}
