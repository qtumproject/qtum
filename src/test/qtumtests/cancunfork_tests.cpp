#include <boost/test/unit_test.hpp>
#include <test/qtumtests/test_utils.h>
#include <qtum/qtumutils.h>
#include <chainparams.h>

namespace CancunTest{

const dev::u256 GASLIMIT = dev::u256(500000);
const dev::h256 HASHTX = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

// Codes used to check that cancun fork
const std::vector<valtype> CODE = {
    // EIP-1153
    /*
    pragma solidity ^0.8.24;

    contract MulService {
        function setMultiplier(uint multiplier) external {
            assembly {
                tstore(0, multiplier)
            }
        }

        function getMultiplier() private view returns (uint multiplier) {
            assembly {
                multiplier := tload(0)
            }
        }

        function multiply(uint value) external view returns (uint) {
            return value * getMultiplier();
        }
    }
    */
    valtype(ParseHex("6080604052348015600e575f80fd5b506101db8061001c5f395ff3fe608060405234801561000f575f80fd5b5060043610610034575f3560e01c8063641579a614610038578063c6888fa114610054575b5f80fd5b610052600480360381019061004d91906100e4565b610084565b005b61006e600480360381019061006991906100e4565b61008a565b60405161007b919061011e565b60405180910390f35b805f5d50565b5f6100936100a5565b8261009e9190610164565b9050919050565b5f805c905090565b5f80fd5b5f819050919050565b6100c3816100b1565b81146100cd575f80fd5b50565b5f813590506100de816100ba565b92915050565b5f602082840312156100f9576100f86100ad565b5b5f610106848285016100d0565b91505092915050565b610118816100b1565b82525050565b5f6020820190506101315f83018461010f565b92915050565b7f4e487b71000000000000000000000000000000000000000000000000000000005f52601160045260245ffd5b5f61016e826100b1565b9150610179836100b1565b9250828202610187816100b1565b9150828204841483151761019e5761019d610137565b5b509291505056fea26469706673582212203dda524f45e82e20c7a5140d36ef2410c23bec4dee31ca21bc368cf508cde0f764736f6c634300081a0033")),
    // setMultiplier(5)
    valtype(ParseHex("641579a60000000000000000000000000000000000000000000000000000000000000005")),
    // multiply(7)
    valtype(ParseHex("c6888fa10000000000000000000000000000000000000000000000000000000000000007")),

    // EIP-7516
    /*
    pragma solidity 0.8.25;

    contract BlobBaseFee {
        function getBlobBaseFee() external view returns (uint256 blobBaseFee) {
            assembly {
                blobBaseFee := blobbasefee()
            }
        }
    }
    */
    valtype(ParseHex("6080604052348015600e575f80fd5b5060ae80601a5f395ff3fe6080604052348015600e575f80fd5b50600436106026575f3560e01c80631f6d6ef714602a575b5f80fd5b60306044565b604051603b91906061565b60405180910390f35b5f4a905090565b5f819050919050565b605b81604b565b82525050565b5f60208201905060725f8301846054565b9291505056fea2646970667358221220d07c85aaeda13f99ab3684717e3cb625952e90b29aeb1493d1e517a3832d385564736f6c63430008190033")),
    // getBlobBaseFee()
    valtype(ParseHex("1f6d6ef7")),

    // EIP-5656
    /*
    pragma solidity 0.8.25;

    contract MemoryCopy {
        function memoryCopy() external pure returns (bytes32 x) {
            assembly {
                mstore(0x20, 0x50)  // Store 0x50 at word 1 in memory
                mcopy(0, 0x20, 0x20)  // Copies 0x50 to word 0 in memory
                x := mload(0)    // Returns 32 bytes "0x50"
            }
        }
    }
    */
    valtype(ParseHex("6080604052348015600e575f80fd5b5060b980601a5f395ff3fe6080604052348015600e575f80fd5b50600436106026575f3560e01c80632dbaeee914602a575b5f80fd5b60306044565b604051603b9190606c565b60405180910390f35b5f60506020526020805f5e5f51905090565b5f819050919050565b6066816056565b82525050565b5f602082019050607d5f830184605f565b9291505056fea2646970667358221220ec84bee5416d7b140d71e23734f9e1a309a366eb0b1d63b19ee56f026ac3713a64736f6c63430008190033")),
    // memoryCopy()
    valtype(ParseHex("2dbaeee9")),

    // EIP-4844
    /*
    pragma solidity 0.8.25;

    contract EmptyBlob {
        function getBlobHash(uint256 index) external view returns (bytes32 x) {
            assembly {
                x := blobhash(index)
            }
        }
    }
    */
    valtype(ParseHex("6080604052348015600e575f80fd5b506101198061001c5f395ff3fe6080604052348015600e575f80fd5b50600436106026575f3560e01c80634ebf82e514602a575b5f80fd5b60406004803603810190603c91906090565b6054565b604051604b919060cc565b60405180910390f35b5f81499050919050565b5f80fd5b5f819050919050565b6072816062565b8114607b575f80fd5b50565b5f81359050608a81606b565b92915050565b5f6020828403121560a25760a1605e565b5b5f60ad84828501607e565b91505092915050565b5f819050919050565b60c68160b6565b82525050565b5f60208201905060dd5f83018460bf565b9291505056fea2646970667358221220e1794f1ba3bc6a431eab483e401ed5c1562913b49baee8a4d484d87b39c4ae2164736f6c63430008190033")),
    // getBlobHash(7)
    valtype(ParseHex("4ebf82e50000000000000000000000000000000000000000000000000000000000000007")),

    /*
    pragma solidity ^0.8.0;

    contract PointEvaluation {
        function verifyKZGCommitment(
            bytes calldata data
        ) public view returns (bool) {
            (bool ok, bytes memory out) = address(10).staticcall(data);
            require(ok, "Precompile call failed");
            bool ret = ok && out.length != 0;
            return ret;
        }
    }
    */
    valtype(ParseHex("6080604052348015600e575f80fd5b506103108061001c5f395ff3fe608060405234801561000f575f80fd5b5060043610610029575f3560e01c80637b7b86101461002d575b5f80fd5b61004760048036038101906100429190610190565b61005d565b60405161005491906101f5565b60405180910390f35b5f805f600a73ffffffffffffffffffffffffffffffffffffffff16858560405161008892919061024a565b5f60405180830381855afa9150503d805f81146100c0576040519150601f19603f3d011682016040523d82523d5f602084013e6100c5565b606091505b50915091508161010a576040517f08c379a0000000000000000000000000000000000000000000000000000000008152600401610101906102bc565b60405180910390fd5b5f82801561011957505f825114155b905080935050505092915050565b5f80fd5b5f80fd5b5f80fd5b5f80fd5b5f80fd5b5f8083601f8401126101505761014f61012f565b5b8235905067ffffffffffffffff81111561016d5761016c610133565b5b60208301915083600182028301111561018957610188610137565b5b9250929050565b5f80602083850312156101a6576101a5610127565b5b5f83013567ffffffffffffffff8111156101c3576101c261012b565b5b6101cf8582860161013b565b92509250509250929050565b5f8115159050919050565b6101ef816101db565b82525050565b5f6020820190506102085f8301846101e6565b92915050565b5f81905092915050565b828183375f83830152505050565b5f610231838561020e565b935061023e838584610218565b82840190509392505050565b5f610256828486610226565b91508190509392505050565b5f82825260208201905092915050565b7f507265636f6d70696c652063616c6c206661696c6564000000000000000000005f82015250565b5f6102a6601683610262565b91506102b182610272565b602082019050919050565b5f6020820190508181035f8301526102d38161029a565b905091905056fea26469706673582212203da8696d22b3251f14701af9901c2d5a0471851a1942e162490ab54450eecc3b64736f6c634300081a0033")),
    // verifyKZGCommitment(correct_proof)
    valtype(ParseHex("7b7b8610000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000c0012b08a0504a63aac18383db69fe6b52fc833e3d060b87c2726c4140c909d91807dddd3c80995c2bb3012943e2036e77490b1f6ddc58ca39a4fb4f3225ae56ab11dc2c4d89f777f0f5c2a51f45b73ff1538761f9cf23ed74c74472fea625ad8bace1db77e25ceb316d914182e05dd810f112352e1d6ed9e47af28e2f64e22b94c411794359c2273bc10bc0390963fb1a97bb642307bfa4424c66bd90ecc0ecffd5045e492b40304df20346693db7450457e2c72588a6a2b1a16909e2ab1e6284")),
    // verifyKZGCommitment(incorrect_proof)
    valtype(ParseHex("7b7b8610000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000c001e798154708fe7789429634053cbf9f99b619f9f084048927333fce637f549b5eb7004fe57383e6c88b99d839937fddf3f99279353aaf8d5c9a75f91ce33c624882cf0609af8c7cd4c256e63a35838c95a9ebbf6122540ab344b42fd66d32e18f59a8d2a1a625a17f3fea0fe5eb8c896db3764f3185481bc22f91b4aaffcca25f26936857bc3a7c2539ea8ec3a952b7b8f731ba6a52e419ffc843c50d2947d30e933e3a881b208de54149714ece74a599503f84c6249b5fd8a7c70189882a6b")),
};

// Codes IDs used to check that london fork is present
enum class CodeID
{
    transientStorageContract = 0,
    setMultiplier_5,
    multiply_7,
    blobBaseFeeContract,
    getBlobBaseFee,
    memoryCopyContract,
    memoryCopy_80,
    emptyBlobContract,
    getBlobHash_7,
    pointEvaluationContract,
    verifyKzgCorrectProof,
    verifyKzgIncorrectProof
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
    evmConsensus.nLondonHeight = coinbaseMaturity;
    evmConsensus.nShanghaiHeight = coinbaseMaturity;
    evmConsensus.nCancunHeight = forkHeight;
    UpdateCancunHeight(forkHeight);
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
BOOST_FIXTURE_TEST_SUITE(cancunfork_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(checking_transient_storage_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::transientStorageContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Create a transaction with 2 outputs that use transient storage.
    // The first output set the multiplier to 5.
    // The second output multiply the multiplier by 7, and the result is 35.
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txCancun;
    txCancun.push_back(createQtumTransaction(getCode(CodeID::setMultiplier_5), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txCancun.push_back(createQtumTransaction(getCode(CodeID::multiply_7), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txCancun, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 21688);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 22179);
    BOOST_CHECK(result.first[0].execRes.output.size() == 0);
    BOOST_CHECK(result.first[1].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[1].execRes.output) == dev::h256(35));

    // Create a transaction with an output that use transient storage which is expired.
    txCancun.clear();
    txCancun.push_back(createQtumTransaction(getCode(CodeID::multiply_7), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txCancun, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 22179);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0));
}

BOOST_AUTO_TEST_CASE(checking_transient_storage_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::transientStorageContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that tstore and tload are bad instructions
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txCancun;
    txCancun.push_back(createQtumTransaction(getCode(CodeID::setMultiplier_5), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txCancun.push_back(createQtumTransaction(getCode(CodeID::multiply_7), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txCancun, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::BadInstruction);
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::BadInstruction);
}

BOOST_AUTO_TEST_CASE(checking_blobbasefee_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::blobBaseFeeContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that blobbasefee is 0
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txCancun;
    txCancun.push_back(createQtumTransaction(getCode(CodeID::getBlobBaseFee), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txCancun, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 21373);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0));
}

BOOST_AUTO_TEST_CASE(checking_blobbasefee_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::blobBaseFeeContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that blobbasefee is bad instruction
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txCancun;
    txCancun.push_back(createQtumTransaction(getCode(CodeID::getBlobBaseFee), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txCancun, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::BadInstruction);
}

BOOST_AUTO_TEST_CASE(checking_memory_copy_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::memoryCopyContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that memory copy is 80
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txCancun;
    txCancun.push_back(createQtumTransaction(getCode(CodeID::memoryCopy_80), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txCancun, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 21399);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(80));
}

BOOST_AUTO_TEST_CASE(checking_memory_copy_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::memoryCopyContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that memory copy is bad instruction
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txCancun;
    txCancun.push_back(createQtumTransaction(getCode(CodeID::memoryCopy_80), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txCancun, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::BadInstruction);
}

BOOST_AUTO_TEST_CASE(checking_blob_hash_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::emptyBlobContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that get blob hash is 0 for any index, because it is not a blob transaction
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txCancun;
    txCancun.push_back(createQtumTransaction(getCode(CodeID::getBlobHash_7), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txCancun, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 21778);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0));
}

BOOST_AUTO_TEST_CASE(checking_blob_hash_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::emptyBlobContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that blob hash is bad instruction
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txCancun;
    txCancun.push_back(createQtumTransaction(getCode(CodeID::getBlobHash_7), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txCancun, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::BadInstruction);
}

BOOST_AUTO_TEST_CASE(checking_point_evaluation_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::pointEvaluationContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that point evaluation precompiled contract exist
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txCancun;
    txCancun.push_back(createQtumTransaction(getCode(CodeID::verifyKzgCorrectProof), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txCancun.push_back(createQtumTransaction(getCode(CodeID::verifyKzgIncorrectProof), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txCancun, *m_node.chainman);

    // Check point evaluation with valid data
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 75686);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(1));

    // Check point evaluation with not valid data
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::RevertInstruction);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 492933);
}

BOOST_AUTO_TEST_CASE(checking_point_evaluation_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::pointEvaluationContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that point evaluation precompiled contract doesn't exist
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txCancun;
    txCancun.push_back(createQtumTransaction(getCode(CodeID::verifyKzgCorrectProof), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txCancun.push_back(createQtumTransaction(getCode(CodeID::verifyKzgIncorrectProof), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txCancun, *m_node.chainman);

    // Check point evaluation with valid data
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 28114);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0));

    // Check point evaluation with not valid data
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 28102);
    BOOST_CHECK(result.first[1].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[1].execRes.output) == dev::h256(0));
}

BOOST_AUTO_TEST_SUITE_END()

}
