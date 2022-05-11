#include <boost/test/unit_test.hpp>
#include <qtumtests/test_utils.h>
#include <script/standard.h>
#include <chainparams.h>

namespace EvmoneTest{

const dev::u256 GASLIMIT = dev::u256(500000);
const dev::h256 HASHTX = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

const std::vector<valtype> CODE = {
    // deploy contract that produce create with value exception using OP_CREATE
    valtype(ParseHex("602060006001f0600155")),
    // deploy contract that produce create with value exception using OP_CREATE2
    valtype(ParseHex("605a604160006001f5600155"))
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
BOOST_FIXTURE_TEST_SUITE(Evmone_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(checking_create_with_value){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), hashTx, dev::Address()));
    txs.push_back(createQtumTransaction(CODE[1], 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);

    // Check create with value
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::CreateWithValue);
    BOOST_CHECK(result.first[0].execRes.gasUsed == GASLIMIT);
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::CreateWithValue);
    BOOST_CHECK(result.first[1].execRes.gasUsed == GASLIMIT);

}

BOOST_AUTO_TEST_SUITE_END()

}
