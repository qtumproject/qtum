#include <boost/test/unit_test.hpp>
#include <qtumtests/test_utils.h>
#include <qtum/qtumutils.h>
#include <chainparams.h>

namespace ShanghaiTest{

const dev::u256 GASLIMIT = dev::u256(500000);
const dev::h256 HASHTX = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

// Codes used to check that shanghai fork
const std::vector<valtype> CODE = {
    /*
    pragma solidity ^0.8.0;

    contract ChainIdContract {
        function getChainId() public view returns (uint) {
            return block.chainid;
        }
    }
    */
    valtype(ParseHex("608060405234801561001057600080fd5b5060b58061001f6000396000f3fe6080604052348015600f57600080fd5b506004361060285760003560e01c80633408e47014602d575b600080fd5b60336047565b604051603e91906066565b60405180910390f35b600046905090565b6000819050919050565b606081604f565b82525050565b6000602082019050607960008301846059565b9291505056fea2646970667358221220aa82ea2e577edf0f5b49819ff1c0879014c07b8775b9ce5ad3c455daba68c4fd64736f6c63430008120033")),
    // getChainId()
    valtype(ParseHex("3408e470")),
    // push0_1()
    valtype(ParseHex("5f")),
    // push0_1024()
    valtype(ParseHex("5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f")),
    // push0_1025()
    valtype(ParseHex("5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f5f")),
};

// Codes IDs used to check that london fork is present
enum class CodeID
{
    chainIdcontract = 0,
    getChainId,
    push0_1,
    push0_1024,
    push0_1025,
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
    evmConsensus.nShanghaiHeight = forkHeight;
    UpdateShanghaiHeight(forkHeight);
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
BOOST_FIXTURE_TEST_SUITE(shanghaifork_tests, TestChain100Setup)

void checkChainid(ChainstateManager& chainman, const std::string& chain, const qtumutils::ChainIdType& chainId)
{
    // Select params
    int nShanghaiHeight = Params().GetConsensus().nShanghaiHeight;
    SelectParams(chain);
    UpdateShanghaiHeight(nShanghaiHeight);

    // Create contract
    dev::h256 hashTx(HASHTX);
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::chainIdcontract), 0, GASLIMIT, dev::u256(1), hashTx, dev::Address()));
    executeBC(txs, chainman);

    {
        // Call getChainId
        assert(txs.size() > 0);
        dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
        std::vector<QtumTransaction> txShanghai;
        txShanghai.push_back(createQtumTransaction(getCode(CodeID::getChainId), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
        auto result = executeBC(txShanghai, chainman);
        BOOST_CHECK(result.first[0].execRes.output.size() == 32);
        BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(chainId));
        BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    }
}

void checkPush0(dev::h256& hashTx, ChainstateManager& chainman, const CodeID& codeId, const dev::eth::TransactionException& excepted, const dev::u256& gasUsed)
{
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(codeId), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == excepted);
    BOOST_CHECK(result.first[0].execRes.gasUsed == gasUsed);
}

BOOST_AUTO_TEST_CASE(checking_chainid_shanghai_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);

    checkChainid(*m_node.chainman, CBaseChainParams::MAIN, qtumutils::ChainIdType::MAIN);
    checkChainid(*m_node.chainman, CBaseChainParams::TESTNET, qtumutils::ChainIdType::TESTNET);
    checkChainid(*m_node.chainman, CBaseChainParams::REGTEST, qtumutils::ChainIdType::REGTEST);
}

BOOST_AUTO_TEST_CASE(checking_chainid_shanghai_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);

    checkChainid(*m_node.chainman, CBaseChainParams::MAIN, qtumutils::ChainIdType::MAIN);
    checkChainid(*m_node.chainman, CBaseChainParams::TESTNET, qtumutils::ChainIdType::MAIN);
    checkChainid(*m_node.chainman, CBaseChainParams::REGTEST, qtumutils::ChainIdType::MAIN);
}

BOOST_AUTO_TEST_CASE(checking_push0_shanghai_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);

    dev::h256 hashTx(HASHTX);
    checkPush0(hashTx, *m_node.chainman, CodeID::push0_1, dev::eth::TransactionException::None, 53018);
    checkPush0(hashTx, *m_node.chainman, CodeID::push0_1024, dev::eth::TransactionException::None, 71432);
    checkPush0(hashTx, *m_node.chainman, CodeID::push0_1025, dev::eth::TransactionException::OutOfStack, GASLIMIT);
}

BOOST_AUTO_TEST_CASE(checking_push0_shanghai_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);

    dev::h256 hashTx(HASHTX);
    checkPush0(hashTx, *m_node.chainman, CodeID::push0_1, dev::eth::TransactionException::BadInstruction, GASLIMIT);
    checkPush0(hashTx, *m_node.chainman, CodeID::push0_1024, dev::eth::TransactionException::BadInstruction, GASLIMIT);
    checkPush0(hashTx, *m_node.chainman, CodeID::push0_1025, dev::eth::TransactionException::BadInstruction, GASLIMIT);
}

BOOST_AUTO_TEST_SUITE_END()

}
