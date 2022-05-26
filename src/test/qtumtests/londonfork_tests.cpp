#include <boost/test/unit_test.hpp>
#include <qtumtests/test_utils.h>
#include <script/standard.h>
#include <chainparams.h>
#include <qtumtests/precompiled_utils.h>
#include <test/qtumtests/data/btc_ecrecover.json.h>
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

        function getExtcodesize(address addr) public view returns(uint32 size){
          assembly {
            size := extcodesize(addr)
            size := extcodesize(addr)
          }
        }
    }
    */
    valtype(ParseHex("608060405234801561001057600080fd5b50336000806101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055506001808190555061033c806100676000396000f3fe608060405234801561001057600080fd5b50600436106100575760003560e01c806315e812ad1461005c57806343d726d61461007a578063458f6cf814610084578063c2722ecc146100b4578063dfa2062e146100be575b600080fd5b6100646100dc565b60405161007191906101e3565b60405180910390f35b6100826100e4565b005b61009e60048036038101906100999190610198565b610123565b6040516100ab91906101fe565b60405180910390f35b6100bc610132565b005b6100c661014c565b6040516100d391906101e3565b60405180910390f35b600048905090565b60008060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690508073ffffffffffffffffffffffffffffffffffffffff16ff5b6000813b9050813b9050919050565b600260018190555060036001819055506004600181905550565b60006001805461015c9190610219565b9050600260015461016d9190610219565b9050600360015461017e9190610219565b905090565b600081359050610192816102ef565b92915050565b6000602082840312156101ae576101ad6102ea565b5b60006101bc84828501610183565b91505092915050565b6101ce816102a1565b82525050565b6101dd816102ab565b82525050565b60006020820190506101f860008301846101c5565b92915050565b600060208201905061021360008301846101d4565b92915050565b6000610224826102a1565b915061022f836102a1565b9250827fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff03821115610264576102636102bb565b5b828201905092915050565b600061027a82610281565b9050919050565b600073ffffffffffffffffffffffffffffffffffffffff82169050919050565b6000819050919050565b600063ffffffff82169050919050565b7f4e487b7100000000000000000000000000000000000000000000000000000000600052601160045260246000fd5b600080fd5b6102f88161026f565b811461030357600080fd5b5056fea26469706673582212201ed939a8031f29326c9c390c8e8d511a03db0533bd32fbfaf0aff0a875cba8cb64736f6c63430008070033")),

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
    valtype(ParseHex("dfa2062e")),

    // getExtcodesize()
    valtype(ParseHex("458f6cf8"))
};

// Codes IDs used to check that london fork is present
enum class CodeID
{
    contract = 0,
    getBaseFee,
    close,
    deploy1ef,
    deploy2ef,
    deploy3ef,
    deploy32ef,
    deploy1fe,
    call1ef,
    call2ef,
    getStore,
    getLoad,
    getExtcodesize
};

// Get the code identified by the ID
valtype getCode(CodeID id)
{
    return CODE[(int)id];
}

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
    txs.push_back(createQtumTransaction(getCode(CodeID::contract), 0, GASLIMIT, dev::u256(1), hashTx, dev::Address()));
    executeBC(txs, *m_node.chainman);

    {
        // Call basefee
        assert(txs.size() > 0);
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::getBaseFee), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon, *m_node.chainman);
        BOOST_CHECK(result.first[0].execRes.output.size() == 32);
        BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0));
        BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    }

    {
        // Call invalidcode
        assert(txs.size() > 0);
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::deploy1ef), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::deploy2ef), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::deploy3ef), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::deploy32ef), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::deploy1fe), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::call1ef), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::call2ef), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon, *m_node.chainman);
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
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::getStore), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::getLoad), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon, *m_node.chainman);
        BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
        BOOST_CHECK(result.first[0].execRes.gasUsed == 26494);
        BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::None);
        BOOST_CHECK(dev::h256(result.first[1].execRes.output) == dev::h256(7));
        BOOST_CHECK(result.first[1].execRes.gasUsed == 24365);
    }

    for(size_t a = 1; a < 300; a++)
    {
        // Call extcodesize two times (cold and warm)
        valtype addr = dev::toBigEndian(dev::u256(a));
        valtype data = getCode(CodeID::getExtcodesize);
        data.insert(data.end(), addr.begin(), addr.end());

        assert(txs.size() > 0);
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(data, 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon, *m_node.chainman);

        uint32_t gasUsed = 0;
        if((a >= 0x1 && a <= 0x9) || a == 0x85)
        {
            gasUsed = 22091;
        }
        else
        {
            gasUsed = 24591;
            if(a > 0x100) gasUsed += 12;
        }

        uint32_t codeSize = 0;
        if((a >= 0x80 && a <= 0x84) || a == 0x86)
        {
            codeSize = a == 0x86 ? 6064 : 12885;
        }

        BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
        BOOST_CHECK(result.first[0].execRes.gasUsed == gasUsed);
        BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(codeSize));
    }

    //------------------------------------
    // Precompiled contract tests
    //------------------------------------

    dev::eth::ChainOperationParams const& params = globalSealEngine->chainParams();
    dev::u256 blockNumber = m_node.chainman->ActiveChain().Tip()->nHeight;

    // Call btc_ecrecover 0x85
    RunPrecompiledTests(btc_ecrecover, btc_ecrecover, params, blockNumber);

    // Call ecrecover 0x1
    RunPrecompiledTests(ecrecover, ecrecover, params, blockNumber);

    // Call sha256 0x2
    RunPrecompiledTests(sha256, sha256, params, blockNumber);

    // Call ripemd160 0x3
    RunPrecompiledTests(ripemd160, ripemd160, params, blockNumber);

    // Call identity 0x4
    RunPrecompiledTests(identity, identity, params, blockNumber);

    // Call modexp 0x5
    RunPrecompiledTests(modexp, modexp_eip2565, params, blockNumber);

    // Call alt_bn128_G1_add 0x6
    RunPrecompiledTests(alt_bn128_G1_add, alt_bn128_G1_add, params, blockNumber);

    // Call alt_bn128_G1_mul 0x7
    RunPrecompiledTests(alt_bn128_G1_mul, alt_bn128_G1_mul, params, blockNumber);

    // Call alt_bn128_pairing_product 0x8
    RunPrecompiledTests(alt_bn128_pairing_product, alt_bn128_pairing_product, params, blockNumber);

    // Call blake2_compression 0x9
    RunPrecompiledTests(blake2_compression, blake2_compression, params, blockNumber);

    //------------------------------------
    // Close contract test
    //------------------------------------

    {
        // Call selfdestruct
        assert(txs.size() > 0);
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::close), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon, *m_node.chainman);
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
    txs.push_back(createQtumTransaction(getCode(CodeID::contract), 0, GASLIMIT, dev::u256(1), hashTx, dev::Address()));
    executeBC(txs, *m_node.chainman);

    {
        // Call basefee
        assert(txs.size() > 0);
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::getBaseFee), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon, *m_node.chainman);
        BOOST_CHECK(result.first[0].execRes.output.size() == 0);
        BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::BadInstruction);
    }

    {
        // Call invalidcode
        assert(txs.size() > 0);
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::deploy1ef), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::deploy2ef), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::deploy3ef), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::deploy32ef), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::deploy1fe), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::call1ef), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::call2ef), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon, *m_node.chainman);
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
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::getStore), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::getLoad), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon, *m_node.chainman);
        BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
        BOOST_CHECK(result.first[0].execRes.gasUsed == 27894);
        BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::None);
        BOOST_CHECK(dev::h256(result.first[1].execRes.output) == dev::h256(7));
        BOOST_CHECK(result.first[1].execRes.gasUsed == 24465);
    }

    for(size_t a = 1; a < 300; a++)
    {
        // Call extcodesize two times
        valtype addr = dev::toBigEndian(dev::u256(a));
        valtype data = getCode(CodeID::getExtcodesize);
        data.insert(data.end(), addr.begin(), addr.end());

        assert(txs.size() > 0);
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(data, 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon, *m_node.chainman);

        uint32_t gasUsed = 23291;
        if(a > 0x100) gasUsed += 12;

        uint32_t codeSize = 0;
        if((a >= 0x80 && a <= 0x84) || a == 0x86)
        {
            codeSize = a == 0x86 ? 6064 : 12885;
        }

        BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
        BOOST_CHECK(result.first[0].execRes.gasUsed == gasUsed);
        BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(codeSize));
    }

    //------------------------------------
    // Precompiled contract tests
    //------------------------------------

    dev::eth::ChainOperationParams const& params = globalSealEngine->chainParams();
    dev::u256 blockNumber = m_node.chainman->ActiveChain().Tip()->nHeight;

    // Call btc_ecrecover 0x85
    RunPrecompiledTests(btc_ecrecover, btc_ecrecover, params, blockNumber);

    // Call ecrecover 0x1
    RunPrecompiledTests(ecrecover, ecrecover, params, blockNumber);

    // Call sha256 0x2
    RunPrecompiledTests(sha256, sha256, params, blockNumber);

    // Call ripemd160 0x3
    RunPrecompiledTests(ripemd160, ripemd160, params, blockNumber);

    // Call identity 0x4
    RunPrecompiledTests(identity, identity, params, blockNumber);

    // Call modexp 0x5
    RunPrecompiledTests(modexp, modexp, params, blockNumber);

    // Call alt_bn128_G1_add 0x6
    RunPrecompiledTests(alt_bn128_G1_add, alt_bn128_G1_add, params, blockNumber);

    // Call alt_bn128_G1_mul 0x7
    RunPrecompiledTests(alt_bn128_G1_mul, alt_bn128_G1_mul, params, blockNumber);

    // Call alt_bn128_pairing_product 0x8
    RunPrecompiledTests(alt_bn128_pairing_product, alt_bn128_pairing_product, params, blockNumber);

    // Call blake2_compression 0x9
    RunPrecompiledTests(blake2_compression, blake2_compression, params, blockNumber);

    //------------------------------------
    // Close contract test
    //------------------------------------

    {
        // Call selfdestruct
        assert(txs.size() > 0);
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txIsItLondon;
        txIsItLondon.push_back(createQtumTransaction(getCode(CodeID::close), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txIsItLondon, *m_node.chainman);
        BOOST_CHECK(result.first[0].execRes.gasRefunded == 24000);
        BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
        txs.clear();
    }
}

BOOST_AUTO_TEST_SUITE_END()

}
