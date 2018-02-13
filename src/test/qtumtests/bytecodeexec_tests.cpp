#include <boost/test/unit_test.hpp>
#include <test/test_bitcoin.h>
#include <qtumtests/test_utils.h>

dev::u256 GASLIMIT = dev::u256(500000);
dev::Address SENDERADDRESS = dev::Address("0101010101010101010101010101010101010101");
dev::h256 HASHTX = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
std::vector<valtype> CODE = 
{ 
    /*
        contract Temp {
            function () payable {}
        }
    */
    valtype(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058209cedb722bf57a30e3eb00eeefc392103ea791a2001deed29f5c3809ff10eb1dd0029")),
    /*
        contract Temp {
            function Temp(){
                while(true){
                    
                }
            }
            function () payable {}
        }
    */
    valtype(ParseHex("6060604052346000575b5b600115601457600a565b5b5b60398060236000396000f30060606040525b600b5b5b565b0000a165627a7a7230582036c45484ccdd0a2e017c3d6842a4cf345f28abbb1eda4f27c4c26f2f8cf4dfe20029")),
    /*
        contract Temp {
            function () payable {
                while(true){
                    
                }
            }
        }
    */
    valtype(ParseHex("6060604052346000575b60448060166000396000f30060606040525b60165b5b6001156013576009565b5b565b0000a165627a7a723058209aa6fe3625c7e2eac43ccddf503a8ab61af91a4fc757f5eac78b02412eb3c91b0029")),
    /*
        contract Factory {
            bytes32[] Names;
            address[] newContracts;

            function createContract (bytes32 name) {
                address newContract = new Contract(name);
                newContracts.push(newContract);
            } 

            function getName (uint i) {
                Contract con = Contract(newContracts[i]);
                Names[i] = con.Name();
            }
        }

        contract Contract {
            bytes32 public Name;

            function Contract (bytes32 name) {
                Name = name;
            }
            
            function () payable {}
        }
    */
    valtype(ParseHex("606060405234610000575b61034a806100196000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680633f811b80146100495780636b8ff5741461006a575b610000565b3461000057610068600480803560001916906020019091905050610087565b005b3461000057610085600480803590602001909190505061015b565b005b60008160405160e18061023e833901808260001916600019168152602001915050604051809103906000f08015610000579050600180548060010182818154818355818115116101035781836000526020600020918201910161010291905b808211156100fe5760008160009055506001016100e6565b5090565b5b505050916000526020600020900160005b83909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550505b5050565b6000600182815481101561000057906000526020600020900160005b9054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690508073ffffffffffffffffffffffffffffffffffffffff16638052474d6000604051602001526040518163ffffffff167c0100000000000000000000000000000000000000000000000000000000028152600401809050602060405180830381600087803b156100005760325a03f1156100005750505060405180519050600083815481101561000057906000526020600020900160005b5081600019169055505b50505600606060405234610000576040516020806100e1833981016040528080519060200190919050505b80600081600019169055505b505b609f806100426000396000f30060606040523615603d576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638052474d146045575b60435b5b565b005b34600057604f606d565b60405180826000191660001916815260200191505060405180910390f35b600054815600a165627a7a72305820fe28ec2b77f3b306095bda73561b85d147a1026db2e5714aeeb2f29246cffcbb0029a165627a7a7230582086cf938db13cf2aa8bca8ad6e720861683ef2cc971ad66dad68708438a5e4a9b0029")),
    /*
        contract sui {
            address addr = 0x382f0a81f70a2c43e652c353caf15494d1b57fae;
            
            function sui() payable {}

            function kill() payable {
                suicide(addr);
            }
            
            function () payable {}
        }
    */
    valtype(ParseHex("6060604052734de45add9f5f0b6887081cfcfe3aca6da9eb3365600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b5b5b60b68061006a6000396000f30060606040523615603d576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806341c0e1b5146045575b60435b5b565b005b604b604d565b005b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16ff5b5600a165627a7a72305820e296f585c72ea3d4dce6880122cfe387d26c48b7960676a52e811b56ef8297a80029"))
};

void checkExecResult(std::vector<ResultExecute>& result, size_t execResSize, size_t addressesSize, 
                     dev::eth::TransactionException except, std::vector<dev::Address> newAddresses, 
                     valtype output, dev::u256 balance, bool normalAndIncorrect = false){
    std::unordered_map<dev::Address, dev::u256> addresses = globalState->addresses();
    BOOST_CHECK(result.size() == execResSize);
    BOOST_CHECK(addresses.size() == addressesSize);
    for(size_t i = 0; i < result.size(); i++){
        if(normalAndIncorrect){
            if(i%2 == 0){
                BOOST_CHECK(result[i].execRes.excepted == dev::eth::TransactionException::None);
                BOOST_CHECK(result[i].execRes.newAddress == newAddresses[i]);
                BOOST_CHECK(result[i].execRes.output == output);
            } else {
                BOOST_CHECK(result[i].execRes.excepted == except);
                BOOST_CHECK(result[i].execRes.newAddress == dev::Address());
                BOOST_CHECK(result[i].execRes.output == valtype());
            }
        } else {
            BOOST_CHECK(result[i].execRes.excepted == except);
            BOOST_CHECK(result[i].execRes.newAddress == newAddresses[i]);
            BOOST_CHECK(result[i].execRes.output == output);
        }
        BOOST_CHECK(globalState->balance(newAddresses[i]) == balance);
    }
}

void checkBCEResult(ByteCodeExecResult result, uint64_t usedGas, CAmount refundSender, size_t nVouts, uint64_t sum, size_t nTxs = 0){
    BOOST_CHECK(result.usedGas + result.refundSender == sum);
    BOOST_CHECK(result.usedGas == usedGas);
    BOOST_CHECK(result.refundSender == refundSender);
    BOOST_CHECK(result.refundOutputs.size() == nVouts);
    for(size_t i = 0; i < result.refundOutputs.size(); i++){
        BOOST_CHECK(result.refundOutputs[i].scriptPubKey == CScript() << OP_DUP << OP_HASH160 << SENDERADDRESS.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
    }
    BOOST_CHECK(result.valueTransfers.size() == nTxs);
}

BOOST_FIXTURE_TEST_SUITE(bytecodeexec_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(bytecodeexec_txs_empty){
    initState();
    std::vector<QtumTransaction> txs;
    auto result = executeBC(txs);
    BOOST_CHECK(result.first.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_create_contract){
    initState();
    QtumTransaction txEth = createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address());
    std::vector<QtumTransaction> txs(1, txEth);
    auto result = executeBC(txs);
    
    std::vector<dev::Address> addrs = {createQtumAddress(txs[0].getHashWith(), txs[0].getNVout())};
    valtype code = ParseHex("60606040525b600b5b5b565b0000a165627a7a723058209cedb722bf57a30e3eb00eeefc392103ea791a2001deed29f5c3809ff10eb1dd0029");
    checkExecResult(result.first, 1, 1, dev::eth::TransactionException::None, addrs, code, dev::u256(0));
    checkBCEResult(result.second, 69382, 430618, 1, CAmount(GASLIMIT));
}

BOOST_AUTO_TEST_CASE(bytecodeexec_create_contract_OutOfGasBase){
    initState();
    QtumTransaction txEth = createQtumTransaction(CODE[0], 0, dev::u256(100), dev::u256(1), HASHTX, dev::Address());
    std::vector<QtumTransaction> txs(1, txEth);
    auto result = executeBC(txs);

    std::vector<dev::Address> addrs = {dev::Address()};
    checkExecResult(result.first, 1, 0, dev::eth::TransactionException::OutOfGasBase, addrs, valtype(), dev::u256(0));
    checkBCEResult(result.second, 100, 0, 0, 100);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_create_contract_OutOfGas){
    initState();
    QtumTransaction txEth = createQtumTransaction(CODE[1], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address());
    std::vector<QtumTransaction> txs(1, txEth);
    auto result = executeBC(txs);

    std::vector<dev::Address> addrs = {dev::Address()};
    checkExecResult(result.first, 1, 0, dev::eth::TransactionException::OutOfGas, addrs, valtype(), dev::u256(0));
    checkBCEResult(result.second, CAmount(GASLIMIT), 0, 0, CAmount(GASLIMIT));
}

BOOST_AUTO_TEST_CASE(bytecodeexec_OutOfGasBase_create_contract_normal_create_contract){
    initState();
    std::vector<dev::Address> newAddressGen;
    std::vector<QtumTransaction> txs;
    dev::h256 hash(HASHTX);
    for(size_t i = 0; i < 10; i++){
        dev::u256 gasLimit = i%2 == 0 ? GASLIMIT : dev::u256(100);
        QtumTransaction txEth = createQtumTransaction(CODE[0], 0, gasLimit, dev::u256(1), hash, dev::Address());
        newAddressGen.push_back(createQtumAddress(txEth.getHashWith(), txEth.getNVout()));
        txs.push_back(txEth);
        ++hash;
    }
    auto result = executeBC(txs);

    valtype code = ParseHex("60606040525b600b5b5b565b0000a165627a7a723058209cedb722bf57a30e3eb00eeefc392103ea791a2001deed29f5c3809ff10eb1dd0029");
    checkExecResult(result.first, 10, 5, dev::eth::TransactionException::OutOfGasBase, newAddressGen, code, dev::u256(0), true);
    checkBCEResult(result.second, 347410, 2153090, 5, CAmount(GASLIMIT * 5 + 500));
}

BOOST_AUTO_TEST_CASE(bytecodeexec_OutOfGas_create_contract_normal_create_contract){
    initState();
    std::vector<dev::Address> newAddressGen;
    std::vector<QtumTransaction> txs;
    dev::h256 hash(HASHTX);
    for(size_t i = 0; i < 10; i++){
        valtype code = i%2 == 0 ? CODE[0] : CODE[1];
        QtumTransaction txEth = createQtumTransaction(code, 0, GASLIMIT, dev::u256(1), hash, dev::Address());
        newAddressGen.push_back(createQtumAddress(txEth.getHashWith(), txEth.getNVout()));
        txs.push_back(txEth);
        ++hash;
    }
    auto result = executeBC(txs);
    
    valtype code = ParseHex("60606040525b600b5b5b565b0000a165627a7a723058209cedb722bf57a30e3eb00eeefc392103ea791a2001deed29f5c3809ff10eb1dd0029");
    checkExecResult(result.first, 10, 5, dev::eth::TransactionException::OutOfGas, newAddressGen, code, dev::u256(0), true);
    checkBCEResult(result.second, 2846910, 2153090, 5, CAmount(GASLIMIT * 10));
}

BOOST_AUTO_TEST_CASE(bytecodeexec_create_contract_many){
    initState();
    std::vector<dev::Address> newAddressGen;
    std::vector<QtumTransaction> txs;
    dev::h256 hash(HASHTX);
    for(size_t i = 0; i < 130; i++){
        QtumTransaction txEth = createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), hash, dev::Address(), i);
        newAddressGen.push_back(createQtumAddress(txEth.getHashWith(), txEth.getNVout()));
        txs.push_back(txEth);
        ++hash;
    }
    auto result = executeBC(txs);

    valtype code = ParseHex("60606040525b600b5b5b565b0000a165627a7a723058209cedb722bf57a30e3eb00eeefc392103ea791a2001deed29f5c3809ff10eb1dd0029");
    checkExecResult(result.first, 130, 130, dev::eth::TransactionException::None, newAddressGen, code, dev::u256(0));
    checkBCEResult(result.second, 9019660, 55980340, 130, CAmount(GASLIMIT * 130));
}

BOOST_AUTO_TEST_CASE(bytecodeexec_call_contract_transfer){
    initState();
    QtumTransaction txEthCreate = createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address());
    std::vector<QtumTransaction> txsCreate(1, txEthCreate);
    executeBC(txsCreate);
    std::vector<dev::Address> addrs = {createQtumAddress(txsCreate[0].getHashWith(), txsCreate[0].getNVout())};
    QtumTransaction txEthCall = createQtumTransaction(ParseHex("00"), 1300, GASLIMIT, dev::u256(1), HASHTX, addrs[0]);
    std::vector<QtumTransaction> txsCall(1, txEthCall);
    auto result = executeBC(txsCall);

    checkExecResult(result.first, 1, 1, dev::eth::TransactionException::None, addrs, valtype(), dev::u256(1300));
    checkBCEResult(result.second, 21037, 478963, 1, CAmount(GASLIMIT), 1);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_call_contract_transfer_OutOfGasBase_return_value){
    initState();
    QtumTransaction txEthCreate = createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address());
    std::vector<QtumTransaction> txsCreate(1, txEthCreate);
    executeBC(txsCreate);
    dev::Address newAddress(createQtumAddress(txsCreate[0].getHashWith(), txsCreate[0].getNVout()));
    QtumTransaction txEthCall = createQtumTransaction(ParseHex("00"), 1300, dev::u256(1), dev::u256(1), HASHTX, newAddress);
    std::vector<QtumTransaction> txsCall(1, txEthCall);
    auto result = executeBC(txsCall);

    std::vector<dev::Address> addrs = {txEthCall.receiveAddress()};
    checkExecResult(result.first, 1, 1, dev::eth::TransactionException::OutOfGasBase, addrs, valtype(), dev::u256(0));
    checkBCEResult(result.second, 1, 0, 0, 1, 1);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_call_contract_transfer_OutOfGas_return_value){
    initState();
    QtumTransaction txEthCreate = createQtumTransaction(CODE[2], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address());
    std::vector<QtumTransaction> txsCreate(1, txEthCreate);
    executeBC(txsCreate);
    dev::Address newAddress(createQtumAddress(txsCreate[0].getHashWith(), txsCreate[0].getNVout()));
    QtumTransaction txEthCall = createQtumTransaction(ParseHex("00"), 1300, GASLIMIT, dev::u256(1), HASHTX, newAddress);
    std::vector<QtumTransaction> txsCall(1, txEthCall);
    auto result = executeBC(txsCall);

    std::vector<dev::Address> addrs = {txEthCall.receiveAddress()};
    checkExecResult(result.first, 1, 1, dev::eth::TransactionException::OutOfGas, addrs, valtype(), dev::u256(0));
    checkBCEResult(result.second, CAmount(GASLIMIT), 0, 0, CAmount(GASLIMIT), 1);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_call_contract_transfer_many){
    initState();
    std::vector<dev::Address> newAddressGen;
    std::vector<QtumTransaction> txs;
    dev::h256 hash(HASHTX);
    for(size_t i = 0; i < 130; i++){
        QtumTransaction txEth = createQtumTransaction(CODE[0], 0, GASLIMIT, dev::u256(1), hash, dev::Address(), i);
        newAddressGen.push_back(createQtumAddress(txEth.getHashWith(), txEth.getNVout()));
        txs.push_back(txEth);
        ++hash;
    }
    executeBC(txs);
    std::vector<QtumTransaction> txsCall;
    std::vector<dev::Address> addrs;
    valtype codeCall(ParseHex("00"));
    dev::h256 hashCall(HASHTX);
    for(size_t i = 0; i < txs.size(); i++){
        QtumTransaction txEthCall = createQtumTransaction(codeCall, 1300, GASLIMIT, dev::u256(1), hash, newAddressGen[i], i);
        txsCall.push_back(txEthCall);
        addrs.push_back(txEthCall.receiveAddress());
        ++hashCall;
    }
    auto result = executeBC(txsCall);
    
    checkExecResult(result.first, 130, 130, dev::eth::TransactionException::None, addrs, valtype(), dev::u256(1300));
    checkBCEResult(result.second, 2734810, 62265190, 130, CAmount(GASLIMIT * 130), 130);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_call_contract_OutOfGas_transfer_many_return_value){
    initState();
    std::vector<dev::Address> newAddressGen;
    std::vector<QtumTransaction> txs;
    dev::h256 hash(HASHTX);
    for(size_t i = 0; i < 130; i++){
        QtumTransaction txEth = createQtumTransaction(CODE[2], 0, GASLIMIT, dev::u256(1), hash, dev::Address(), i);
        newAddressGen.push_back(createQtumAddress(txEth.getHashWith(), txEth.getNVout()));
        txs.push_back(txEth);
        ++hash;
    }
    executeBC(txs);
    std::vector<QtumTransaction> txsCall;
    std::vector<dev::Address> addrs;
    valtype codeCall(ParseHex("00"));
    dev::h256 hashCall(HASHTX);
    for(size_t i = 0; i < txs.size(); i++){
        QtumTransaction txEthCall = createQtumTransaction(codeCall, 1300, GASLIMIT, dev::u256(1), hashCall, newAddressGen[i], i);
        txsCall.push_back(txEthCall);
        addrs.push_back(txEthCall.receiveAddress());
        ++hashCall;
    }
    auto result = executeBC(txsCall);

    checkExecResult(result.first, 130, 130, dev::eth::TransactionException::OutOfGas, addrs, valtype(), dev::u256(0));
    checkBCEResult(result.second, CAmount(GASLIMIT) * 130, 0, 0, CAmount(GASLIMIT) * 130, 130);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_suicide){
    initState();
    std::vector<QtumTransaction> txsCreate;
    std::vector<dev::Address> newAddresses;
    dev::h256 hash(HASHTX);
    for(size_t i = 0; i < 10; i++){
        valtype code = i == 0 ? CODE[0] : CODE[4];
        dev::u256 value = i == 0 ? dev::u256(0) : dev::u256(13);
        QtumTransaction txEthCreate = createQtumTransaction(code, 0, GASLIMIT, dev::u256(1), hash, dev::Address(), i);
        txsCreate.push_back(txEthCreate);

        QtumTransaction txEthSendValue;
        if(i != 0){
            txEthSendValue = createQtumTransaction(valtype(), value, GASLIMIT, dev::u256(1), hash, createQtumAddress(txEthCreate.getHashWith(), txEthCreate.getNVout()), i);
            txsCreate.push_back(txEthSendValue);
        }

        newAddresses.push_back(createQtumAddress(txEthCreate.getHashWith(), txEthCreate.getNVout()));
        ++hash;
    }

    executeBC(txsCreate);

    std::vector<QtumTransaction> txsCall;
    std::vector<dev::Address> addrs;
    valtype codeCall(ParseHex("41c0e1b5"));
    for(size_t i = 0; i < newAddresses.size() - 1; i++){
        QtumTransaction txEthCall = createQtumTransaction(codeCall, 0, GASLIMIT, dev::u256(1), HASHTX, newAddresses[i + 1]);
        txsCall.push_back(txEthCall);
        addrs.push_back(txEthCall.receiveAddress());
    }
    auto result = executeBC(txsCall);

    checkExecResult(result.first, 9, 1, dev::eth::TransactionException::None, addrs, valtype(), dev::u256(0));
    checkBCEResult(result.second, 96588, 4403412, 9, CAmount(GASLIMIT * 9), 9);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_contract_create_contracts){
    initState();
    QtumTransaction txEthCreate = createQtumTransaction(CODE[3], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address());
    std::vector<QtumTransaction> txs(1, txEthCreate);
    executeBC(txs);
    std::vector<QtumTransaction> txsCall;
    valtype codeCall(ParseHex("3f811b80"));
    dev::Address newAddress(createQtumAddress(txEthCreate.getHashWith(), txEthCreate.getNVout()));
    std::vector<dev::Address> addrs;
    for(size_t i = 0; i < 20; i++){
        QtumTransaction txEthCall = createQtumTransaction(codeCall, 0, GASLIMIT, dev::u256(1), HASHTX, newAddress, i);
        txsCall.push_back(txEthCall);
        addrs.push_back(txEthCall.receiveAddress());
    }
    auto result = executeBC(txsCall);

    checkExecResult(result.first, 20, 21, dev::eth::TransactionException::None, addrs, valtype(), dev::u256(0));
    BOOST_CHECK(result.second.usedGas + result.second.refundSender == GASLIMIT * 20);
    BOOST_CHECK(result.second.usedGas == 2335520);
    BOOST_CHECK(result.second.refundSender == 7664480);
    BOOST_CHECK(result.second.refundOutputs.size() == 20);
    BOOST_CHECK(result.second.valueTransfers.size() == 0);
}

BOOST_AUTO_TEST_SUITE_END()
