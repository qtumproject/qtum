#include <boost/test/unit_test.hpp>
#include <qtumtests/test_utils.h>
#include <script/standard.h>

namespace ByzantiumTest{

dev::u256 GASLIMIT = dev::u256(500000);
dev::h256 HASHTX = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

std::vector<valtype> CODE = {
	/*
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
	 valtype(ParseHex("40c10f1900000000000000000000000001010101010101010101010101010101010101010000000000000000000000000000000000000000000000000000000000000010"))
};

void genesisLoading(){
    dev::eth::ChainParams cp((dev::eth::genesisInfo(dev::eth::Network::qtumMainNetwork)));
    cp.byzantiumForkBlock = dev::u256(1000);
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
BOOST_FIXTURE_TEST_SUITE(byzantiumfork_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(checking_returndata_opcode_after_fork){
    initState();
    genesisLoading();
    createNewBlocks(this,999 - COINBASE_MATURITY);
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address()));
    txs.push_back(createQtumTransaction(CODE[1], 0, GASLIMIT, dev::u256(1), ++HASHTX, dev::Address()));
    executeBC(txs);
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());

    std::vector<QtumTransaction> txsCall;
    txsCall.push_back(createQtumTransaction(CODE[2], 0, GASLIMIT, dev::u256(1), ++HASHTX, proxy));
    executeBC(txsCall);

    std::vector<QtumTransaction> txsMint;
    txsMint.push_back(createQtumTransaction(CODE[3], 0, GASLIMIT, dev::u256(1), ++HASHTX, proxy));
    auto result = executeBC(txsMint);

    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    std::vector<QtumTransaction> txsbalance;
    txsbalance.push_back(createQtumTransaction(ParseHex("70a082310000000000000000000000000101010101010101010101010101010101010101"), 0, GASLIMIT, dev::u256(1), ++HASHTX, proxy));
    result = executeBC(txsbalance);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0x0000000000000000000000000000000000000000000000000000000000000020));

}

BOOST_AUTO_TEST_CASE(checking_returndata_opcode_before_fork){
    initState();
    genesisLoading();
    createNewBlocks(this,998 - COINBASE_MATURITY);
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address()));
    txs.push_back(createQtumTransaction(CODE[1], 0, GASLIMIT, dev::u256(1), ++HASHTX, dev::Address()));
    executeBC(txs);
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());

    std::vector<QtumTransaction> txsCall;
    txsCall.push_back(createQtumTransaction(CODE[2], 0, GASLIMIT, dev::u256(1), ++HASHTX, proxy));
    executeBC(txsCall);

    std::vector<QtumTransaction> txsMint;
    txsMint.push_back(createQtumTransaction(CODE[3], 0, GASLIMIT, dev::u256(1), ++HASHTX, proxy));
    auto result = executeBC(txsMint);

    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::BadInstruction);
}

BOOST_AUTO_TEST_SUITE_END()

}