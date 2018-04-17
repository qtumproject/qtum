#include <boost/test/unit_test.hpp>
#include <qtumtests/test_utils.h>

std::vector<valtype> code = {
    /*
        contract Sender1 {
            address sender2;
            address sender3;
            
            function Sender1() {
            }
            function setSenders(address senderx, address sendery) public{
                sender2 = senderx;
                sender3 = sendery;
            }
            function share() public payable{
                if(msg.sender != address(sender3)){
                    sender2.call.value(msg.value/2)(bytes4(sha3("share()")));
                }
            }
            function sendAll() public payable{
                sender2.call.value(this.balance)(bytes4(sha3("keep()")));
            }
            function keep() public payable{
            }
            function() payable { } //always payable
        }
    */
    valtype(ParseHex("606060405234610000575b5b5b6103bf8061001b6000396000f30060606040523615610060576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680635579818d14610069578063a8d5fd65146100bb578063e14f680f146100c5578063e4d06d82146100cf575b6100675b5b565b005b34610000576100b9600480803573ffffffffffffffffffffffffffffffffffffffff1690602001909190803573ffffffffffffffffffffffffffffffffffffffff169060200190919050506100d9565b005b6100c3610160565b005b6100cd61029d565b005b6100d7610390565b005b81600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555080600160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b5050565b600160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff163373ffffffffffffffffffffffffffffffffffffffff1614151561029a57600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff166002348115610000570460405180807f7368617265282900000000000000000000000000000000000000000000000000815250600701905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886185025a03f19350505050505b5b565b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff163073ffffffffffffffffffffffffffffffffffffffff163160405180807f6b65657028290000000000000000000000000000000000000000000000000000815250600601905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886185025a03f19350505050505b565b5b5600a165627a7a7230582094424c92e68d8ea77caec662d2895cc9086f115ed7baa3f7e508b4d8d011161f0029")),
    /*
        contract Sender2{
            address sender1;
            address sender3;
            
            function Sender2() {
            }
            function setSenders(address senderx, address sendery) public{
                sender1 = senderx;
                sender3 = sendery;
            }
            function share() public payable{
                sender3.call.value(msg.value/2)(bytes4(sha3("share()")));
            }
            function keep() public payable{
            }
            function withdrawAll() public{
                sender3.call(bytes4(sha3("withdraw()")));
                msg.sender.send(this.balance);
            }
            function() payable { } //always payable
        }
    */
    valtype(ParseHex("606060405234610000575b5b5b6103a38061001b6000396000f30060606040523615610060576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680635579818d14610069578063853828b6146100bb578063a8d5fd65146100ca578063e4d06d82146100d4575b6100675b5b565b005b34610000576100b9600480803573ffffffffffffffffffffffffffffffffffffffff1690602001909190803573ffffffffffffffffffffffffffffffffffffffff169060200190919050506100de565b005b34610000576100c8610165565b005b6100d261028f565b005b6100dc610374565b005b81600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555080600160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b5050565b600160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1660405180807f7769746864726177282900000000000000000000000000000000000000000000815250600a01905060405180910390207c010000000000000000000000000000000000000000000000000000000090046040518163ffffffff167c01000000000000000000000000000000000000000000000000000000000281526004018090506000604051808303816000876161da5a03f192505050503373ffffffffffffffffffffffffffffffffffffffff166108fc3073ffffffffffffffffffffffffffffffffffffffff16319081150290604051809050600060405180830381858888f19350505050505b565b600160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff166002348115610000570460405180807f7368617265282900000000000000000000000000000000000000000000000000815250600701905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886185025a03f19350505050505b565b5b5600a165627a7a723058206dd48a1be1f30e54f5105f13673a3fff6be78a63bb148924dd62a145b43694440029")),
    /*
        contract Sender3 {
            address sender1;
            address sender2;
            
            function Sender3() {
            }
            function setSenders(address senderx, address sendery) public{
                sender1 = senderx;
                sender2 = sendery;
            }
            function share() public payable{
                sender1.call.value(msg.value/2)(bytes4(sha3("share()")));
                sender2.call.value(msg.value/4)(bytes4(sha3("keep()")));
            }
            function withdraw() public{
                msg.sender.send(this.balance);
            }
            function() payable { } //always payable
        }
    */
    valtype(ParseHex("606060405234610000575b5b5b6103968061001b6000396000f30060606040523615610055576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680633ccfd60b1461005e5780635579818d1461006d578063a8d5fd65146100bf575b61005c5b5b565b005b346100005761006b6100c9565b005b34610000576100bd600480803573ffffffffffffffffffffffffffffffffffffffff1690602001909190803573ffffffffffffffffffffffffffffffffffffffff1690602001909190505061011c565b005b6100c76101a3565b005b3373ffffffffffffffffffffffffffffffffffffffff166108fc3073ffffffffffffffffffffffffffffffffffffffff16319081150290604051809050600060405180830381858888f19350505050505b565b81600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555080600160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b5050565b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff166002348115610000570460405180807f7368617265282900000000000000000000000000000000000000000000000000815250600701905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886185025a03f1935050505050600160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff166004348115610000570460405180807f6b65657028290000000000000000000000000000000000000000000000000000815250600601905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886185025a03f19350505050505b5600a165627a7a723058208fddda1b1d980244617e9e88336ac5c0d3e1a836d77985d899cd6d96058106260029")),
    /*call Sender1 setSender*/
    valtype(ParseHex("5579818d00000000000000000000000000a64bc3531cd43517a1eee783245effd6770f48000000000000000000000000ca1d76da7e66c5db9459f098e7e6a09381eef2b5")),
    /*call Sender2 setSender*/
    valtype(ParseHex("5579818d00000000000000000000000047b725b087f9ef7802b4fef599cfeb08a451e46f000000000000000000000000ca1d76da7e66c5db9459f098e7e6a09381eef2b5")),
    /*call Sender3 setSender*/
    valtype(ParseHex("5579818d00000000000000000000000047b725b087f9ef7802b4fef599cfeb08a451e46f00000000000000000000000000a64bc3531cd43517a1eee783245effd6770f48")),
    /*share*/
    valtype(ParseHex("a8d5fd65")),
    /*keep*/
    valtype(ParseHex("e4d06d82")),
    /*sendAll*/
    valtype(ParseHex("e14f680f")),
    /*withdrawAll*/
    valtype(ParseHex("853828b6")),
    /*
        contract Test1 {
            address Sender1 = 0x47b725b087f9ef7802b4fef599cfeb08a451e46f;
            address Sender2 = 0x00a64bc3531cd43517a1eee783245effd6770f48;
            address Sender3 = 0xca1d76da7e66c5db9459f098e7e6a09381eef2b5;
            
            function transfer() {
                Sender1.send(this.balance/3);
                Sender2.send(this.balance/2);
                Sender3.send(this.balance);
            }
            
            function() payable { }
        }
    */
    valtype(ParseHex("60606040527347b725b087f9ef7802b4fef599cfeb08a451e46f600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055507300a64bc3531cd43517a1eee783245effd6770f48600160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555073ca1d76da7e66c5db9459f098e7e6a09381eef2b5600260006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555034610000575b6101ee806101186000396000f3006060604052361561003f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638a4068dd14610048575b6100465b5b565b005b3461000057610055610057565b005b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff166108fc60033073ffffffffffffffffffffffffffffffffffffffff1631811561000057049081150290604051809050600060405180830381858888f1935050505050600160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff166108fc60023073ffffffffffffffffffffffffffffffffffffffff1631811561000057049081150290604051809050600060405180830381858888f1935050505050600260009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff166108fc3073ffffffffffffffffffffffffffffffffffffffff16319081150290604051809050600060405180830381858888f19350505050505b5600a165627a7a72305820a16c66241bc68fd2da2088dfbf7847f57fda4273502d7b817499d4bb32d41f1b0029")),
    /*transfer*/
    valtype(ParseHex("8a4068dd")),
    /*
        contract Test11{
            function() payable { }
        }
    */
    valtype(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a7230582089f5187aa25f85528a07c69078180c9616660442c881510ebc3534a29011b49e0029")),
    /*
        contract Test12{
            address addr = 0xcf8c04c7c0a68f7483ce660df9e3056d05347d5c;
            function transfer() payable {
                addr.call.value(this.balance/2)(bytes4(sha3("00")));
            }
            function() payable { }
        }
    */
    valtype(ParseHex("606060405273cf8c04c7c0a68f7483ce660df9e3056d05347d5c600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550341561006157fe5b5b61017d806100716000396000f3006060604052361561003f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638a4068dd14610048575b6100465b5b565b005b610050610052565b005b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1660023073ffffffffffffffffffffffffffffffffffffffff16318115156100ae57fe5b0460405180807f3030000000000000000000000000000000000000000000000000000000000000815250600201905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886187965a03f19350505050505b5600a165627a7a723058207f49346fdf78e94fe907fa5c0cc37c1bc5e86a5f91ef2811fe70e302cab979ee0029")),
    /*
        contract Test13{
            address addr = 0xef1bada115ec9dcc117ca8d395f649fee774498c;
            function transfer() payable {
                addr.call.value(this.balance/2)(bytes4(sha3("transfer()")));
            }
            function() payable { }
        }
    */
    valtype(ParseHex("606060405273ef1bada115ec9dcc117ca8d395f649fee774498c600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550341561006157fe5b5b61017d806100716000396000f3006060604052361561003f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638a4068dd14610048575b6100465b5b565b005b610050610052565b005b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1660023073ffffffffffffffffffffffffffffffffffffffff16318115156100ae57fe5b0460405180807f7472616e73666572282900000000000000000000000000000000000000000000815250600a01905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886187965a03f19350505050505b5600a165627a7a72305820b568c0caeeeb3a6af971f764ca36556567fb91dd3f5beac33dbe0a18d7902dc80029")),
    /*
        contract Test14{
            address addr = 0xbf280ab611c4866bf3d4721d834633e5e615f6c7;
            function transfer() payable {
                addr.call.value(this.balance/2)(bytes4(sha3("transfer()")));
            }
            function() payable { }
        }
    */
    valtype(ParseHex("606060405273bf280ab611c4866bf3d4721d834633e5e615f6c7600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550341561006157fe5b5b61017d806100716000396000f3006060604052361561003f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638a4068dd14610048575b6100465b5b565b005b610050610052565b005b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1660023073ffffffffffffffffffffffffffffffffffffffff16318115156100ae57fe5b0460405180807f7472616e73666572282900000000000000000000000000000000000000000000815250600a01905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886187965a03f19350505050505b5600a165627a7a72305820daeb96d07fc42ad749f5f2c00724a6b1818986262e8aa37572c35ef7fb1260c60029")),
    /*
        contract Test15{
            address addr = 0xefc304b02e965bf2fc34654a9b6c35e587aebb55;
            function transfer() payable {
                addr.call.value(this.balance/2)(bytes4(sha3("transfer()")));
            }
            function() payable { }
        }
    */
    valtype(ParseHex("606060405273efc304b02e965bf2fc34654a9b6c35e587aebb55600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550341561006157fe5b5b61017d806100716000396000f3006060604052361561003f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638a4068dd14610048575b6100465b5b565b005b610050610052565b005b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1660023073ffffffffffffffffffffffffffffffffffffffff16318115156100ae57fe5b0460405180807f7472616e73666572282900000000000000000000000000000000000000000000815250600a01905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886187965a03f19350505050505b5600a165627a7a723058203289bd2de8b19d77e8d408d2a3f9bf60d53dda62c2aa77dec4e6d982c7aca30b0029")),
    /*
        contract SuicideTest{
            address addr = 0x47b725b087f9ef7802b4fef599cfeb08a451e46f;
            function SuicideTest() payable {}
            function sui() payable {
                suicide(addr);
            }
            function() payable { }
        }
    */
    valtype(ParseHex("60606040527347b725b087f9ef7802b4fef599cfeb08a451e46f600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b5b5b60b68061006a6000396000f30060606040523615603d576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff168063c421249a146045575b60435b5b565b005b604b604d565b005b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16ff5b5600a165627a7a72305820e3bd6e50ab35c5105478fab7852d2307ea9c3adefc32ae8da2ec3e72dd791aed0029")),
    /*sui*/
    valtype(ParseHex("c421249a")),
    /*
        contract Test{
            address addr = 0x47b725b087f9ef7802b4fef599cfeb08a451e46f;
            function transfer() payable {
                addr.call.value(this.balance/2)(bytes4(sha3("transfer()")));
            }
            function Test() payable{}
            
            function() payable {}
        }
    */
    valtype(ParseHex("60606040527347b725b087f9ef7802b4fef599cfeb08a451e46f600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b5b5b61017a8061006b6000396000f3006060604052361561003f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638a4068dd14610048575b6100465b5b565b005b610050610052565b005b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1660023073ffffffffffffffffffffffffffffffffffffffff16318115610000570460405180807f7472616e73666572282900000000000000000000000000000000000000000000815250600a01905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886185025a03f19350505050505b5600a165627a7a72305820709abc77d99f7e829396b41fcf78a6d4444b9f9734ea765177bd11cbd7357e960029"))
};

dev::h256 hash = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

void checkRes(ByteCodeExecResult& res, std::vector<dev::Address>& addresses, std::vector<dev::u256>& balances, size_t sizeTxs){
    std::unordered_map<dev::Address, Vin> vins = globalState->vins();
    BOOST_CHECK(res.valueTransfers.size() == sizeTxs);
    for(size_t i = 0; i < addresses.size(); i++){
        BOOST_CHECK(globalState->balance(addresses[i]) == balances[i]);
        if(balances[i] > 0){
            BOOST_CHECK(vins.count(addresses[i]));
            BOOST_CHECK(vins[addresses[i]].value = balances[i]);
        } else {
            BOOST_CHECK(!vins.count(addresses[i]));
        }
    }
}

void checkTx(CTransaction& tx, size_t sizeVin, size_t sizeVout, std::vector<CAmount> values){
    BOOST_CHECK(tx.vin.size() == sizeVin);
    BOOST_CHECK(tx.vout.size() == sizeVout);

    for(size_t i = 0; i < tx.vout.size(); i++){
        BOOST_CHECK(tx.vout[i].nValue == values[i]);
    }
}

BOOST_FIXTURE_TEST_SUITE(condensingtransaction_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(condensingtransactionbehavior_tests){
    initState();
    dev::h256 hashTemp(hash);

    std::vector<QtumTransaction> txs;
    std::vector<dev::Address> addresses;
    for(size_t i = 0; i < 3; i++){
        txs.push_back(createQtumTransaction(code[i], 0, dev::u256(500000), dev::u256(1), hashTemp, dev::Address(), i));
        addresses.push_back(createQtumAddress(hashTemp, i));
        ++hashTemp;
    }
    auto result = executeBC(txs);
    std::vector<dev::u256> balances = {0,0,0};
    checkRes(result.second, addresses, balances, 0);

    txs.clear();
    for(size_t i = 0; i < 3; i++){
        txs.push_back(createQtumTransaction(code[i + 3], 0, dev::u256(500000), dev::u256(1), hashTemp, addresses[i]));
    }
    result = executeBC(txs);
    balances = {0,0,0};
    checkRes(result.second, addresses, balances, 0);

    txs.clear();
    txs.push_back(createQtumTransaction(code[6], 8000, dev::u256(500000), dev::u256(1), hashTemp, addresses[0]));
    result = executeBC(txs);
    balances = {5000,2500,500};
    checkRes(result.second, addresses, balances, 1);
    checkTx(result.second.valueTransfers[0], 1, 3, {2500,5000,500});

    txs.clear();
    txs.push_back(createQtumTransaction(code[7], 2000, dev::u256(500000), dev::u256(1), hashTemp, addresses[0]));
    txs.push_back(createQtumTransaction(code[8], 2000, dev::u256(500000), dev::u256(1), hashTemp, addresses[0]));
    result = executeBC(txs);
    balances = {0,11500,500};
    checkRes(result.second, addresses, balances, 2);
    checkTx(result.second.valueTransfers[0], 2, 1, {7000});
    checkTx(result.second.valueTransfers[1], 3, 1, {11500});

    txs.clear();
    txs.push_back(createQtumTransaction(code[6], 2000, dev::u256(30000), dev::u256(1), hashTemp, addresses[1]));
    txs.push_back(createQtumTransaction(code[9], 0, dev::u256(500000), dev::u256(1), hashTemp, addresses[1]));
    result = executeBC(txs);
    balances = {0,0,0};
    checkRes(result.second, addresses, balances, 2);
    checkTx(result.second.valueTransfers[0], 1, 1, {2000});
    checkTx(result.second.valueTransfers[1], 2, 1, {12000});
}

BOOST_AUTO_TEST_CASE(condensingtransactionbreadthways_tests){
    initState();
    dev::h256 hashTemp(hash);
    std::vector<dev::Address> addresses;
    std::vector<QtumTransaction> txs;
    for(size_t i = 0; i < 3; i++){
        txs.push_back(createQtumTransaction(code[i], 0, dev::u256(500000), dev::u256(1), hashTemp, dev::Address(), i));
        addresses.push_back(createQtumAddress(hashTemp, i));
        ++hashTemp;
    }
    txs.push_back(createQtumTransaction(code[10], 0, dev::u256(500000), dev::u256(1), hashTemp, dev::Address(), 4));
    addresses.push_back(createQtumAddress(hashTemp, 4));
    auto result = executeBC(txs);

    txs.clear();
    txs.push_back(createQtumTransaction(code[0], 15000, dev::u256(500000), dev::u256(1), hashTemp, addresses[3]));
    txs.push_back(createQtumTransaction(code[11], 0, dev::u256(500000), dev::u256(1), hashTemp, addresses[3]));

    result = executeBC(txs);
    std::vector<dev::u256> balances = {5000,5000,5000,0};
    checkRes(result.second, addresses, balances, 2);
    checkTx(result.second.valueTransfers[0], 1, 1, {15000});
    checkTx(result.second.valueTransfers[1], 1, 3, {5000,5000,5000});
}

BOOST_AUTO_TEST_CASE(condensingtransactiondeep_tests){
    initState();
    dev::h256 hashTemp(hash);
    std::vector<dev::Address> addresses;
    std::vector<QtumTransaction> txs;
    for(size_t i = 12; i < 17; i++){
        txs.push_back(createQtumTransaction(code[i], 0, dev::u256(500000), dev::u256(1), hashTemp, dev::Address(), i));
        addresses.push_back(createQtumAddress(hashTemp, i));
        ++hashTemp;
    }
    auto result = executeBC(txs);
    txs.clear();
    txs.push_back(createQtumTransaction(code[11], 20000, dev::u256(500000), dev::u256(1), hashTemp, addresses[4]));
    result = executeBC(txs);
    std::vector<dev::u256> balances = {1250,1250,2500,5000,10000};
    checkRes(result.second, addresses, balances, 1);
    checkTx(result.second.valueTransfers[0], 1, 5, {10000,2500,1250,1250,5000});
}

BOOST_AUTO_TEST_CASE(condensingtransactionsuicide_tests){
    initState();
    dev::h256 hashTemp(hash);
    std::vector<dev::Address> addresses;
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(code[12], 0, dev::u256(500000), dev::u256(1), hashTemp, dev::Address(), 0));
    addresses.push_back(createQtumAddress(hashTemp, 0));
    
    txs.push_back(createQtumTransaction(code[17], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address(), 1));
    txs.push_back(createQtumTransaction(valtype(), 13000, dev::u256(500000), dev::u256(1), hashTemp, createQtumAddress(hashTemp, 1), 1));

    addresses.push_back(createQtumAddress(hashTemp, 1));
    auto result = executeBC(txs);

    txs.clear();
    txs.push_back(createQtumTransaction(code[18], 0, dev::u256(500000), dev::u256(1), hashTemp, addresses[1]));
    result = executeBC(txs);
    std::vector<dev::u256> balances = {13000,0};
    checkRes(result.second, addresses, balances, 1);
    checkTx(result.second.valueTransfers[0], 1, 1, {13000});
}

BOOST_AUTO_TEST_CASE(condensingtransactionpaytopubkeyhash_tests){
    initState();
    dev::h256 hashTemp(hash);
    std::vector<dev::Address> addresses;
    std::vector<QtumTransaction> txs;

    txs.push_back(createQtumTransaction(code[19], 0, dev::u256(500000), dev::u256(1), hashTemp, dev::Address(), 13));
    txs.push_back(createQtumTransaction(valtype(), 13000, dev::u256(500000), dev::u256(1), hashTemp, createQtumAddress(hashTemp, 13), 13));

    addresses.push_back(createQtumAddress(hashTemp, 13));
    auto result = executeBC(txs);

    txs.clear();
    txs.push_back(createQtumTransaction(code[11], 0, dev::u256(500000), dev::u256(1), hashTemp, addresses[0]));
    result = executeBC(txs);
    std::vector<dev::u256> balances = {6500,6500};
    checkRes(result.second, addresses, balances, 1);
    checkTx(result.second.valueTransfers[0], 1, 2, {6500,6500});
    BOOST_CHECK(result.second.valueTransfers[0].vout[0].scriptPubKey.IsPayToPubkeyHash());
    BOOST_CHECK(result.second.valueTransfers[0].vout[1].scriptPubKey.HasOpCall());
}

BOOST_AUTO_TEST_SUITE_END()
