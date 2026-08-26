#include <boost/test/unit_test.hpp>
#include <test/qtumtests/test_utils.h>
#include <qtum/qtumutils.h>
#include <chainparams.h>
#include <test/qtumtests/precompiled_utils.h>
#include <test/qtumtests/data/modexp_eip7883.json.h>

namespace OsakaTest{

const dev::u256 GASLIMIT = dev::u256(500000);
const dev::h256 HASHTX = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

std::vector<unsigned char> concat(
    const std::vector<unsigned char>& a,
    const std::vector<unsigned char>& b)
{
    std::vector<unsigned char> result;
    result.reserve(a.size() + b.size());
    result.insert(result.end(), a.begin(), a.end());
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

// Codes used to check that osaka fork
const std::vector<valtype> CODE = {
    // EIP-7939
    /*
    // SPDX-License-Identifier: MIT
    pragma solidity ^0.8.25;

    contract LeadingZerosChecks {
        /// @notice Returns the number of leading zeros in a 256-bit word
        function leadingZeros(uint256 x) external pure returns (uint256 result) {
            assembly {
                // CLZ opcode: 0x1e
                result := clz(x)
            }
        }
    }
    */
  valtype(ParseHex("6080604052348015600e575f5ffd5b506101108061001c5f395ff3fe6080604052348015600e575f5ffd5b50600436106026575f3560e01c80638d9d94e214602a575b5f5ffd5b60406004803603810190603c91906090565b6054565b604051604b919060c3565b60405180910390f35b5f811e9050919050565b5f5ffd5b5f819050919050565b6072816062565b8114607b575f5ffd5b50565b5f81359050608a81606b565b92915050565b5f6020828403121560a25760a1605e565b5b5f60ad84828501607e565b91505092915050565b60bd816062565b82525050565b5f60208201905060d45f83018460b6565b9291505056fea2646970667358221220eafa86cd369b48c6442f80518f19aa92aaee659d17626949285e0eaa637cfed264736f6c63430008220033")),
    // leadingZeros()
    valtype(ParseHex("8d9d94e2")),
    // EIP-7951
    /*
    // SPDX-License-Identifier: MIT
    pragma solidity ^0.8.25;

    contract Secp256r1Verify {
        /// @notice Verify a secp256r1 signature
        /// @param data Encoded input
        /// @return valid True if signature is valid, false otherwise
        function p256verify(bytes calldata data) public view returns (bool valid) {
            (bool success, bytes memory out) = address(0x100).staticcall(data);

            // The precompile doesn't revert under any circumstances
            require(success, "Precompile problem");

            // Invalid inputs or verification failures return empty output
            if (out.length == 32) {
                uint256 ret = abi.decode(out, (uint256));
                valid = (ret == 1);
            } else {
                valid = false;
            }
        }
    }
    */
    valtype(ParseHex("6080604052348015600e575f5ffd5b5061038a8061001c5f395ff3fe608060405234801561000f575f5ffd5b5060043610610029575f3560e01c8063ff01170a1461002d575b5f5ffd5b610047600480360381019061004291906101ac565b61005d565b6040516100549190610211565b60405180910390f35b5f5f5f61010073ffffffffffffffffffffffffffffffffffffffff168585604051610089929190610266565b5f60405180830381855afa9150503d805f81146100c1576040519150601f19603f3d011682016040523d82523d5f602084013e6100c6565b606091505b50915091508161010b576040517f08c379a0000000000000000000000000000000000000000000000000000000008152600401610102906102d8565b60405180910390fd5b6020815103610137575f818060200190518101906101299190610329565b90506001811493505061013b565b5f92505b505092915050565b5f5ffd5b5f5ffd5b5f5ffd5b5f5ffd5b5f5ffd5b5f5f83601f84011261016c5761016b61014b565b5b8235905067ffffffffffffffff8111156101895761018861014f565b5b6020830191508360018202830111156101a5576101a4610153565b5b9250929050565b5f5f602083850312156101c2576101c1610143565b5b5f83013567ffffffffffffffff8111156101df576101de610147565b5b6101eb85828601610157565b92509250509250929050565b5f8115159050919050565b61020b816101f7565b82525050565b5f6020820190506102245f830184610202565b92915050565b5f81905092915050565b828183375f83830152505050565b5f61024d838561022a565b935061025a838584610234565b82840190509392505050565b5f610272828486610242565b91508190509392505050565b5f82825260208201905092915050565b7f507265636f6d70696c652070726f626c656d00000000000000000000000000005f82015250565b5f6102c260128361027e565b91506102cd8261028e565b602082019050919050565b5f6020820190508181035f8301526102ef816102b6565b9050919050565b5f819050919050565b610308816102f6565b8114610312575f5ffd5b50565b5f81519050610323816102ff565b92915050565b5f6020828403121561033e5761033d610143565b5b5f61034b84828501610315565b9150509291505056fea2646970667358221220f5d2fdc95d365d9010cf8f5f40430ab42803215860b756f470d62ed7859d3e1364736f6c63430008220033")),
    // p256verify valid
    valtype(ParseHex("ff01170a000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000a0bb5a52f42f9c9261ed4361f59422a1e30036e7c32b270c8807a419feca6050237cf27b188d034f7e8a52380304b51ac3c08969e277f21b35a60b48fc47669978555555550000000055555555555555553ef7a8e48d07df81a693439654210c704fea55b32cb32aca0c12c4cd0abfb4e64b0f5a516e578c016591a93f5a0fbcc5d7d3fd10b2be668c547b212f6bb14c88f0fecd38a8a4b2c785ed3be62ce4b280")),
    // p256verify not valid
    valtype(ParseHex("ff01170a000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000a0bb5a52f42f9c9261ed4361f59422a1e30036e7c32b270c8807a419feca605023d45c5740946b2a147f59262ee6f5bc90bd01ed280528b62b3aed5fc93f06f739b329f479a2bbd0a5c384ee1493b1f5186a87139cac5df4087c134b49156847db2927b10512bae3eddcfe467828128bad2903269919f7086069c8c4df6c732838c7787964eaac00e5921fb1498a60f4606766b3d9685001558d1a974e7341513e")),

};

// Codes IDs used to check that osaka fork is present
enum class CodeID
{
    leadingZerosContract = 0,
    getCountLeadingZeros,
    secpVerifyContract,
    secpVerifyValid,
    secpVerifyNotValid
};

// Get the code identified by the ID
valtype getCode(CodeID id)
{
    return CODE[(int)id];
}

// Get the code identified by the ID
valtype getCode(CodeID id, dev::h256 nWord)
{
    valtype vCode = getCode(id);
    valtype vWord = nWord.asBytes();
    return concat(vCode, vWord);
}

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

BOOST_AUTO_TEST_CASE(checking_clz_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::leadingZerosContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Create clz opcode transactions
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txOsaka;
    for (uint32_t i = 0; i <= 256; i++)
    {
        dev::u256 nNumber = i < 3 ? i : (dev::u256) 1 << (i - 1);
        dev::h256 nWord = (dev::h256) nNumber;
        txOsaka.push_back(createQtumTransaction(getCode(CodeID::getCountLeadingZeros, nWord), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    }
    result = executeBC(txOsaka, *m_node.chainman);

    // Check clz opcode result
    for (uint32_t i = 0; i <= 256; i++)
    {
        BOOST_CHECK(result.first[i].execRes.excepted == dev::eth::TransactionException::None);
        uint32_t gasUsed = i == 0 ? 21768 : 21780;
        BOOST_CHECK(result.first[i].execRes.gasUsed == gasUsed);
        BOOST_CHECK(result.first[i].execRes.output.size() == 32);
        BOOST_CHECK(dev::h256(result.first[i].execRes.output) == dev::h256(256 - i));
    }
}

BOOST_AUTO_TEST_CASE(checking_clz_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::leadingZerosContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Create clz opcode transactions
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txOsaka;
    for (uint32_t i = 0; i <= 256; i++)
    {
        dev::u256 nNumber = i < 3 ? i : (dev::u256) 1 << (i - 1);
        dev::h256 nWord = (dev::h256) nNumber;
        txOsaka.push_back(createQtumTransaction(getCode(CodeID::getCountLeadingZeros, nWord), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    }
    result = executeBC(txOsaka, *m_node.chainman);

    // Check clz opcode result
    for (uint32_t i = 0; i <= 256; i++)
    {
        BOOST_CHECK(result.first[i].execRes.excepted == dev::eth::TransactionException::BadInstruction);
        BOOST_CHECK(result.first[i].execRes.gasUsed == GASLIMIT);
        BOOST_CHECK(result.first[i].execRes.output.size() == 0);
    }
}

BOOST_AUTO_TEST_CASE(checking_p256verify_precompile_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::secpVerifyContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Create p256verify precompile transactions
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txOsaka;
    txOsaka.push_back(createQtumTransaction(getCode(CodeID::secpVerifyValid), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txOsaka.push_back(createQtumTransaction(getCode(CodeID::secpVerifyNotValid), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txOsaka, *m_node.chainman);

    // Check p256verify precompile result with valid data
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 32269);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(1));

    // Check p256verify precompile result with not valid data
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 31947);
    BOOST_CHECK(result.first[1].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[1].execRes.output) == dev::h256(0));
}

BOOST_AUTO_TEST_CASE(checking_p256verify_precompile_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::secpVerifyContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Create p256verify precompile transactions
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txOsaka;
    txOsaka.push_back(createQtumTransaction(getCode(CodeID::secpVerifyValid), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txOsaka.push_back(createQtumTransaction(getCode(CodeID::secpVerifyNotValid), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txOsaka, *m_node.chainman);

    // Check p256verify precompile result with valid data
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 27511);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0));

    // Check p256verify precompile result with not valid data
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 27547);
    BOOST_CHECK(result.first[1].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[1].execRes.output) == dev::h256(0));
}

BOOST_AUTO_TEST_SUITE_END()

}
