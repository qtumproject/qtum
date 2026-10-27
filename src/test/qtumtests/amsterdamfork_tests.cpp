#include <boost/test/unit_test.hpp>
#include <test/qtumtests/test_utils.h>
#include <qtum/qtumutils.h>
#include <chainparams.h>

namespace AmsterdamTest{

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

// Codes used to check that amsterdam fork
const std::vector<valtype> CODE = {

};

// Codes IDs used to check that amsterdam fork is present
enum class CodeID
{
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
    evmConsensus.nOsakaHeight = coinbaseMaturity;
    evmConsensus.nAmsterdamHeight = forkHeight;
    UpdateAmsterdamHeight(forkHeight);
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
BOOST_FIXTURE_TEST_SUITE(amsterdamfork_tests, TestChain100Setup)
BOOST_AUTO_TEST_CASE(max_code_size_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);
    BOOST_CHECK(globalSealEngine->getQtumSchedule().maxCodeSize == 0x10000);
}

BOOST_AUTO_TEST_CASE(max_code_size_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);
    BOOST_CHECK(globalSealEngine->getQtumSchedule().maxCodeSize == 0x6000);
}

BOOST_AUTO_TEST_CASE(empty_test_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);
}

BOOST_AUTO_TEST_CASE(empty_test_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);
}

BOOST_AUTO_TEST_SUITE_END()

}
