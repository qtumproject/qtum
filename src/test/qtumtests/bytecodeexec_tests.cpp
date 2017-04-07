#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>
#include <test/test_bitcoin.h>
#include <validation.h>
#include <util.h>

extern std::unique_ptr<QtumState> globalState;
dev::u256 GASLIMIT = dev::u256(500000);
dev::Address SENDERADDRESS = dev::Address("0101010101010101010101010101010101010101");
dev::h256 HASHTX = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

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

BOOST_FIXTURE_TEST_SUITE(bytecodeexec_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(bytecodeexec_txs_empty){
    initState();
    CBlock block(generateBlock());
    std::vector<QtumTransaction> txs;
    ByteCodeExec exec(block, txs);
    exec.performByteCode();
    std::vector<execResult> res = exec.getResult();
    BOOST_CHECK(res.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_create_contract){
    initState();
    CBlock block(generateBlock());

    QtumTransaction txEth;
    valtype code(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029"));
    txEth = QtumTransaction(0, GASLIMIT, (GASLIMIT * dev::u256(1)), code, dev::u256(0));
    txEth.forceSender(SENDERADDRESS);
    txEth.setHashWith(HASHTX);
    txEth.setNVout(0);

    std::vector<QtumTransaction> txs(1, txEth);
    ByteCodeExec exec(block, txs);
    exec.performByteCode();
    std::vector<execResult> res = exec.getResult();
    ByteCodeExecResult bceExecRes = exec.processingResults();
    globalState->db().commit();

    dev::Address newAddress(createQtumAddress(txs[0].getHashWith(), txs[0].getNVout()));
    std::unordered_map<dev::Address, dev::u256> addresses = globalState->addresses();

    BOOST_CHECK(res.size() == 1);
    BOOST_CHECK(addresses.size() == 1);
    BOOST_CHECK(res[0].first.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(res[0].first.newAddress == newAddress);
    BOOST_CHECK(res[0].first.output == ParseHex("60606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029"));
    BOOST_CHECK(globalState->balance(newAddress) == dev::u256(0));

    BOOST_CHECK(bceExecRes.usedFee + bceExecRes.refundSender == GASLIMIT);
    BOOST_CHECK(bceExecRes.usedFee == 69382);
    BOOST_CHECK(bceExecRes.refundSender == 430618);
    BOOST_CHECK(bceExecRes.refundVOuts.size() == 1);
    BOOST_CHECK(bceExecRes.refundVOuts[0].nValue == 430618);
    BOOST_CHECK(bceExecRes.refundVOuts[0].scriptPubKey == CScript() << OP_DUP << OP_HASH160 << SENDERADDRESS.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
    BOOST_CHECK(bceExecRes.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_create_contract_OutOfGasBase){
    initState();
    CBlock block(generateBlock());

    QtumTransaction txEth;
    valtype code(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029"));
    txEth = QtumTransaction(0, dev::u256(100), (dev::u256(100) * dev::u256(1)), code, dev::u256(0));
    txEth.forceSender(SENDERADDRESS);
    txEth.setHashWith(HASHTX);
    txEth.setNVout(0);

    std::vector<QtumTransaction> txs(1, txEth);
    ByteCodeExec exec(block, txs);
    exec.performByteCode();
    std::vector<execResult> res = exec.getResult();
    ByteCodeExecResult bceExecRes = exec.processingResults();
    globalState->db().commit();

    std::unordered_map<dev::Address, dev::u256> addresses = globalState->addresses();

    BOOST_CHECK(res.size() == 1);
    BOOST_CHECK(addresses.size() == 0);
    BOOST_CHECK(res[0].first.excepted == dev::eth::TransactionException::OutOfGasBase);
    BOOST_CHECK(res[0].first.newAddress == dev::Address());
    BOOST_CHECK(res[0].first.output == valtype());

    BOOST_CHECK(bceExecRes.usedFee + bceExecRes.refundSender == 0);
    BOOST_CHECK(bceExecRes.usedFee == 0);
    BOOST_CHECK(bceExecRes.refundSender == 0);
    BOOST_CHECK(bceExecRes.refundVOuts.size() == 0);
    BOOST_CHECK(bceExecRes.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_create_contract_OutOfGas){
    initState();
    CBlock block(generateBlock());

    QtumTransaction txEth;
    valtype code(ParseHex("6060604052346000575b60005b600115601a5760019050600c565b5b505b603980602a6000396000f30060606040525b600b5b5b565b0000a165627a7a723058200c2277fb43f84eb7fe9afb9553caa1c8e80c75e989256069e593d6e9336565320029"));
    txEth = QtumTransaction(0, GASLIMIT, (GASLIMIT * dev::u256(1)), code, dev::u256(0));
    txEth.forceSender(SENDERADDRESS);
    txEth.setHashWith(HASHTX);
    txEth.setNVout(0);

    std::vector<QtumTransaction> txs(1, txEth);
    ByteCodeExec exec(block, txs);
    exec.performByteCode();
    std::vector<execResult> res = exec.getResult();
    ByteCodeExecResult bceExecRes = exec.processingResults();
    globalState->db().commit();

    std::unordered_map<dev::Address, dev::u256> addresses = globalState->addresses();

    BOOST_CHECK(res.size() == 1);
    BOOST_CHECK(addresses.size() == 0);
    BOOST_CHECK(res[0].first.excepted == dev::eth::TransactionException::OutOfGas);
    BOOST_CHECK(res[0].first.newAddress == dev::Address());
    BOOST_CHECK(res[0].first.output == valtype());

    BOOST_CHECK(bceExecRes.usedFee + bceExecRes.refundSender == 0);
    BOOST_CHECK(bceExecRes.usedFee == 0);
    BOOST_CHECK(bceExecRes.refundSender == 0);
    BOOST_CHECK(bceExecRes.refundVOuts.size() == 0);
    BOOST_CHECK(bceExecRes.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_OutOfGasBase_create_contract_normal_create_contract){
    initState();
    CBlock block(generateBlock());

    std::vector<dev::Address> newAddressGen;
    std::vector<QtumTransaction> txs;
    dev::h256 hash(HASHTX);
    for(size_t i = 0; i < 10; i++){
        QtumTransaction txEth;
        valtype code(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029"));
        if(i%2 == 0){
            txEth = QtumTransaction(0, GASLIMIT, (GASLIMIT * dev::u256(1)), code, dev::u256(0));
        } else {
            txEth = QtumTransaction(0, dev::u256(100), (dev::u256(100) * dev::u256(1)), code, dev::u256(0));
        }
        txEth.forceSender(SENDERADDRESS);
        txEth.setHashWith(hash);
        txEth.setNVout(i);

        newAddressGen.push_back(createQtumAddress(txEth.getHashWith(), txEth.getNVout()));
        txs.push_back(txEth);
        ++hash;
    }

    ByteCodeExec exec(block, txs);
    exec.performByteCode();
    std::vector<execResult> res = exec.getResult();
    ByteCodeExecResult bceExecRes = exec.processingResults();
    globalState->db().commit();

    std::unordered_map<dev::Address, dev::u256> addresses = globalState->addresses();

    BOOST_CHECK(res.size() == 10);
    BOOST_CHECK(addresses.size() == 5);
    for(size_t i = 0; i < res.size(); i++){
        if(i%2 == 0){
            BOOST_CHECK(res[i].first.excepted == dev::eth::TransactionException::None);
            BOOST_CHECK(res[i].first.newAddress == dev::Address(newAddressGen[i]));
            BOOST_CHECK(res[i].first.output == ParseHex("60606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029"));
        } else {
            BOOST_CHECK(res[i].first.excepted == dev::eth::TransactionException::OutOfGasBase);
            BOOST_CHECK(res[i].first.newAddress == dev::Address());
            BOOST_CHECK(res[i].first.output == valtype());
        }
    }

    BOOST_CHECK(bceExecRes.usedFee + bceExecRes.refundSender == GASLIMIT * 5);
    BOOST_CHECK(bceExecRes.usedFee == 346910);
    BOOST_CHECK(bceExecRes.refundSender == 2153090);
    BOOST_CHECK(bceExecRes.refundVOuts.size() == 5);
    for(const CTxOut& out : bceExecRes.refundVOuts){
        BOOST_CHECK(out.scriptPubKey == CScript() << OP_DUP << OP_HASH160 << SENDERADDRESS.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
        BOOST_CHECK(out.nValue == 430618);
    }
    BOOST_CHECK(bceExecRes.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_OutOfGas_create_contract_normal_create_contract){
    initState();
    CBlock block(generateBlock());

    std::vector<dev::Address> newAddressGen;
    std::vector<QtumTransaction> txs;
    dev::h256 hash(HASHTX);
    for(size_t i = 0; i < 10; i++){
        QtumTransaction txEth;
        valtype code;
        if(i%2 == 0){
            code = ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029");
        } else {
            code = ParseHex("6060604052346000575b60005b600115601a5760019050600c565b5b505b603980602a6000396000f30060606040525b600b5b5b565b0000a165627a7a723058200c2277fb43f84eb7fe9afb9553caa1c8e80c75e989256069e593d6e9336565320029");
        }
        txEth = QtumTransaction(0, GASLIMIT, (GASLIMIT * dev::u256(1)), code, dev::u256(0));
        txEth.forceSender(SENDERADDRESS);
        txEth.setHashWith(hash);
        txEth.setNVout(i);

        newAddressGen.push_back(createQtumAddress(txEth.getHashWith(), txEth.getNVout()));
        txs.push_back(txEth);
        ++hash;
    }

    ByteCodeExec exec(block, txs);
    exec.performByteCode();
    std::vector<execResult> res = exec.getResult();
    ByteCodeExecResult bceExecRes = exec.processingResults();
    globalState->db().commit();

    std::unordered_map<dev::Address, dev::u256> addresses = globalState->addresses();

    BOOST_CHECK(res.size() == 10);
    BOOST_CHECK(addresses.size() == 5);
    for(size_t i = 0; i < res.size(); i++){
        if(i%2 == 0){
            BOOST_CHECK(res[i].first.excepted == dev::eth::TransactionException::None);
            BOOST_CHECK(res[i].first.newAddress == dev::Address(newAddressGen[i]));
            BOOST_CHECK(res[i].first.output == ParseHex("60606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029"));
        } else {
            BOOST_CHECK(res[i].first.excepted == dev::eth::TransactionException::OutOfGas);
            BOOST_CHECK(res[i].first.newAddress == dev::Address());
            BOOST_CHECK(res[i].first.output == valtype());
        }
    }

    BOOST_CHECK(bceExecRes.usedFee + bceExecRes.refundSender == GASLIMIT * 5);
    BOOST_CHECK(bceExecRes.usedFee == 346910);
    BOOST_CHECK(bceExecRes.refundSender == 2153090);
    BOOST_CHECK(bceExecRes.refundVOuts.size() == 5);
    for(const CTxOut& out : bceExecRes.refundVOuts){
        BOOST_CHECK(out.scriptPubKey == CScript() << OP_DUP << OP_HASH160 << SENDERADDRESS.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
        BOOST_CHECK(out.nValue == 430618);
    }
    BOOST_CHECK(bceExecRes.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_create_contract_many){
    initState();
    CBlock block(generateBlock());

    std::vector<dev::Address> newAddressGen;
    std::vector<QtumTransaction> txs;
    QtumTransaction txEth;
    valtype code(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029"));
    txEth = QtumTransaction(0, GASLIMIT, (GASLIMIT * dev::u256(1)), code, dev::u256(0));
    txEth.forceSender(SENDERADDRESS);
    dev::h256 hash(HASHTX);
    for(size_t i = 0; i < 130; i++){
        txEth.setHashWith(hash);
        txEth.setNVout(i);

        newAddressGen.push_back(createQtumAddress(txEth.getHashWith(), txEth.getNVout()));
        txs.push_back(txEth);
        ++hash;
    }
    
    ByteCodeExec exec(block, txs);
    exec.performByteCode();
    std::vector<execResult> res = exec.getResult();
    ByteCodeExecResult bceExecRes = exec.processingResults();
    globalState->db().commit();

    std::unordered_map<dev::Address, dev::u256> addresses = globalState->addresses();

    BOOST_CHECK(res.size() == txs.size());
    BOOST_CHECK(addresses.size() == txs.size());
    for(size_t i = 0; i < txs.size(); i++){
        BOOST_CHECK(res[i].first.excepted == dev::eth::TransactionException::None);
        BOOST_CHECK(res[i].first.newAddress == newAddressGen[i]);
        BOOST_CHECK(res[i].first.output == ParseHex("60606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029"));
        BOOST_CHECK(globalState->balance(newAddressGen[i]) == dev::u256(0));
    }

    BOOST_CHECK(bceExecRes.usedFee + bceExecRes.refundSender == GASLIMIT * 130);
    BOOST_CHECK(bceExecRes.usedFee == 9019660);
    BOOST_CHECK(bceExecRes.refundSender == 55980340);
    BOOST_CHECK(bceExecRes.refundVOuts.size() == 130);
    for(const CTxOut& out : bceExecRes.refundVOuts){
        BOOST_CHECK(out.scriptPubKey == CScript() << OP_DUP << OP_HASH160 << SENDERADDRESS.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
        BOOST_CHECK(out.nValue == 430618);
    }
    BOOST_CHECK(bceExecRes.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_call_contract_transfer){
    initState();
    CBlock block(generateBlock());

    QtumTransaction txEthCreate;
    valtype code(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029"));
    txEthCreate = QtumTransaction(0, GASLIMIT, (GASLIMIT * dev::u256(1)), code, dev::u256(0));
    txEthCreate.forceSender(SENDERADDRESS);
    txEthCreate.setHashWith(HASHTX);
    txEthCreate.setNVout(0);

    std::vector<QtumTransaction> txsCreate(1, txEthCreate);
    ByteCodeExec exec(block, txsCreate);
    exec.performByteCode();
    std::vector<execResult> res = exec.getResult();
    globalState->db().commit();

    dev::Address newAddress(createQtumAddress(txsCreate[0].getHashWith(), txsCreate[0].getNVout()));

    QtumTransaction txEthCall;
    valtype codeCall(ParseHex("00"));
    txEthCall = QtumTransaction(1300, GASLIMIT, (GASLIMIT * dev::u256(1)), newAddress, codeCall, dev::u256(0));
    txEthCall.forceSender(SENDERADDRESS);
    txEthCall.setHashWith(HASHTX);
    txEthCall.setNVout(0);

    std::vector<QtumTransaction> txsCall(1, txEthCall);
    ByteCodeExec exec2(block, txsCall);
    exec2.performByteCode();
    std::vector<execResult> resCall = exec2.getResult();
    ByteCodeExecResult bceExecRes = exec2.processingResults();
    globalState->db().commit();

    std::unordered_map<dev::Address, dev::u256> addressesCall = globalState->addresses();

    BOOST_CHECK(resCall.size() == 1);
    BOOST_CHECK(addressesCall.size() == 1);
    BOOST_CHECK(resCall[0].first.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(resCall[0].first.newAddress == dev::Address());
    BOOST_CHECK(resCall[0].first.output == valtype());
    BOOST_CHECK(globalState->balance(newAddress) == dev::u256(1300));

    BOOST_CHECK(bceExecRes.usedFee + bceExecRes.refundSender == GASLIMIT);
    BOOST_CHECK(bceExecRes.usedFee == 21037);
    BOOST_CHECK(bceExecRes.refundSender == 478963);
    BOOST_CHECK(bceExecRes.refundVOuts.size() == 1);
    BOOST_CHECK(bceExecRes.refundVOuts[0].nValue == 478963);
    BOOST_CHECK(bceExecRes.refundVOuts[0].scriptPubKey == CScript() << OP_DUP << OP_HASH160 << SENDERADDRESS.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
    BOOST_CHECK(bceExecRes.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_call_contract_transfer_OutOfGasBase_return_value){
    initState();
    CBlock block(generateBlock());

    QtumTransaction txEthCreate;
    valtype code(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029"));
    txEthCreate = QtumTransaction(0, GASLIMIT, (GASLIMIT * dev::u256(1)), code, dev::u256(0));
    txEthCreate.forceSender(SENDERADDRESS);
    txEthCreate.setHashWith(HASHTX);
    txEthCreate.setNVout(0);

    std::vector<QtumTransaction> txsCreate(1, txEthCreate);
    ByteCodeExec exec(block, txsCreate);
    exec.performByteCode();
    std::vector<execResult> res = exec.getResult();
    globalState->db().commit();

    dev::Address newAddress(createQtumAddress(txsCreate[0].getHashWith(), txsCreate[0].getNVout()));

    QtumTransaction txEthCall;
    valtype codeCall(ParseHex("00"));
    txEthCall = QtumTransaction(1300, dev::u256(1), (dev::u256(1) * dev::u256(1)), newAddress, codeCall, dev::u256(0));
    txEthCall.forceSender(SENDERADDRESS);
    txEthCall.setHashWith(HASHTX);
    txEthCall.setNVout(0);

    std::vector<QtumTransaction> txsCall(1, txEthCall);
    ByteCodeExec exec2(block, txsCall);
    exec2.performByteCode();
    std::vector<execResult> resCall = exec2.getResult();
    ByteCodeExecResult bceExecRes = exec2.processingResults();
    globalState->db().commit();

    std::unordered_map<dev::Address, dev::u256> addressesCall = globalState->addresses();

    BOOST_CHECK(resCall.size() == 1);
    BOOST_CHECK(addressesCall.size() == 1);
    BOOST_CHECK(resCall[0].first.excepted == dev::eth::TransactionException::OutOfGasBase);
    BOOST_CHECK(resCall[0].first.newAddress == dev::Address());
    BOOST_CHECK(resCall[0].first.output == valtype());
    BOOST_CHECK(globalState->balance(newAddress) == dev::u256(0));

    BOOST_CHECK(bceExecRes.usedFee + bceExecRes.refundSender == 0);
    BOOST_CHECK(bceExecRes.usedFee == 0);
    BOOST_CHECK(bceExecRes.refundSender == 0);
    BOOST_CHECK(bceExecRes.refundVOuts.size() == 0);
    BOOST_CHECK(bceExecRes.refundValueTx.size() == 1);
    BOOST_CHECK(bceExecRes.refundValueTx[0].vout[0].nValue == 1300);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_call_contract_transfer_OutOfGas_return_value){
    initState();
    CBlock block(generateBlock());

    QtumTransaction txEthCreate;
    valtype code(ParseHex("6060604052346000575b60448060166000396000f30060606040525b60165b5b6001156013576009565b5b565b0000a165627a7a7230582036699c18276860a335b2ba5670733c6b864f40fe3a36680866d9af2f8c2ef6ca0029"));
    txEthCreate = QtumTransaction(0, GASLIMIT, (GASLIMIT * dev::u256(1)), code, dev::u256(0));
    txEthCreate.forceSender(SENDERADDRESS);
    txEthCreate.setHashWith(HASHTX);
    txEthCreate.setNVout(0);

    std::vector<QtumTransaction> txsCreate(1, txEthCreate);
    ByteCodeExec exec(block, txsCreate);
    exec.performByteCode();
    std::vector<execResult> res = exec.getResult();
    globalState->db().commit();

    dev::Address newAddress(createQtumAddress(txsCreate[0].getHashWith(), txsCreate[0].getNVout()));

    QtumTransaction txEthCall;
    valtype codeCall(ParseHex("00"));
    txEthCall = QtumTransaction(1300, GASLIMIT, (GASLIMIT * dev::u256(1)), newAddress, codeCall, dev::u256(0));
    txEthCall.forceSender(SENDERADDRESS);
    txEthCall.setHashWith(HASHTX);
    txEthCall.setNVout(0);

    std::vector<QtumTransaction> txsCall(1, txEthCall);
    ByteCodeExec exec2(block, txsCall);
    exec2.performByteCode();
    std::vector<execResult> resCall = exec2.getResult();
    ByteCodeExecResult bceExecRes = exec2.processingResults();
    globalState->db().commit();

    std::unordered_map<dev::Address, dev::u256> addressesCall = globalState->addresses();

    BOOST_CHECK(resCall.size() == 1);
    BOOST_CHECK(addressesCall.size() == 1);
    BOOST_CHECK(resCall[0].first.excepted == dev::eth::TransactionException::OutOfGas);
    BOOST_CHECK(resCall[0].first.newAddress == dev::Address());
    BOOST_CHECK(resCall[0].first.output == valtype());
    BOOST_CHECK(globalState->balance(newAddress) == dev::u256(0));

    BOOST_CHECK(bceExecRes.usedFee + bceExecRes.refundSender == 0);
    BOOST_CHECK(bceExecRes.usedFee == 0);
    BOOST_CHECK(bceExecRes.refundSender == 0);
    BOOST_CHECK(bceExecRes.refundVOuts.size() == 0);
    BOOST_CHECK(bceExecRes.refundValueTx.size() == 1);
    BOOST_CHECK(bceExecRes.refundValueTx[0].vout[0].nValue == 1300);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_call_contract_transfer_many){
    initState();
    CBlock block(generateBlock());

    std::vector<dev::Address> newAddressGen;
    std::vector<QtumTransaction> txs;
    QtumTransaction txEth;
    valtype code(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029"));
    txEth = QtumTransaction(0, GASLIMIT, (GASLIMIT * dev::u256(1)), code, dev::u256(0));
    txEth.forceSender(SENDERADDRESS);
    dev::h256 hash(HASHTX);
    for(size_t i = 0; i < 130; i++){
        txEth.setHashWith(hash);
        txEth.setNVout(i);

        newAddressGen.push_back(createQtumAddress(txEth.getHashWith(), txEth.getNVout()));
        txs.push_back(txEth);
        ++hash;
    }
    
    ByteCodeExec exec(block, txs);
    exec.performByteCode();
    std::vector<execResult> res = exec.getResult();
    globalState->db().commit();

    std::vector<QtumTransaction> txsCall;
    QtumTransaction txEthCall;
    valtype codeCall(ParseHex("00"));
    dev::h256 hashCall(HASHTX);
    for(size_t i = 0; i < txs.size(); i++){
        txEthCall = QtumTransaction(1300 + i, GASLIMIT, (GASLIMIT * dev::u256(1)), newAddressGen[i], codeCall, dev::u256(0));
        txEthCall.forceSender(SENDERADDRESS);
        txEthCall.setHashWith(hashCall);
        txEthCall.setNVout(i);

        txsCall.push_back(txEthCall);
        ++hashCall;
    }

    ByteCodeExec exec2(block, txsCall);
    exec2.performByteCode();
    std::vector<execResult> resCall = exec2.getResult();
    ByteCodeExecResult bceExecRes = exec2.processingResults();
    globalState->db().commit();

    std::unordered_map<dev::Address, dev::u256> addressesCall = globalState->addresses();

    BOOST_CHECK(resCall.size() == txs.size());
    BOOST_CHECK(addressesCall.size() == txs.size());
    for(size_t i =0; i < txs.size(); i++){
        BOOST_CHECK(resCall[i].first.excepted == dev::eth::TransactionException::None);
        BOOST_CHECK(resCall[i].first.newAddress == dev::Address());
        BOOST_CHECK(resCall[i].first.output == valtype());
        BOOST_CHECK(globalState->balance(newAddressGen[i]) == dev::u256(1300 + i));
    }

    BOOST_CHECK(bceExecRes.usedFee + bceExecRes.refundSender == GASLIMIT * 130);
    BOOST_CHECK(bceExecRes.usedFee == 2734810);
    BOOST_CHECK(bceExecRes.refundSender == 62265190);
    BOOST_CHECK(bceExecRes.refundVOuts.size() == 130);
    for(const CTxOut& out : bceExecRes.refundVOuts){
        BOOST_CHECK(out.scriptPubKey == CScript() << OP_DUP << OP_HASH160 << SENDERADDRESS.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
        BOOST_CHECK(out.nValue == 478963);
    }
    BOOST_CHECK(bceExecRes.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_call_contract_OutOfGas_transfer_many_return_value){
    initState();
    CBlock block(generateBlock());

    std::vector<dev::Address> newAddressGen;
    std::vector<QtumTransaction> txs;
    QtumTransaction txEth;
    valtype code(ParseHex("6060604052346000575b60448060166000396000f30060606040525b60165b5b6001156013576009565b5b565b0000a165627a7a7230582036699c18276860a335b2ba5670733c6b864f40fe3a36680866d9af2f8c2ef6ca0029"));
    txEth = QtumTransaction(0, GASLIMIT, (GASLIMIT * dev::u256(1)), code, dev::u256(0));
    txEth.forceSender(SENDERADDRESS);
    dev::h256 hash(HASHTX);
    for(size_t i = 0; i < 130; i++){
        txEth.setHashWith(hash);
        txEth.setNVout(i);

        newAddressGen.push_back(createQtumAddress(txEth.getHashWith(), txEth.getNVout()));
        txs.push_back(txEth);
        ++hash;
    }
    
    ByteCodeExec exec(block, txs);
    exec.performByteCode();
    std::vector<execResult> res = exec.getResult();
    globalState->db().commit();

    std::vector<QtumTransaction> txsCall;
    QtumTransaction txEthCall;
    valtype codeCall(ParseHex("00"));
    dev::h256 hashCall(HASHTX);
    for(size_t i = 0; i < txs.size(); i++){
        txEthCall = QtumTransaction(1300 + i, GASLIMIT, (GASLIMIT * dev::u256(1)), newAddressGen[i], codeCall, dev::u256(0));
        txEthCall.forceSender(SENDERADDRESS);
        txEthCall.setHashWith(hashCall);
        txEthCall.setNVout(i);

        txsCall.push_back(txEthCall);
        ++hashCall;
    }

    ByteCodeExec exec2(block, txsCall);
    exec2.performByteCode();
    std::vector<execResult> resCall = exec2.getResult();
    ByteCodeExecResult bceExecRes = exec2.processingResults();
    globalState->db().commit();

    std::unordered_map<dev::Address, dev::u256> addressesCall = globalState->addresses();

    BOOST_CHECK(resCall.size() == txs.size());
    BOOST_CHECK(addressesCall.size() == txs.size());
    for(size_t i =0; i < txs.size(); i++){
        BOOST_CHECK(resCall[i].first.excepted == dev::eth::TransactionException::OutOfGas);
        BOOST_CHECK(resCall[i].first.newAddress == dev::Address());
        BOOST_CHECK(resCall[i].first.output == valtype());
        BOOST_CHECK(globalState->balance(newAddressGen[i]) == dev::u256(0));
    }

    BOOST_CHECK(bceExecRes.usedFee + bceExecRes.refundSender == 0);
    BOOST_CHECK(bceExecRes.usedFee == 0);
    BOOST_CHECK(bceExecRes.refundSender == 0);
    BOOST_CHECK(bceExecRes.refundVOuts.size() == 0);
    BOOST_CHECK(bceExecRes.refundValueTx.size() == 130);
    size_t add = 0;
    for(const CTransaction& tx : bceExecRes.refundValueTx){
        BOOST_CHECK(tx.vout[0].nValue == CAmount(1300 + add));
        add++;
    }
}

BOOST_AUTO_TEST_CASE(bytecodeexec_suicide){
    initState();
    CBlock block(generateBlock());

    std::vector<QtumTransaction> txsCreate;
    std::vector<dev::Address> newAddresses;
    dev::h256 hash(HASHTX);
    for(size_t i = 0; i < 10; i++){
        QtumTransaction txEthCreate;
        if(i == 0){
            valtype code(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a723058208dcf9e9f355ea43af9d928d96e2197200e046d281d2e701af1027acf35415c100029"));
            txEthCreate = QtumTransaction(0, GASLIMIT, (GASLIMIT * dev::u256(1)), code, dev::u256(0));
        } else {
            valtype code(ParseHex("6060604052734de45add9f5f0b6887081cfcfe3aca6da9eb3365600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b5b5b60b68061006a6000396000f30060606040523615603d576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806341c0e1b5146045575b60435b5b565b005b604b604d565b005b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16ff5b5600a165627a7a72305820e296f585c72ea3d4dce6880122cfe387d26c48b7960676a52e811b56ef8297a80029"));
            txEthCreate = QtumTransaction(13, GASLIMIT, (GASLIMIT * dev::u256(1)), code, dev::u256(0));
        }
        txEthCreate.forceSender(SENDERADDRESS);
        txEthCreate.setHashWith(hash);
        txEthCreate.setNVout(i);
        txsCreate.push_back(txEthCreate);
        newAddresses.push_back(createQtumAddress(txEthCreate.getHashWith(), txEthCreate.getNVout()));
        ++hash;
    }

    ByteCodeExec exec(block, txsCreate);
    exec.performByteCode();
    std::vector<execResult> resCreate = exec.getResult();
    globalState->db().commit();

    std::unordered_map<dev::Address, dev::u256> addressesCreate = globalState->addresses();

    BOOST_CHECK(resCreate.size() == 10);
    BOOST_CHECK(addressesCreate.size() == 10);

    std::vector<QtumTransaction> txsCall;
    QtumTransaction txEthCall;
    valtype codeCall(ParseHex("41c0e1b5"));
    for(size_t i = 0; i < txsCreate.size() - 1; i++){
        txEthCall = QtumTransaction(0, GASLIMIT, (GASLIMIT * dev::u256(1)), newAddresses[i + 1], codeCall, dev::u256(0));
        txEthCall.forceSender(SENDERADDRESS);
        txEthCall.setHashWith(HASHTX);
        txEthCall.setNVout(0);

        txsCall.push_back(txEthCall);
    }

    ByteCodeExec exec2(block, txsCall);
    exec2.performByteCode();
    std::vector<execResult> resCall = exec2.getResult();
    ByteCodeExecResult bceExecRes = exec.processingResults();
    globalState->db().commit();

    std::unordered_map<dev::Address, dev::u256> addressesCall = globalState->addresses();

    BOOST_CHECK(resCall.size() == 9);
    BOOST_CHECK(addressesCall.size() == 1);
    for(size_t i = 0; i < resCall.size(); i++){
        BOOST_CHECK(resCall[i].first.excepted == dev::eth::TransactionException::None);
        BOOST_CHECK(resCall[i].first.newAddress == dev::Address());
        BOOST_CHECK(resCall[0].first.output == valtype());
    }
    BOOST_CHECK(globalState->balance(newAddresses[0]) == dev::u256(117));

    BOOST_CHECK(bceExecRes.usedFee + bceExecRes.refundSender == GASLIMIT * 10);
    BOOST_CHECK(bceExecRes.usedFee == 1207117);
    BOOST_CHECK(bceExecRes.refundSender == 3792883);
    BOOST_CHECK(bceExecRes.refundVOuts.size() == 10);
    for(size_t i = 0; i < 10; i++){
        BOOST_CHECK(bceExecRes.refundVOuts[i].scriptPubKey == CScript() << OP_DUP << OP_HASH160 << SENDERADDRESS.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
        if(i == 0){
            BOOST_CHECK(bceExecRes.refundVOuts[i].nValue == 430618);
            continue;
        }
        BOOST_CHECK(bceExecRes.refundVOuts[i].nValue == 373585);
    }
    BOOST_CHECK(bceExecRes.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_CASE(bytecodeexec_contract_create_contracts){
    initState();
    CBlock block(generateBlock());

    QtumTransaction txEthCreate;
    valtype code(ParseHex("606060405234610000575b61034a806100196000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680633f811b80146100495780636b8ff5741461006a575b610000565b3461000057610068600480803560001916906020019091905050610087565b005b3461000057610085600480803590602001909190505061015b565b005b60008160405160e18061023e833901808260001916600019168152602001915050604051809103906000f08015610000579050600180548060010182818154818355818115116101035781836000526020600020918201910161010291905b808211156100fe5760008160009055506001016100e6565b5090565b5b505050916000526020600020900160005b83909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550505b5050565b6000600182815481101561000057906000526020600020900160005b9054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690508073ffffffffffffffffffffffffffffffffffffffff16638052474d6000604051602001526040518163ffffffff167c0100000000000000000000000000000000000000000000000000000000028152600401809050602060405180830381600087803b156100005760325a03f1156100005750505060405180519050600083815481101561000057906000526020600020900160005b5081600019169055505b50505600606060405234610000576040516020806100e1833981016040528080519060200190919050505b80600081600019169055505b505b609f806100426000396000f30060606040523615603d576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638052474d146045575b60435b5b565b005b34600057604f606d565b60405180826000191660001916815260200191505060405180910390f35b600054815600a165627a7a7230582054caf5870995bc5d22259287e288b70bff8d1e814c0974f8c27d10976fa334c10029a165627a7a72305820005b7140037b71c850595a05796b31ff9b55aa0442237dd7607667567074fb1c0029"));
    txEthCreate = QtumTransaction(0, GASLIMIT, (GASLIMIT * dev::u256(1)), code, dev::u256(0));
    txEthCreate.forceSender(SENDERADDRESS);
    txEthCreate.setHashWith(HASHTX);
    txEthCreate.setNVout(0);

    std::vector<QtumTransaction> txs(1, txEthCreate);
    ByteCodeExec exec(block, txs);
    exec.performByteCode();
    std::vector<execResult> res = exec.getResult();
    globalState->db().commit();

    std::vector<QtumTransaction> txsCall;
    QtumTransaction txEthCall;
    valtype codeCall(ParseHex("3f811b80"));
    dev::Address newAddress(createQtumAddress(txEthCreate.getHashWith(), txEthCreate.getNVout()));
    for(size_t i = 0; i < 20; i++){
        txEthCall = QtumTransaction(0, GASLIMIT, (GASLIMIT * dev::u256(1)), newAddress, codeCall, dev::u256(0));
        txEthCall.forceSender(SENDERADDRESS);
        txEthCall.setHashWith(HASHTX);
        txEthCall.setNVout(i);
        txsCall.push_back(txEthCall);
    }

    ByteCodeExec exec2(block, txsCall);
    exec2.performByteCode();
    std::vector<execResult> resCall = exec2.getResult();
    ByteCodeExecResult bceExecRes = exec.processingResults();
    globalState->db().commit();

    std::unordered_map<dev::Address, dev::u256> addressesCall = globalState->addresses();

    BOOST_CHECK(resCall.size() == 20);
    BOOST_CHECK(addressesCall.size() == 21);
    BOOST_CHECK(globalState->balance(newAddress) == dev::u256(0));
    for(size_t i = 0; i < resCall.size(); i++){
        BOOST_CHECK(resCall[i].first.excepted == dev::eth::TransactionException::None);
        BOOST_CHECK(resCall[i].first.newAddress == dev::Address());
        BOOST_CHECK(resCall[i].first.output == valtype());
    }

    BOOST_CHECK(bceExecRes.usedFee + bceExecRes.refundSender == GASLIMIT);
    BOOST_CHECK(bceExecRes.usedFee == 270258);
    BOOST_CHECK(bceExecRes.refundSender == 229742);
    BOOST_CHECK(bceExecRes.refundVOuts.size() == 1);
    BOOST_CHECK(bceExecRes.refundValueTx.size() == 0);
}

BOOST_AUTO_TEST_SUITE_END()
