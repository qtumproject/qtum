#include <boost/test/unit_test.hpp>
#include <qtumtests/test_utils.h>
#include <script/standard.h>
#include <chainparams.h>

namespace BtcEcrecoverTest{

const dev::u256 GASLIMIT = dev::u256(500000);
const dev::h256 HASHTX = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

// Contract for btc_ecrecover check
const std::vector<valtype> CODE = {
    /*
    pragma solidity ^0.4.0;
    library Crypto
    {
        function btc_ecrecover(bytes32 msgh, uint8 v, bytes32 r, bytes32 s) public view returns(bytes addr)
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
            addr = toBytes(p[0]);
        }

        function toBytes(uint256 x) internal pure returns (bytes b)
        {
            b = new bytes(32);
            assembly { mstore(add(b, 32), x) }
        }
    }
    */
    // Contract that call btc_ecrecover
    valtype(ParseHex("61029e610030600b82828239805160001a6073146000811461002057610022565bfe5b5030600052607381538281f3007300000000000000000000000000000000000000003014608060405260043610610058576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806369bc09631461005d575b600080fd5b6100a86004803603810190808035600019169060200190929190803560ff16906020019092919080356000191690602001909291908035600019169060200190929190505050610123565b6040518080602001828103825283818151815260200191508051906020019080838360005b838110156100e85780820151818401526020810190506100cd565b50505050905090810190601f1680156101155780820380516001836020036101000a031916815260200191505b509250505060405180910390f35b606061012d61022d565b610135610250565b866001900482600060048110151561014957fe5b6020020181815250508560ff1682600160048110151561016557fe5b602002018181525050846001900482600260048110151561018257fe5b602002018181525050836001900482600360048110151561019f57fe5b60200201818152505060408160808460006085600019f115156101c157600080fd5b6101dd8160006002811015156101d357fe5b60200201516101e9565b92505050949350505050565b606060206040519080825280601f01601f19166020018201604052801561021f5781602001602082028038833980820191505090505b509050816020820152919050565b608060405190810160405280600490602082028038833980820191505090505090565b60408051908101604052806002906020820280388339808201915050905050905600a165627a7a723058202c030fcf0261d136398030f4a0109ecc4c041333bf65bcbd4995b2f8332926890029")),

    // btc_ecrecover(bytes32 msgh, uint8 v, bytes32 r, bytes32 s) v=27
    valtype(ParseHex("69bc09631476abb745d423bf09273f1afd887d951181d25adc66c4834a70491911b7f750000000000000000000000000000000000000000000000000000000000000001be6ca9bba58c88611fad66a6ce8f996908195593807c4b38bd528d2cff09d4eb33e5bfbbf4d3e39b1a2fd816a7680c19ebebaf3a141b239934ad43cb33fcec8ce")),

    // btc_ecrecover(bytes32 msgh, uint8 v, bytes32 r, bytes32 s) v=28
    valtype(ParseHex("69bc09631476abb745d423bf09273f1afd887d951181d25adc66c4834a70491911b7f750000000000000000000000000000000000000000000000000000000000000001ce6ca9bba58c88611fad66a6ce8f996908195593807c4b38bd528d2cff09d4eb33e5bfbbf4d3e39b1a2fd816a7680c19ebebaf3a141b239934ad43cb33fcec8ce")),

    // btc_ecrecover(bytes32 msgh, uint8 v, bytes32 r, bytes32 s) v=31
    valtype(ParseHex("69bc09631476abb745d423bf09273f1afd887d951181d25adc66c4834a70491911b7f750000000000000000000000000000000000000000000000000000000000000001fe6ca9bba58c88611fad66a6ce8f996908195593807c4b38bd528d2cff09d4eb33e5bfbbf4d3e39b1a2fd816a7680c19ebebaf3a141b239934ad43cb33fcec8ce")),

    // btc_ecrecover(bytes32 msgh, uint8 v, bytes32 r, bytes32 s) v=32
    valtype(ParseHex("69bc09631476abb745d423bf09273f1afd887d951181d25adc66c4834a70491911b7f7500000000000000000000000000000000000000000000000000000000000000020e6ca9bba58c88611fad66a6ce8f996908195593807c4b38bd528d2cff09d4eb33e5bfbbf4d3e39b1a2fd816a7680c19ebebaf3a141b239934ad43cb33fcec8ce")),

    // Valid address result v=27
    valtype(ParseHex("00000000000000000000000011f50cfcb40d4e0bcdb4c728664d26bd969c8661")),

    // Valid address result v=28
    valtype(ParseHex("0000000000000000000000005b0bf163d8d62090d634bc5c20bb47602cc686fe")),

    // Valid address result v=31
    valtype(ParseHex("000000000000000000000000931c1f36709cd3aa8623a38db7a905d31ddf97e3")),

    // Valid address result v=32
    valtype(ParseHex("000000000000000000000000575e154116a125cee7053db093d94c6fb522144f")),

    // Not valid address result
    valtype(ParseHex("0000000000000000000000000000000000000000000000000000000000000000"))
};

dev::bytes parseOutput(const dev::bytes& output)
{
    return dev::bytes(output.begin() + 64, output.end());
}

void genesisLoading(){
    const CChainParams& chainparams = Params();
    dev::eth::ChainParams cp(chainparams.EVMGenesisInfo());
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
//    initState();
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), hashTx, dev::Address()));
    executeBC(txs, *m_node.chainman);

    // Call btc_ecrecover
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txBtcEcrecover;
    txBtcEcrecover.push_back(createQtumTransaction(CODE[1], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txBtcEcrecover.push_back(createQtumTransaction(CODE[2], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txBtcEcrecover.push_back(createQtumTransaction(CODE[3], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txBtcEcrecover.push_back(createQtumTransaction(CODE[4], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));

    // Execute contracts
    auto result = executeBC(txBtcEcrecover, *m_node.chainman);

    // Check results
    dev::bytes output = parseOutput(result.first[0].execRes.output);
    BOOST_CHECK(dev::h256(output) == dev::h256(CODE[5]));
    output = parseOutput(result.first[1].execRes.output);
    BOOST_CHECK(dev::h256(output) == dev::h256(CODE[6]));
    output = parseOutput(result.first[2].execRes.output);
    BOOST_CHECK(dev::h256(output) == dev::h256(CODE[7]));
    output = parseOutput(result.first[3].execRes.output);
    BOOST_CHECK(dev::h256(output) == dev::h256(CODE[8]));
}

BOOST_AUTO_TEST_CASE(checking_btcecrecover_before_fork){
    // Initialize
//    initState();
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), hashTx, dev::Address()));
    executeBC(txs, *m_node.chainman);

    // Call btc_ecrecover
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txBtcEcrecover;
    txBtcEcrecover.push_back(createQtumTransaction(CODE[1], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txBtcEcrecover.push_back(createQtumTransaction(CODE[2], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txBtcEcrecover.push_back(createQtumTransaction(CODE[3], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txBtcEcrecover.push_back(createQtumTransaction(CODE[4], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));

     // Execute contracts
    auto result = executeBC(txBtcEcrecover, *m_node.chainman);

    // Check results
    dev::bytes output = parseOutput(result.first[0].execRes.output);
    BOOST_CHECK(dev::h256(output) == dev::h256(CODE[9]));
    output = parseOutput(result.first[1].execRes.output);
    BOOST_CHECK(dev::h256(output) == dev::h256(CODE[9]));
    output = parseOutput(result.first[2].execRes.output);
    BOOST_CHECK(dev::h256(output) == dev::h256(CODE[9]));
    output = parseOutput(result.first[3].execRes.output);
    BOOST_CHECK(dev::h256(output) == dev::h256(CODE[9]));
}

BOOST_AUTO_TEST_SUITE_END()

}
