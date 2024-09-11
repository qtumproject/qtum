#include <boost/test/unit_test.hpp>
#include <qtumtests/test_utils.h>
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
};

// Codes IDs used to check that london fork is present
enum class CodeID
{
    transientStorageContract = 0,
    setMultiplier_5,
    multiply_7,
    blobBaseFeeContract,
    getBlobBaseFee
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

BOOST_AUTO_TEST_SUITE_END()

}
