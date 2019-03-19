#include <boost/test/unit_test.hpp>
#include <qtumtests/test_utils.h>
#include <script/standard.h>
#include <chainparams.h>

namespace BtcEcrecoverTest{

dev::u256 GASLIMIT = dev::u256(500000);
dev::h256 HASHTX = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

// Contract for btc_ecrecover check
std::vector<valtype> CODE = {
    /*
    pragma solidity ^0.4.0;
    library Crypto {
        function btc_ecrecover(bytes32 msgh, uint8 v, bytes32 r, bytes32 s) public view returns(bytes key)
        {
            uint256[4] memory input;
            input[0] = uint256(msgh);
            input[1] = v;
            input[2] = uint256(r);
            input[3] = uint256(s);

            uint256[2] memory p;
            assembly
            {
                if iszero(call(not(0), 0x85, 0, input, 0x80, p, 0x40))
                {
                    revert(0, 0)
                }
            }
            key = toBytes(p[0]);
        }

        function toBytes(uint256 x) internal pure returns (bytes b)
        {
            b = new bytes(32);
            assembly { mstore(add(b, 32), x) }
        }
    }
    */
    // Contract that call btc_ecrecover
    valtype(ParseHex("6060604052341561000f57600080fd5b6102958061001e6000396000f300606060405260043610610041576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806369bc096314610046575b600080fd5b61008660048080356000191690602001909190803560ff169060200190919080356000191690602001909190803560001916906020019091905050610101565b6040518080602001828103825283818151815260200191508051906020019080838360005b838110156100c65780820151818401526020810190506100ab565b50505050905090810190601f1680156100f35780820380516001836020036101000a031916815260200191505b509250505060405180910390f35b610109610206565b61011161021a565b610119610242565b866001900482600060048110151561012d57fe5b6020020181815250508560ff1682600160048110151561014957fe5b602002018181525050846001900482600260048110151561016657fe5b602002018181525050836001900482600360048110151561018357fe5b60200201818152505060408160808460006085600019f115156101a557600080fd5b6101c18160006002811015156101b757fe5b60200201516101cd565b92505050949350505050565b6101d5610206565b60206040518059106101e45750595b9080825280601f01601f19166020018201604052509050816020820152919050565b602060405190810160405280600081525090565b6080604051908101604052806004905b600081526020019060019003908161022a5790505090565b60408051908101604052806002905b600081526020019060019003908161025157905050905600a165627a7a72305820f7efe1b53a84d5eb09de6a3e8468390253480c7028e7985fc5d7cf546771f5c60029")),

    // btc_ecrecover(bytes32 msgh, uint8 v, bytes32 r, bytes32 s)
    valtype(ParseHex("69bc09631476abb745d423bf09273f1afd887d951181d25adc66c4834a70491911b7f750000000000000000000000000000000000000000000000000000000000000001ce6ca9bba58c88611fad66a6ce8f996908195593807c4b38bd528d2cff09d4eb33e5bfbbf4d3e39b1a2fd816a7680c19ebebaf3a141b239934ad43cb33fcec8ce")),

    // Valid key result
    valtype(ParseHex("0000000000000000000000005b0bf163d8d62090d634bc5c20bb47602cc686fe")),

    // Not valid key result
    valtype(ParseHex("0000000000000000000000000000000000000000000000000000000000000000"))
};

dev::bytes parseOutput(const dev::bytes& output)
{
    return dev::bytes(output.begin() + 64, output.end());
}

void genesisLoading(){
    const CChainParams& chainparams = Params();
    dev::eth::ChainParams cp((chainparams.EVMGenesisInfo(dev::eth::Network::qtumMainNetwork)));
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
BOOST_FIXTURE_TEST_SUITE(btcecrecoverfork_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(checking_btcecrecover_after_fork){
    // Initialize
    initState();
    genesisLoading();
    createNewBlocks(this,999 - COINBASE_MATURITY);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address()));
    executeBC(txs);

    // Call btc_ecrecover
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txBtcEcrecover;
    txBtcEcrecover.push_back(createQtumTransaction(CODE[1], 0, GASLIMIT, dev::u256(1), ++HASHTX, proxy));
    auto result = executeBC(txBtcEcrecover);
    dev::bytes output = parseOutput(result.first[0].execRes.output);
    BOOST_CHECK(dev::h256(output) == dev::h256(CODE[2]));
}

BOOST_AUTO_TEST_CASE(checking_btcecrecover_before_fork){
    // Initialize
    initState();
    genesisLoading();
    createNewBlocks(this,998 - COINBASE_MATURITY);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address()));
    executeBC(txs);

    // Call btc_ecrecover
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txBtcEcrecover;
    txBtcEcrecover.push_back(createQtumTransaction(CODE[1], 0, GASLIMIT, dev::u256(1), ++HASHTX, proxy));
    auto result = executeBC(txBtcEcrecover);
    dev::bytes output = parseOutput(result.first[0].execRes.output);
    BOOST_CHECK(dev::h256(output) == dev::h256(CODE[3]));
}

BOOST_AUTO_TEST_SUITE_END()

}
