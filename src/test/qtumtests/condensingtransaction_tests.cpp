#include <boost/filesystem/operations.hpp>
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
    valtype(ParseHex("5579818d0000000000000000000000008cd673a7e9322cb860bb3b743820921661c7e2a7000000000000000000000000edc7cea5c45f9b8804b2383cc410c7e08072d126")),
    /*call Sender2 setSender*/
    valtype(ParseHex("5579818d0000000000000000000000004de45add9f5f0b6887081cfcfe3aca6da9eb3365000000000000000000000000edc7cea5c45f9b8804b2383cc410c7e08072d126")),
    /*call Sender3 setSender*/
    valtype(ParseHex("5579818d0000000000000000000000004de45add9f5f0b6887081cfcfe3aca6da9eb33650000000000000000000000008cd673a7e9322cb860bb3b743820921661c7e2a7")),
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
            address Sender1 = 0x4de45add9f5f0b6887081cfcfe3aca6da9eb3365;
            address Sender2 = 0x8cd673a7e9322cb860bb3b743820921661c7e2a7;
            address Sender3 = 0xedc7cea5c45f9b8804b2383cc410c7e08072d126;
            
            function transfer() {
                Sender1.send(this.balance/3);
                Sender2.send(this.balance/2);
                Sender3.send(this.balance);
            }
            
            function() payable { }
        }
    */
    valtype(ParseHex("6060604052734de45add9f5f0b6887081cfcfe3aca6da9eb3365600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550738cd673a7e9322cb860bb3b743820921661c7e2a7600160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555073edc7cea5c45f9b8804b2383cc410c7e08072d126600260006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555034610000575b6101ee806101186000396000f3006060604052361561003f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638a4068dd14610048575b6100465b5b565b005b3461000057610055610057565b005b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff166108fc60033073ffffffffffffffffffffffffffffffffffffffff1631811561000057049081150290604051809050600060405180830381858888f1935050505050600160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff166108fc60023073ffffffffffffffffffffffffffffffffffffffff1631811561000057049081150290604051809050600060405180830381858888f1935050505050600260009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff166108fc3073ffffffffffffffffffffffffffffffffffffffff16319081150290604051809050600060405180830381858888f19350505050505b5600a165627a7a72305820a16c66241bc68fd2da2088dfbf7847f57fda4273502d7b817499d4bb32d41f1b0029")),
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
            address addr = 0x747a802725849fbcf3a2ccd90e25685a4a244ab0;
            function transfer() payable {
                addr.call.value(this.balance/2)(bytes4(sha3("00")));
            }
            function() payable { }
        }
    */
    valtype(ParseHex("606060405273747a802725849fbcf3a2ccd90e25685a4a244ab0600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555034610000575b61017a8061006e6000396000f3006060604052361561003f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638a4068dd14610048575b6100465b5b565b005b610050610052565b005b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1660023073ffffffffffffffffffffffffffffffffffffffff16318115610000570460405180807f3030000000000000000000000000000000000000000000000000000000000000815250600201905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886185025a03f19350505050505b5600a165627a7a723058209534adcc5831b13598cf32a78ded12b023f66269c90a45c87c1f36d9241594580029")),
    /*
        contract Test13{
            address addr = 0x7e976c98b7354c2972848031d955f628a29509dc;
            function transfer() payable {
                addr.call.value(this.balance/2)(bytes4(sha3("transfer()")));
            }
            function() payable { }
        }
    */
    valtype(ParseHex("6060604052737e976c98b7354c2972848031d955f628a29509dc600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555034610000575b61017a8061006e6000396000f3006060604052361561003f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638a4068dd14610048575b6100465b5b565b005b610050610052565b005b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1660023073ffffffffffffffffffffffffffffffffffffffff16318115610000570460405180807f7472616e73666572282900000000000000000000000000000000000000000000815250600a01905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886185025a03f19350505050505b5600a165627a7a7230582046f96a0ac3f73afc70f71bcc07cdbccabe47ae51ee394a2cfab75f00d2f157eb0029")),
    /*
        contract Test14{
            address addr = 0x2ea90ccb41921cf63b01d0f44a1a3082038d6494;
            function transfer() payable {
                addr.call.value(this.balance/2)(bytes4(sha3("transfer()")));
            }
            function() payable { }
        }
    */
    valtype(ParseHex("6060604052732ea90ccb41921cf63b01d0f44a1a3082038d6494600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555034610000575b61017a8061006e6000396000f3006060604052361561003f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638a4068dd14610048575b6100465b5b565b005b610050610052565b005b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1660023073ffffffffffffffffffffffffffffffffffffffff16318115610000570460405180807f7472616e73666572282900000000000000000000000000000000000000000000815250600a01905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886185025a03f19350505050505b5600a165627a7a72305820a13c5ddcd1ac647b97968133a5d983664fd76ba48cfd38ab19e259ab1288eb330029")),
    /*
        contract Test15{
            address addr = 0x78ec6322824c9378136937e02cecde3fd36e1e18;
            function transfer() payable {
                addr.call.value(this.balance/2)(bytes4(sha3("transfer()")));
            }
            function() payable { }
        }
    */
    valtype(ParseHex("60606040527378ec6322824c9378136937e02cecde3fd36e1e18600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555034610000575b61017a8061006e6000396000f3006060604052361561003f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638a4068dd14610048575b6100465b5b565b005b610050610052565b005b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1660023073ffffffffffffffffffffffffffffffffffffffff16318115610000570460405180807f7472616e73666572282900000000000000000000000000000000000000000000815250600a01905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886185025a03f19350505050505b5600a165627a7a72305820bbfbc38327968645d3f9520195b9ac616ae17bf099046cac79d605032375956f0029")),
    /*
        contract SuicideTest{
            address addr = 0x4de45add9f5f0b6887081cfcfe3aca6da9eb3365;
            function SuicideTest() payable {}
            function sui() payable {
                suicide(addr);
            }
            function() payable { }
        }
    */
    valtype(ParseHex("6060604052734de45add9f5f0b6887081cfcfe3aca6da9eb3365600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b5b5b60b68061006a6000396000f30060606040523615603d576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff168063c421249a146045575b60435b5b565b005b604b604d565b005b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16ff5b5600a165627a7a72305820e3bd6e50ab35c5105478fab7852d2307ea9c3adefc32ae8da2ec3e72dd791aed0029")),
    /*sui*/
    valtype(ParseHex("c421249a")),
    /*
        contract Test{
            address addr = 0x4de45add9f5f0b6887081cfcfe3aca6da9eb3365;
            function transfer() payable {
                addr.call.value(this.balance/2)(bytes4(sha3("transfer()")));
            }
            function Test() payable{}
        }
    */
    valtype(ParseHex("6060604052734de45add9f5f0b6887081cfcfe3aca6da9eb3365600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b5b5b6101708061006b6000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638a4068dd1461003e575b610000565b610046610048565b005b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1660023073ffffffffffffffffffffffffffffffffffffffff16318115610000570460405180807f7472616e73666572282900000000000000000000000000000000000000000000815250600a01905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886185025a03f19350505050505b5600a165627a7a723058208624b9ba0441e9d6e1e2a655d0384c4cd65cc76928dcff6eab562722140805650029"))
};

dev::h256 hash = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

void checkRes(ByteCodeExecResult& res, std::vector<dev::Address>& addresses, std::vector<dev::u256>& balances, size_t sizeTxs){
    std::unordered_map<dev::Address, Vin> vins = globalState->vins();
    BOOST_CHECK(res.refundValueTx.size() == sizeTxs);
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
    checkTx(result.second.refundValueTx[0], 1, 3, {5000,2500,500});

    txs.clear();
    txs.push_back(createQtumTransaction(code[7], 2000, dev::u256(500000), dev::u256(1), hashTemp, addresses[0]));
    txs.push_back(createQtumTransaction(code[8], 2000, dev::u256(500000), dev::u256(1), hashTemp, addresses[0]));
    result = executeBC(txs);
    balances = {0,11500,500};
    checkRes(result.second, addresses, balances, 2);
    checkTx(result.second.refundValueTx[0], 2, 1, {7000});
    checkTx(result.second.refundValueTx[1], 3, 1, {11500});

    txs.clear();
    txs.push_back(createQtumTransaction(code[6], 2000, dev::u256(30000), dev::u256(1), hashTemp, addresses[1]));
    txs.push_back(createQtumTransaction(code[9], 0, dev::u256(500000), dev::u256(1), hashTemp, addresses[1]));
    result = executeBC(txs);
    balances = {0,0,0};
    checkRes(result.second, addresses, balances, 2);
    checkTx(result.second.refundValueTx[0], 1, 1, {2000});
    checkTx(result.second.refundValueTx[1], 2, 1, {12000});
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
    checkTx(result.second.refundValueTx[0], 1, 1, {15000});
    checkTx(result.second.refundValueTx[1], 1, 3, {5000,5000,5000});
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
    checkTx(result.second.refundValueTx[0], 1, 5, {2500,1250,5000,1250,10000});
}

BOOST_AUTO_TEST_CASE(condensingtransactionsuicide_tests){
    initState();
    dev::h256 hashTemp(hash);
    std::vector<dev::Address> addresses;
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(code[12], 0, dev::u256(500000), dev::u256(1), hashTemp, dev::Address(), 0));
    addresses.push_back(createQtumAddress(hashTemp, 0));
    txs.push_back(createQtumTransaction(code[17], 13000, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address(), 1));
    addresses.push_back(createQtumAddress(hashTemp, 1));
    auto result = executeBC(txs);

    txs.clear();
    txs.push_back(createQtumTransaction(code[18], 0, dev::u256(500000), dev::u256(1), hashTemp, addresses[1]));
    result = executeBC(txs);
    std::vector<dev::u256> balances = {13000,0};
    checkRes(result.second, addresses, balances, 1);
    checkTx(result.second.refundValueTx[0], 1, 1, {13000});
}

BOOST_AUTO_TEST_CASE(condensingtransactionpaytopubkeyhash_tests){
    initState();
    dev::h256 hashTemp(hash);
    std::vector<dev::Address> addresses;
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(code[19], 13000, dev::u256(500000), dev::u256(1), hashTemp, dev::Address(), 13));
    addresses.push_back(createQtumAddress(hashTemp, 13));
    auto result = executeBC(txs);

    txs.clear();
    txs.push_back(createQtumTransaction(code[11], 0, dev::u256(500000), dev::u256(1), hashTemp, addresses[0]));
    result = executeBC(txs);
    std::vector<dev::u256> balances = {6500,6500};
    checkRes(result.second, addresses, balances, 1);
    checkTx(result.second.refundValueTx[0], 1, 2, {6500,6500});
    BOOST_CHECK(result.second.refundValueTx[0].vout[0].scriptPubKey.HasOpCall());
    BOOST_CHECK(result.second.refundValueTx[0].vout[1].scriptPubKey.IsPayToPubkeyHash());
}

BOOST_AUTO_TEST_SUITE_END()
