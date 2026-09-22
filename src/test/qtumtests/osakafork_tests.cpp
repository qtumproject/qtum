#include <boost/test/unit_test.hpp>
#include <test/qtumtests/test_utils.h>
#include <qtum/qtumutils.h>
#include <chainparams.h>
#include <test/qtumtests/precompiled_utils.h>
#include <test/qtumtests/data/modexp_eip7883.json.h>
#include <libethcore/ABI.h>

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
    // EIP-7823
    /*
    // SPDX-License-Identifier: MIT
    pragma solidity ^0.8.0;

    contract ModExp {
        /// @notice Run modular exponentiation.
        /// @param data Encoded input: [baseLength, expLength, modLength, base, exp, mod]
        /// @return result The raw bytes
        function run(bytes calldata data) public view returns (bytes memory result) {
            // Call precompile at address(5)
            (bool ok, bytes memory out) = address(5).staticcall(data);

            require(ok, "Precompile call failed");

            return out;
        }
    }
    */
    valtype(ParseHex("6080604052348015600e575f5ffd5b5061035c8061001c5f395ff3fe608060405234801561000f575f5ffd5b5060043610610029575f3560e01c80633a2765231461002d575b5f5ffd5b6100476004803603810190610042919061017f565b61005d565b604051610054919061023a565b60405180910390f35b60605f5f600573ffffffffffffffffffffffffffffffffffffffff168585604051610089929190610296565b5f60405180830381855afa9150503d805f81146100c1576040519150601f19603f3d011682016040523d82523d5f602084013e6100c6565b606091505b50915091508161010b576040517f08c379a000000000000000000000000000000000000000000000000000000000815260040161010290610308565b60405180910390fd5b809250505092915050565b5f5ffd5b5f5ffd5b5f5ffd5b5f5ffd5b5f5ffd5b5f5f83601f84011261013f5761013e61011e565b5b8235905067ffffffffffffffff81111561015c5761015b610122565b5b60208301915083600182028301111561017857610177610126565b5b9250929050565b5f5f6020838503121561019557610194610116565b5b5f83013567ffffffffffffffff8111156101b2576101b161011a565b5b6101be8582860161012a565b92509250509250929050565b5f81519050919050565b5f82825260208201905092915050565b8281835e5f83830152505050565b5f601f19601f8301169050919050565b5f61020c826101ca565b61021681856101d4565b93506102268185602086016101e4565b61022f816101f2565b840191505092915050565b5f6020820190508181035f8301526102528184610202565b905092915050565b5f81905092915050565b828183375f83830152505050565b5f61027d838561025a565b935061028a838584610264565b82840190509392505050565b5f6102a2828486610272565b91508190509392505050565b5f82825260208201905092915050565b7f507265636f6d70696c652063616c6c206661696c6564000000000000000000005f82015250565b5f6102f26016836102ae565b91506102fd826102be565b602082019050919050565b5f6020820190508181035f83015261031f816102e6565b905091905056fea2646970667358221220223a87221255c06dba1b99e5119f09e248657dc5dd6b47beae99a7c8d7b7208864736f6c63430008220033")),
    // run base length 1025
    valtype(ParseHex("3a27652300000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000463000000000000000000000000000000000000000000000000000000000000040100000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000101020000000000000000000000000000000000000000000000000000000000")),
    // run exponent length 1025
    valtype(ParseHex("3a27652300000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000463000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000004010000000000000000000000000000000000000000000000000000000000000001010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001020000000000000000000000000000000000000000000000000000000000")),
    // run modulus length 1025
    valtype(ParseHex("3a27652300000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000463000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000000401010100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000")),

};

// Codes IDs used to check that osaka fork is present
enum class CodeID
{
    leadingZerosContract = 0,
    getCountLeadingZeros,
    secpVerifyContract,
    secpVerifyValid,
    secpVerifyNotValid,
    modExpContract,
    modExpRunBase1025,
    modExpRunExponent1025,
    modExpRunModulus1025,
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

bool checkLastByteIsOne(const dev::bytes& rawData)
{
    bool ret = false;

    try
    {
        // Deserialize the byte array and check that the last byte is 1 and all other are 0
        dev::bytesConstRef o(&rawData);
        std::string output = dev::eth::ABIDeserialiser<std::string>::deserialise(o);
        size_t lastIndex = output.size() ? output.size() - 1 : 0;
        ret = output.size() > 0;
        for (size_t i = 0; i < output.size(); i++)
        {
            bool isLast = i == lastIndex;
            ret &= isLast ? output[i] == 1 : output[i] == 0;
        }
    }
    catch (...)
    {
        ret = false;
    }

    return ret;
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

BOOST_AUTO_TEST_CASE(checking_modexp_precompile_inputs_limit_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::modExpContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Create modexp precompile transactions
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txOsaka;
    txOsaka.push_back(createQtumTransaction(getCode(CodeID::modExpRunBase1025), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txOsaka.push_back(createQtumTransaction(getCode(CodeID::modExpRunExponent1025), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txOsaka.push_back(createQtumTransaction(getCode(CodeID::modExpRunModulus1025), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txOsaka, *m_node.chainman);

    // Check modexp precompile result with base length more then 1024 bytes
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::RevertInstruction);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 492961);

    // Check modexp precompile result with exponent length more then 1024 bytes
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::RevertInstruction);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 492961);

    // Check modexp precompile result with modulus length more then 1024 bytes
    BOOST_CHECK(result.first[2].execRes.excepted == dev::eth::TransactionException::RevertInstruction);
    BOOST_CHECK(result.first[2].execRes.gasUsed == 492961);
}

BOOST_AUTO_TEST_CASE(checking_modexp_precompile_inputs_limit_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::modExpContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Create modexp precompile transactions
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txOsaka;
    txOsaka.push_back(createQtumTransaction(getCode(CodeID::modExpRunBase1025), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txOsaka.push_back(createQtumTransaction(getCode(CodeID::modExpRunExponent1025), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txOsaka.push_back(createQtumTransaction(getCode(CodeID::modExpRunModulus1025), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txOsaka, *m_node.chainman);

    // Check modexp precompile result with base length more then 1024 bytes
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 33231);
    BOOST_CHECK(result.first[0].execRes.output.size() == 96);
    BOOST_CHECK(checkLastByteIsOne(result.first[0].execRes.output));

    // Check modexp precompile result with exponent length more then 1024 bytes
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 30332);
    BOOST_CHECK(result.first[1].execRes.output.size() == 96);
    BOOST_CHECK(checkLastByteIsOne(result.first[1].execRes.output));

    // Check modexp precompile result with modulus length more then 1024 bytes
    BOOST_CHECK(result.first[2].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[2].execRes.gasUsed == 33529);
    BOOST_CHECK(result.first[2].execRes.output.size() == 1120);
    BOOST_CHECK(checkLastByteIsOne(result.first[2].execRes.output));
}

BOOST_AUTO_TEST_SUITE_END()

}
