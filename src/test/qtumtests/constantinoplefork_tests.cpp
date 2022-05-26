#include <boost/test/unit_test.hpp>
#include <qtumtests/test_utils.h>
#include <script/standard.h>
#include <chainparams.h>

namespace ConstantinopleTest{

const dev::u256 GASLIMIT = dev::u256(500000);
const dev::h256 HASHTX = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

// Codes used to check that the newer forks up to constantinople are present
const std::vector<valtype> CODE = {
    /*
    // Contract for Byzantium check
    contract Proxy {
        address public implementation;

        function upgradeTo(address _address) public {
            implementation = _address;
        }

        function () payable public {
            address _impl = implementation;
            require(_impl != address(0));

            assembly {
                let ptr := mload(0x40)
                calldatacopy(ptr, 0, calldatasize)
                let result := delegatecall(gas, _impl, ptr, calldatasize, 0, 0)
                let size := returndatasize
                returndatacopy(ptr, 0, size)

                switch result
                case 0 { revert(ptr, size) }
                default { return(ptr, size) }
            }
        }
    }
    */
    valtype(ParseHex("608060405234801561001057600080fd5b50610187806100206000396000f30060806040526004361061004b5763ffffffff7c01000000000000000000000000000000000000000000000000000000006000350416633659cfe681146100955780635c60da1b146100c5575b60005473ffffffffffffffffffffffffffffffffffffffff1680151561007057600080fd5b60405136600082376000803683855af43d806000843e818015610091578184f35b8184fd5b3480156100a157600080fd5b506100c373ffffffffffffffffffffffffffffffffffffffff60043516610103565b005b3480156100d157600080fd5b506100da61013f565b6040805173ffffffffffffffffffffffffffffffffffffffff9092168252519081900360200190f35b6000805473ffffffffffffffffffffffffffffffffffffffff191673ffffffffffffffffffffffffffffffffffffffff92909216919091179055565b60005473ffffffffffffffffffffffffffffffffffffffff16815600a165627a7a723058209f0b14aef0ec22d6019dddc8ad7a0c8f0fbb76c8847e588da0a1559bc958db040029")),

    /*
    // Contract for Byzantium check
    contract TokenVersion1 {
        mapping (address => uint) balances;

        event Transfer(address _from, address _to, uint256 _value);

        function balanceOf(address _address) public view returns (uint) {
            return balances[_address];
        }

        function transfer(address _to, uint256 _value) public {
            require(balances[msg.sender] >= _value);
            balances[msg.sender] -= _value;
            balances[_to] += _value;
            emit Transfer(msg.sender, _to, _value);
        }

        function mint(address _to, uint256 _value) public {
            balances[_to] += _value * 2;
            emit Transfer(0x0, _to, _value);
        }
    }
    */
    valtype(ParseHex("608060405234801561001057600080fd5b506101e3806100206000396000f3006080604052600436106100565763ffffffff7c010000000000000000000000000000000000000000000000000000000060003504166340c10f19811461005b57806370a082311461008e578063a9059cbb146100ce575b600080fd5b34801561006757600080fd5b5061008c73ffffffffffffffffffffffffffffffffffffffff600435166024356100ff565b005b34801561009a57600080fd5b506100bc73ffffffffffffffffffffffffffffffffffffffff60043516610134565b60408051918252519081900360200190f35b3480156100da57600080fd5b5061008c73ffffffffffffffffffffffffffffffffffffffff6004351660243561015c565b73ffffffffffffffffffffffffffffffffffffffff909116600090815260208190526040902080546002909202919091019055565b73ffffffffffffffffffffffffffffffffffffffff1660009081526020819052604090205490565b3360009081526020819052604090205481111561017857600080fd5b336000908152602081905260408082208054849003905573ffffffffffffffffffffffffffffffffffffffff93909316815291909120805490910190555600a165627a7a72305820c517c25d8609e1668bebed32141ed2c2415e8b77ba9f2aef29c6d84e5756b4c20029")),

    // upgradeTo(address _address)
    valtype(ParseHex("3659cfe6000000000000000000000000c4c1d7375918557df2ef8f1d1f0b2329cb248a15")),

    // mint(address _to, uint256 _value)
    valtype(ParseHex("40c10f1900000000000000000000000001010101010101010101010101010101010101010000000000000000000000000000000000000000000000000000000000000010")),

    /*
    // Contract for Constantinople check
    contract ConstantinopleCheck2 {
        event Constantinople(bool);

        // call this function to check if we are on constantinple
        function IsItConstantinople() public view returns (bool){
            (bool success) = address(this).call(abi.encodeWithSignature("ConstantinopleCheckFunction()"));
            emit Constantinople(success);
            return success;
        }

        // reverts if not constantinople
        // call IsItConstantinople() not this one (is available to be called though)
        // using shl for low gas use. "now" has to be called to make sure this function doesnt return a constant
        function ConstantinopleCheckFunction() public view returns (bytes32){
            bytes32 test = bytes32(now);
            assembly {
                test := shl(test, 1)
            }
            return test; // explicitly return test so we are sure that the optimizer doesnt optimize this out
        }
    }
    */
    valtype(ParseHex("608060405234801561001057600080fd5b506101d5806100206000396000f30060806040526004361061004b5763ffffffff7c01000000000000000000000000000000000000000000000000000000006000350416634ac429f28114610050578063f8af251414610077575b600080fd5b34801561005c57600080fd5b506100656100a0565b60408051918252519081900360200190f35b34801561008357600080fd5b5061008c6100a7565b604080519115158252519081900360200190f35b6001421b90565b60408051600481526024810182526020810180517bffffffffffffffffffffffffffffffffffffffffffffffffffffffff167f4ac429f2000000000000000000000000000000000000000000000000000000001781529151815160009384933093909290918291808383895b8381101561012b578181015183820152602001610113565b50505050905090810190601f1680156101585780820380516001836020036101000a031916815260200191505b509150506000604051808303816000865af160408051821515815290519194507f27cb433ab98fc487efab30ed965c7b21cb0c65b05a72ae5bf5a9815af956fe1693508190036020019150a19190505600a165627a7a7230582069c72d05c3b31b65dc0b5de1dc97b02549a66fe816407fc4e98bcee6792074c40029")),

    // IsItConstantinople()
    valtype(ParseHex("f8af2514"))
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
BOOST_FIXTURE_TEST_SUITE(constantinoplefork_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(checking_returndata_opcode_after_fork){
    // Initialize
//    initState();
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contracts
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), hashTx, dev::Address()));
    txs.push_back(createQtumTransaction(CODE[1], 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    executeBC(txs, *m_node.chainman);

    // Call upgrade to
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txsCall;
    txsCall.push_back(createQtumTransaction(CODE[2], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    executeBC(txsCall, *m_node.chainman);

    // Call mint
    std::vector<QtumTransaction> txsMint;
    txsMint.push_back(createQtumTransaction(CODE[3], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    auto result = executeBC(txsMint, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Call balance of
    std::vector<QtumTransaction> txsbalance;
    txsbalance.push_back(createQtumTransaction(ParseHex("70a082310000000000000000000000000101010101010101010101010101010101010101"), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txsbalance, *m_node.chainman);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0x0000000000000000000000000000000000000000000000000000000000000020));
}

BOOST_AUTO_TEST_CASE(checking_returndata_opcode_before_fork){
    // Initialize
//    initState();
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contracts
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), hashTx, dev::Address()));
    txs.push_back(createQtumTransaction(CODE[1], 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    executeBC(txs, *m_node.chainman);

    // Call upgrade to
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txsCall;
    txsCall.push_back(createQtumTransaction(CODE[2], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    executeBC(txsCall, *m_node.chainman);

    // Call mint
    std::vector<QtumTransaction> txsMint;
    txsMint.push_back(createQtumTransaction(CODE[3], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    auto result = executeBC(txsMint, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::BadInstruction);
}

BOOST_AUTO_TEST_CASE(checking_constantinople_after_fork){
    // Initialize
//    initState();
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(CODE[4], 0, GASLIMIT, dev::u256(1), hashTx, dev::Address()));
    executeBC(txs, *m_node.chainman);

    // Call is it constantinople
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txIsItConstantinople;
    txIsItConstantinople.push_back(createQtumTransaction(CODE[5], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    auto result = executeBC(txIsItConstantinople, *m_node.chainman);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0x0000000000000000000000000000000000000000000000000000000000000001));
}

BOOST_AUTO_TEST_CASE(checking_constantinople_before_fork){
    // Initialize
//    initState();
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(CODE[4], 0, GASLIMIT, dev::u256(1), hashTx, dev::Address()));
    executeBC(txs, *m_node.chainman);

    // Call is it constantinople
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txIsItConstantinople;
    txIsItConstantinople.push_back(createQtumTransaction(CODE[5], 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    auto result = executeBC(txIsItConstantinople, *m_node.chainman);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0x0000000000000000000000000000000000000000000000000000000000000000));
}

BOOST_AUTO_TEST_SUITE_END()

}
