#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>
#include <test/test_bitcoin.h>
#include <validation.h>
#include <util.h>

extern std::unique_ptr<QtumState> globalState;
dev::u256 GASLIMIT = dev::u256(500000);
dev::Address SENDERADDRESS = dev::Address("0101010101010101010101010101010101010101");
dev::h256 HASHTX = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
std::vector<valtype> CODE = 
{ 
    valtype(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029")),
    valtype(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029")),
    valtype(ParseHex("6060604052346000575b60005b600115601a5760019050600c565b5b505b603980602a6000396000f30060606040525b600b5b5b565b0000a165627a7a723058200c2277fb43f84eb7fe9afb9553caa1c8e80c75e989256069e593d6e9336565320029")),
    valtype(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029")),
    valtype(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029")),
    valtype(ParseHex("6060604052346000575b60005b600115601a5760019050600c565b5b505b603980602a6000396000f30060606040525b600b5b5b565b0000a165627a7a723058200c2277fb43f84eb7fe9afb9553caa1c8e80c75e989256069e593d6e9336565320029")),
    valtype(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029")),
    valtype(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029")),
    valtype(ParseHex("00")),
    valtype(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029")),
    valtype(ParseHex("00")),
    valtype(ParseHex("6060604052346000575b60448060166000396000f30060606040525b60165b5b6001156013576009565b5b565b0000a165627a7a7230582036699c18276860a335b2ba5670733c6b864f40fe3a36680866d9af2f8c2ef6ca0029")),
    valtype(ParseHex("00")),
    valtype(ParseHex("606060405234610000575b61034a806100196000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680633f811b80146100495780636b8ff5741461006a575b610000565b3461000057610068600480803560001916906020019091905050610087565b005b3461000057610085600480803590602001909190505061015b565b005b60008160405160e18061023e833901808260001916600019168152602001915050604051809103906000f08015610000579050600180548060010182818154818355818115116101035781836000526020600020918201910161010291905b808211156100fe5760008160009055506001016100e6565b5090565b5b505050916000526020600020900160005b83909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550505b5050565b6000600182815481101561000057906000526020600020900160005b9054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690508073ffffffffffffffffffffffffffffffffffffffff16638052474d6000604051602001526040518163ffffffff167c0100000000000000000000000000000000000000000000000000000000028152600401809050602060405180830381600087803b156100005760325a03f1156100005750505060405180519050600083815481101561000057906000526020600020900160005b5081600019169055505b50505600606060405234610000576040516020806100e1833981016040528080519060200190919050505b80600081600019169055505b505b609f806100426000396000f30060606040523615603d576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638052474d146045575b60435b5b565b005b34600057604f606d565b60405180826000191660001916815260200191505060405180910390f35b600054815600a165627a7a7230582054caf5870995bc5d22259287e288b70bff8d1e814c0974f8c27d10976fa334c10029a165627a7a72305820005b7140037b71c850595a05796b31ff9b55aa0442237dd7607667567074fb1c0029")),
    valtype(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029")),
    valtype(ParseHex("6060604052346000575b60448060166000396000f30060606040525b60165b5b6001156013576009565b5b565b0000a165627a7a7230582036699c18276860a335b2ba5670733c6b864f40fe3a36680866d9af2f8c2ef6ca0029")),
    valtype(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029")),
    valtype(ParseHex("6060604052734de45add9f5f0b6887081cfcfe3aca6da9eb3365600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b5b5b60b68061006a6000396000f30060606040523615603d576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806341c0e1b5146045575b60435b5b565b005b604b604d565b005b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16ff5b5600a165627a7a72305820e296f585c72ea3d4dce6880122cfe387d26c48b7960676a52e811b56ef8297a80029"))
};

void initState(){
    dev::eth::Ethash::init();
    boost::filesystem::path full_path = boost::filesystem::current_path();
    const std::string dirQtum = full_path.string() + "/src/test/qtumtests/tempState/";
    const dev::h256 hashDB(dev::sha3(dev::rlp("")));
    globalState = std::unique_ptr<QtumState>(new QtumState(dev::u256(0), QtumState::openDB(dirQtum, hashDB, dev::WithExisting::Trust), dev::eth::BaseState::Empty));
}

CBlock generateBlock(){
    CBlock block;
    CMutableTransaction tx;
    std::vector<unsigned char> address(ParseHex("abababababababababababababababababababab"));
    tx.vout.push_back(CTxOut(0, CScript() << OP_DUP << OP_HASH160 << address << OP_EQUALVERIFY << OP_CHECKSIG));
    block.vtx.push_back(MakeTransactionRef(CTransaction(tx)));
    return block;
}

dev::Address createQtumAddress(dev::h256 hashTx, uint32_t voutNumber){
    uint256 hashTXid(h256Touint(hashTx));
	std::vector<unsigned char> txIdAndVout(hashTXid.begin(), hashTXid.end());
	txIdAndVout.push_back(voutNumber);
	std::vector<unsigned char> SHA256TxVout(32);
    CSHA256().Write(txIdAndVout.data(), txIdAndVout.size()).Finalize(SHA256TxVout.data());
	std::vector<unsigned char> hashTxIdAndVout(20);
    CRIPEMD160().Write(SHA256TxVout.data(), SHA256TxVout.size()).Finalize(hashTxIdAndVout.data());
	return dev::Address(hashTxIdAndVout);
}

QtumTransaction createQtumTransaction(valtype data, dev::u256 value, dev::u256 gasLimit, dev::u256 gasPrice, dev::h256 hashTransaction, dev::Address recipient, int32_t nvout = 0){
    QtumTransaction txEth;
    if(recipient == dev::Address()){
        txEth = QtumTransaction(value, gasLimit, (gasLimit * gasPrice), data, dev::u256(0));
    } else {
        txEth = QtumTransaction(value, gasLimit, (gasLimit * gasPrice), recipient, data, dev::u256(0));
    }
    txEth.forceSender(SENDERADDRESS);
    txEth.setHashWith(hashTransaction);
    txEth.setNVout(nvout);
    return txEth;
}

std::pair<std::vector<execResult>, ByteCodeExecResult> executeBC(std::vector<QtumTransaction> txs){
    CBlock block(generateBlock());
    ByteCodeExec exec(block, txs);
    exec.performByteCode();
    std::vector<execResult> res = exec.getResult();
    ByteCodeExecResult bceExecRes = exec.processingResults();
    globalState->db().commit();
    return std::make_pair(res, bceExecRes);
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
    dev::Address newAddress(createQtumAddress(txs[0].getHashWith(), txs[0].getNVout()));
    std::unordered_map<dev::Address, dev::u256> addresses = globalState->addresses();

    BOOST_CHECK(result.first.size() == 1);
    BOOST_CHECK(addresses.size() == 1);
    BOOST_CHECK(result.first[0].first.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].first.newAddress == newAddress);
    BOOST_CHECK(result.first[0].first.output == ParseHex("60606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029"));
    BOOST_CHECK(globalState->balance(newAddress) == dev::u256(0));

    BOOST_CHECK(result.second.usedFee + result.second.refundSender == GASLIMIT);
    BOOST_CHECK(result.second.usedFee == 69382);
    BOOST_CHECK(result.second.refundSender == 430618);
    BOOST_CHECK(result.second.refundVOuts.size() == 1);
    BOOST_CHECK(result.second.refundVOuts[0].nValue == 430618);
    BOOST_CHECK(result.second.refundVOuts[0].scriptPubKey == CScript() << OP_DUP << OP_HASH160 << SENDERADDRESS.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
    BOOST_CHECK(result.second.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_create_contract_OutOfGasBase){
    initState();
    QtumTransaction txEth = createQtumTransaction(CODE[1], 0, dev::u256(100), dev::u256(1), HASHTX, dev::Address());
    std::vector<QtumTransaction> txs(1, txEth);
    auto result = executeBC(txs);
    std::unordered_map<dev::Address, dev::u256> addresses = globalState->addresses();

    BOOST_CHECK(result.first.size() == 1);
    BOOST_CHECK(addresses.size() == 0);
    BOOST_CHECK(result.first[0].first.excepted == dev::eth::TransactionException::OutOfGasBase);
    BOOST_CHECK(result.first[0].first.newAddress == dev::Address());
    BOOST_CHECK(result.first[0].first.output == valtype());

    BOOST_CHECK(result.second.usedFee + result.second.refundSender == 0);
    BOOST_CHECK(result.second.usedFee == 0);
    BOOST_CHECK(result.second.refundSender == 0);
    BOOST_CHECK(result.second.refundVOuts.size() == 0);
    BOOST_CHECK(result.second.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_create_contract_OutOfGas){
    initState();
    QtumTransaction txEth = createQtumTransaction(CODE[2], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address());
    std::vector<QtumTransaction> txs(1, txEth);
    auto result = executeBC(txs);
    std::unordered_map<dev::Address, dev::u256> addresses = globalState->addresses();

    BOOST_CHECK(result.first.size() == 1);
    BOOST_CHECK(addresses.size() == 0);
    BOOST_CHECK(result.first[0].first.excepted == dev::eth::TransactionException::OutOfGas);
    BOOST_CHECK(result.first[0].first.newAddress == dev::Address());
    BOOST_CHECK(result.first[0].first.output == valtype());

    BOOST_CHECK(result.second.usedFee + result.second.refundSender == 0);
    BOOST_CHECK(result.second.usedFee == 0);
    BOOST_CHECK(result.second.refundSender == 0);
    BOOST_CHECK(result.second.refundVOuts.size() == 0);
    BOOST_CHECK(result.second.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_OutOfGasBase_create_contract_normal_create_contract){
    initState();
    std::vector<dev::Address> newAddressGen;
    std::vector<QtumTransaction> txs;
    dev::h256 hash(HASHTX);
    for(size_t i = 0; i < 10; i++){
        dev::u256 gasLimit = i%2 == 0 ? GASLIMIT : dev::u256(100);
        QtumTransaction txEth = createQtumTransaction(CODE[3], 0, gasLimit, dev::u256(1), hash, dev::Address());
        newAddressGen.push_back(createQtumAddress(txEth.getHashWith(), txEth.getNVout()));
        txs.push_back(txEth);
        ++hash;
    }
    auto result = executeBC(txs);
    std::unordered_map<dev::Address, dev::u256> addresses = globalState->addresses();

    BOOST_CHECK(result.first.size() == 10);
    BOOST_CHECK(addresses.size() == 5);
    for(size_t i = 0; i < result.first.size(); i++){
        if(i%2 == 0){
            BOOST_CHECK(result.first[i].first.excepted == dev::eth::TransactionException::None);
            BOOST_CHECK(result.first[i].first.newAddress == dev::Address(newAddressGen[i]));
            BOOST_CHECK(result.first[i].first.output == ParseHex("60606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029"));
        } else {
            BOOST_CHECK(result.first[i].first.excepted == dev::eth::TransactionException::OutOfGasBase);
            BOOST_CHECK(result.first[i].first.newAddress == dev::Address());
            BOOST_CHECK(result.first[i].first.output == valtype());
        }
    }

    BOOST_CHECK(result.second.usedFee + result.second.refundSender == GASLIMIT * 5);
    BOOST_CHECK(result.second.usedFee == 346910);
    BOOST_CHECK(result.second.refundSender == 2153090);
    BOOST_CHECK(result.second.refundVOuts.size() == 5);
    for(const CTxOut& out : result.second.refundVOuts){
        BOOST_CHECK(out.scriptPubKey == CScript() << OP_DUP << OP_HASH160 << SENDERADDRESS.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
        BOOST_CHECK(out.nValue == 430618);
    }
    BOOST_CHECK(result.second.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_OutOfGas_create_contract_normal_create_contract){
    initState();
    std::vector<dev::Address> newAddressGen;
    std::vector<QtumTransaction> txs;
    dev::h256 hash(HASHTX);
    for(size_t i = 0; i < 10; i++){
        valtype code = i%2 == 0 ? CODE[4] : CODE[5];
        QtumTransaction txEth = createQtumTransaction(code, 0, GASLIMIT, dev::u256(1), hash, dev::Address());
        newAddressGen.push_back(createQtumAddress(txEth.getHashWith(), txEth.getNVout()));
        txs.push_back(txEth);
        ++hash;
    }
    auto result = executeBC(txs);
    std::unordered_map<dev::Address, dev::u256> addresses = globalState->addresses();

    BOOST_CHECK(result.first.size() == 10);
    BOOST_CHECK(addresses.size() == 5);
    for(size_t i = 0; i < result.first.size(); i++){
        if(i%2 == 0){
            BOOST_CHECK(result.first[i].first.excepted == dev::eth::TransactionException::None);
            BOOST_CHECK(result.first[i].first.newAddress == dev::Address(newAddressGen[i]));
            BOOST_CHECK(result.first[i].first.output == ParseHex("60606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029"));
        } else {
            BOOST_CHECK(result.first[i].first.excepted == dev::eth::TransactionException::OutOfGas);
            BOOST_CHECK(result.first[i].first.newAddress == dev::Address());
            BOOST_CHECK(result.first[i].first.output == valtype());
        }
    }

    BOOST_CHECK(result.second.usedFee + result.second.refundSender == GASLIMIT * 5);
    BOOST_CHECK(result.second.usedFee == 346910);
    BOOST_CHECK(result.second.refundSender == 2153090);
    BOOST_CHECK(result.second.refundVOuts.size() == 5);
    for(const CTxOut& out : result.second.refundVOuts){
        BOOST_CHECK(out.scriptPubKey == CScript() << OP_DUP << OP_HASH160 << SENDERADDRESS.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
        BOOST_CHECK(out.nValue == 430618);
    }
    BOOST_CHECK(result.second.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_create_contract_many){
    initState();
    std::vector<dev::Address> newAddressGen;
    std::vector<QtumTransaction> txs;
    dev::h256 hash(HASHTX);
    for(size_t i = 0; i < 130; i++){
        QtumTransaction txEth = createQtumTransaction(CODE[6], 0, GASLIMIT, dev::u256(1), hash, dev::Address(), i);
        newAddressGen.push_back(createQtumAddress(txEth.getHashWith(), txEth.getNVout()));
        txs.push_back(txEth);
        ++hash;
    }
    auto result = executeBC(txs);
    std::unordered_map<dev::Address, dev::u256> addresses = globalState->addresses();

    BOOST_CHECK(result.first.size() == txs.size());
    BOOST_CHECK(addresses.size() == txs.size());
    for(size_t i = 0; i < txs.size(); i++){
        BOOST_CHECK(result.first[i].first.excepted == dev::eth::TransactionException::None);
        BOOST_CHECK(result.first[i].first.newAddress == newAddressGen[i]);
        BOOST_CHECK(result.first[i].first.output == ParseHex("60606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029"));
        BOOST_CHECK(globalState->balance(newAddressGen[i]) == dev::u256(0));
    }

    BOOST_CHECK(result.second.usedFee + result.second.refundSender == GASLIMIT * 130);
    BOOST_CHECK(result.second.usedFee == 9019660);
    BOOST_CHECK(result.second.refundSender == 55980340);
    BOOST_CHECK(result.second.refundVOuts.size() == 130);
    for(const CTxOut& out : result.second.refundVOuts){
        BOOST_CHECK(out.scriptPubKey == CScript() << OP_DUP << OP_HASH160 << SENDERADDRESS.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
        BOOST_CHECK(out.nValue == 430618);
    }
    BOOST_CHECK(result.second.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_call_contract_transfer){
    initState();
    QtumTransaction txEthCreate = createQtumTransaction(CODE[7], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address());
    std::vector<QtumTransaction> txsCreate(1, txEthCreate);
    executeBC(txsCreate);
    dev::Address newAddress(createQtumAddress(txsCreate[0].getHashWith(), txsCreate[0].getNVout()));
    QtumTransaction txEthCall = createQtumTransaction(CODE[8], 1300, GASLIMIT, dev::u256(1), HASHTX, newAddress);
    std::vector<QtumTransaction> txsCall(1, txEthCall);
    auto result = executeBC(txsCall);
    std::unordered_map<dev::Address, dev::u256> addressesCall = globalState->addresses();

    BOOST_CHECK(result.first.size() == 1);
    BOOST_CHECK(addressesCall.size() == 1);
    BOOST_CHECK(result.first[0].first.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].first.newAddress == dev::Address());
    BOOST_CHECK(result.first[0].first.output == valtype());
    BOOST_CHECK(globalState->balance(newAddress) == dev::u256(1300));

    BOOST_CHECK(result.second.usedFee + result.second.refundSender == GASLIMIT);
    BOOST_CHECK(result.second.usedFee == 21037);
    BOOST_CHECK(result.second.refundSender == 478963);
    BOOST_CHECK(result.second.refundVOuts.size() == 1);
    BOOST_CHECK(result.second.refundVOuts[0].nValue == 478963);
    BOOST_CHECK(result.second.refundVOuts[0].scriptPubKey == CScript() << OP_DUP << OP_HASH160 << SENDERADDRESS.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
    BOOST_CHECK(result.second.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_call_contract_transfer_OutOfGasBase_return_value){
    initState();
    QtumTransaction txEthCreate = createQtumTransaction(CODE[9], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address());
    std::vector<QtumTransaction> txsCreate(1, txEthCreate);
    executeBC(txsCreate);
    dev::Address newAddress(createQtumAddress(txsCreate[0].getHashWith(), txsCreate[0].getNVout()));
    QtumTransaction txEthCall = createQtumTransaction(CODE[10], 1300, dev::u256(1), dev::u256(1), HASHTX, newAddress);
    std::vector<QtumTransaction> txsCall(1, txEthCall);
    auto result = executeBC(txsCall);
    std::unordered_map<dev::Address, dev::u256> addressesCall = globalState->addresses();

    BOOST_CHECK(result.first.size() == 1);
    BOOST_CHECK(addressesCall.size() == 1);
    BOOST_CHECK(result.first[0].first.excepted == dev::eth::TransactionException::OutOfGasBase);
    BOOST_CHECK(result.first[0].first.newAddress == dev::Address());
    BOOST_CHECK(result.first[0].first.output == valtype());
    BOOST_CHECK(globalState->balance(newAddress) == dev::u256(0));

    BOOST_CHECK(result.second.usedFee + result.second.refundSender == 0);
    BOOST_CHECK(result.second.usedFee == 0);
    BOOST_CHECK(result.second.refundSender == 0);
    BOOST_CHECK(result.second.refundVOuts.size() == 0);
    BOOST_CHECK(result.second.refundValueTx.size() == 1);
    BOOST_CHECK(result.second.refundValueTx[0].vout[0].nValue == 1300);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_call_contract_transfer_OutOfGas_return_value){
    initState();
    QtumTransaction txEthCreate = createQtumTransaction(CODE[11], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address());
    std::vector<QtumTransaction> txsCreate(1, txEthCreate);
    executeBC(txsCreate);
    dev::Address newAddress(createQtumAddress(txsCreate[0].getHashWith(), txsCreate[0].getNVout()));
    QtumTransaction txEthCall = createQtumTransaction(CODE[12], 1300, GASLIMIT, dev::u256(1), HASHTX, newAddress);
    std::vector<QtumTransaction> txsCall(1, txEthCall);
    auto result = executeBC(txsCall);
    std::unordered_map<dev::Address, dev::u256> addressesCall = globalState->addresses();

    BOOST_CHECK(result.first.size() == 1);
    BOOST_CHECK(addressesCall.size() == 1);
    BOOST_CHECK(result.first[0].first.excepted == dev::eth::TransactionException::OutOfGas);
    BOOST_CHECK(result.first[0].first.newAddress == dev::Address());
    BOOST_CHECK(result.first[0].first.output == valtype());
    BOOST_CHECK(globalState->balance(newAddress) == dev::u256(0));

    BOOST_CHECK(result.second.usedFee + result.second.refundSender == 0);
    BOOST_CHECK(result.second.usedFee == 0);
    BOOST_CHECK(result.second.refundSender == 0);
    BOOST_CHECK(result.second.refundVOuts.size() == 0);
    BOOST_CHECK(result.second.refundValueTx.size() == 1);
    BOOST_CHECK(result.second.refundValueTx[0].vout[0].nValue == 1300);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_call_contract_transfer_many){
    initState();
    std::vector<dev::Address> newAddressGen;
    std::vector<QtumTransaction> txs;
    dev::h256 hash(HASHTX);
    for(size_t i = 0; i < 130; i++){
        QtumTransaction txEth = createQtumTransaction(CODE[14], 0, GASLIMIT, dev::u256(1), hash, dev::Address(), i);
        newAddressGen.push_back(createQtumAddress(txEth.getHashWith(), txEth.getNVout()));
        txs.push_back(txEth);
        ++hash;
    }
    executeBC(txs);
    std::vector<QtumTransaction> txsCall;
    valtype codeCall(ParseHex("00"));
    dev::h256 hashCall(HASHTX);
    for(size_t i = 0; i < txs.size(); i++){
        QtumTransaction txEthCall = createQtumTransaction(codeCall, 1300 + i, GASLIMIT, dev::u256(1), hash, newAddressGen[i], i);
        txsCall.push_back(txEthCall);
        ++hashCall;
    }
    auto result = executeBC(txsCall);
    std::unordered_map<dev::Address, dev::u256> addressesCall = globalState->addresses();

    BOOST_CHECK(result.first.size() == txs.size());
    BOOST_CHECK(addressesCall.size() == txs.size());
    for(size_t i =0; i < txs.size(); i++){
        BOOST_CHECK(result.first[i].first.excepted == dev::eth::TransactionException::None);
        BOOST_CHECK(result.first[i].first.newAddress == dev::Address());
        BOOST_CHECK(result.first[i].first.output == valtype());
        BOOST_CHECK(globalState->balance(newAddressGen[i]) == dev::u256(1300 + i));
    }

    BOOST_CHECK(result.second.usedFee + result.second.refundSender == GASLIMIT * 130);
    BOOST_CHECK(result.second.usedFee == 2734810);
    BOOST_CHECK(result.second.refundSender == 62265190);
    BOOST_CHECK(result.second.refundVOuts.size() == 130);
    for(const CTxOut& out : result.second.refundVOuts){
        BOOST_CHECK(out.scriptPubKey == CScript() << OP_DUP << OP_HASH160 << SENDERADDRESS.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
        BOOST_CHECK(out.nValue == 478963);
    }
    BOOST_CHECK(result.second.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_call_contract_OutOfGas_transfer_many_return_value){
    initState();
    std::vector<dev::Address> newAddressGen;
    std::vector<QtumTransaction> txs;
    dev::h256 hash(HASHTX);
    for(size_t i = 0; i < 130; i++){
        QtumTransaction txEth = createQtumTransaction(CODE[15], 0, GASLIMIT, dev::u256(1), hash, dev::Address(), i);
        newAddressGen.push_back(createQtumAddress(txEth.getHashWith(), txEth.getNVout()));
        txs.push_back(txEth);
        ++hash;
    }
    executeBC(txs);
    std::vector<QtumTransaction> txsCall;
    valtype codeCall(ParseHex("00"));
    dev::h256 hashCall(HASHTX);
    for(size_t i = 0; i < txs.size(); i++){
        QtumTransaction txEthCall = createQtumTransaction(codeCall, 1300 + i, GASLIMIT, dev::u256(1), hashCall, newAddressGen[i], i);
        txsCall.push_back(txEthCall);
        ++hashCall;
    }
    auto result = executeBC(txsCall);
    std::unordered_map<dev::Address, dev::u256> addressesCall = globalState->addresses();

    BOOST_CHECK(result.first.size() == txs.size());
    BOOST_CHECK(addressesCall.size() == txs.size());
    for(size_t i =0; i < txs.size(); i++){
        BOOST_CHECK(result.first[i].first.excepted == dev::eth::TransactionException::OutOfGas);
        BOOST_CHECK(result.first[i].first.newAddress == dev::Address());
        BOOST_CHECK(result.first[i].first.output == valtype());
        BOOST_CHECK(globalState->balance(newAddressGen[i]) == dev::u256(0));
    }

    BOOST_CHECK(result.second.usedFee + result.second.refundSender == 0);
    BOOST_CHECK(result.second.usedFee == 0);
    BOOST_CHECK(result.second.refundSender == 0);
    BOOST_CHECK(result.second.refundVOuts.size() == 0);
    BOOST_CHECK(result.second.refundValueTx.size() == 130);
    size_t add = 0;
    for(const CTransaction& tx : result.second.refundValueTx){
        BOOST_CHECK(tx.vout[0].nValue == CAmount(1300 + add));
        add++;
    }
}

BOOST_AUTO_TEST_CASE(bytecodeexec_suicide){
    initState();
    std::vector<QtumTransaction> txsCreate;
    std::vector<dev::Address> newAddresses;
    dev::h256 hash(HASHTX);
    for(size_t i = 0; i < 10; i++){
        valtype code = i == 0 ? CODE[16] : CODE[17];
        dev::u256 value = i == 0 ? dev::u256(0) : dev::u256(13);
        QtumTransaction txEthCreate = createQtumTransaction(code, value, GASLIMIT, dev::u256(1), hash, dev::Address(), i);
        txsCreate.push_back(txEthCreate);
        newAddresses.push_back(createQtumAddress(txEthCreate.getHashWith(), txEthCreate.getNVout()));
        ++hash;
    }
    executeBC(txsCreate);
    std::unordered_map<dev::Address, dev::u256> addressesCreate = globalState->addresses();
    std::vector<QtumTransaction> txsCall;
    valtype codeCall(ParseHex("41c0e1b5"));
    for(size_t i = 0; i < txsCreate.size() - 1; i++){
        QtumTransaction txEthCall = createQtumTransaction(codeCall, 0, GASLIMIT, dev::u256(1), HASHTX, newAddresses[i + 1]);
        txsCall.push_back(txEthCall);
    }
    auto result = executeBC(txsCall);
    std::unordered_map<dev::Address, dev::u256> addressesCall = globalState->addresses();

    BOOST_CHECK(result.first.size() == 9);
    BOOST_CHECK(addressesCall.size() == 1);
    for(size_t i = 0; i < result.first.size(); i++){
        BOOST_CHECK(result.first[i].first.excepted == dev::eth::TransactionException::None);
        BOOST_CHECK(result.first[i].first.newAddress == dev::Address());
        BOOST_CHECK(result.first[i].first.output == valtype());
    }
    BOOST_CHECK(globalState->balance(newAddresses[0]) == dev::u256(117));

    BOOST_CHECK(result.second.usedFee + result.second.refundSender == GASLIMIT * 9);
    BOOST_CHECK(result.second.usedFee == 96588);
    BOOST_CHECK(result.second.refundSender == 4403412);
    BOOST_CHECK(result.second.refundVOuts.size() == 9);
    for(size_t i = 0; i < result.first.size(); i++){
        BOOST_CHECK(result.second.refundVOuts[i].scriptPubKey == CScript() << OP_DUP << OP_HASH160 << SENDERADDRESS.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
        BOOST_CHECK(result.second.refundVOuts[i].nValue == 489268);
    }
    BOOST_CHECK(result.second.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_contract_create_contracts){
    initState();
    QtumTransaction txEthCreate = createQtumTransaction(CODE[13], 0, GASLIMIT, dev::u256(1), HASHTX, dev::Address());
    std::vector<QtumTransaction> txs(1, txEthCreate);
    executeBC(txs);
    std::vector<QtumTransaction> txsCall;
    valtype codeCall(ParseHex("3f811b80"));
    dev::Address newAddress(createQtumAddress(txEthCreate.getHashWith(), txEthCreate.getNVout()));
    for(size_t i = 0; i < 20; i++){
        QtumTransaction txEthCall = createQtumTransaction(codeCall, 0, GASLIMIT, dev::u256(1), HASHTX, newAddress, i);
        txsCall.push_back(txEthCall);
    }
    auto result = executeBC(txsCall);
    std::unordered_map<dev::Address, dev::u256> addressesCall = globalState->addresses();

    BOOST_CHECK(result.first.size() == 20);
    BOOST_CHECK(addressesCall.size() == 21);
    BOOST_CHECK(globalState->balance(newAddress) == dev::u256(0));
    for(size_t i = 0; i < result.first.size(); i++){
        BOOST_CHECK(result.first[i].first.excepted == dev::eth::TransactionException::None);
        BOOST_CHECK(result.first[i].first.newAddress == dev::Address());
        BOOST_CHECK(result.first[i].first.output == valtype());
    }

    BOOST_CHECK(result.second.usedFee + result.second.refundSender == GASLIMIT * 20);
    BOOST_CHECK(result.second.usedFee == 2335520);
    BOOST_CHECK(result.second.refundSender == 7664480);
    BOOST_CHECK(result.second.refundVOuts.size() == 20);
    BOOST_CHECK(result.second.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_SUITE_END()
