#include <boost/test/unit_test.hpp>
#include <test/test_bitcoin.h>
#include <consensus/merkle.h>
#include <chainparams.h>
#include <miner.h>
#include <validation.h>

//Tests data
CAmount value(5000000000LL - 1000);
dev::u256 gasPrice(3);
dev::u256 gasLimit(655535);
std::vector<unsigned char> address(ParseHex("abababababababababababababababababababab"));
std::vector<unsigned char> data(ParseHex("6060604052346000575b60398060166000396000f30060606040525b600b5b5b565b0000a165627a7a72305820a5e02d6fa08a384e067a4c1f749729c502e7597980b427d287386aa006e49d6d0029"));

CMutableTransaction createTX(std::vector<CTxOut> vout, uint256 hashprev = uint256()){
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].prevout.hash = hashprev;
    tx.vin[0].prevout.n = 0;
    tx.vout.resize(1);
    tx.vout = vout;
    return tx;
}

void checkResult(bool isCreation, std::vector<QtumTransaction> results, uint256 hash){
    for(size_t i = 0; i < results.size(); i++){
        if(isCreation){
            BOOST_CHECK(results[i].isCreation());
            BOOST_CHECK(results[i].receiveAddress() == dev::Address());
        } else {
            BOOST_CHECK(!results[i].isCreation());
            BOOST_CHECK(results[i].receiveAddress() == dev::Address(address));
        }
        BOOST_CHECK(results[i].data() == data);
        BOOST_CHECK(results[i].value() == value);
        BOOST_CHECK(results[i].gasPrice() == gasPrice);
        BOOST_CHECK(results[i].gas() == gasLimit);
        BOOST_CHECK(results[i].sender() == dev::Address(address));
        BOOST_CHECK(results[i].getNVout() == i);
        BOOST_CHECK(results[i].getHashWith() == uintToh256(hash));
    }
}

void runTest(bool isCreation, size_t n, CScript& script1, CScript script2 = CScript()){
    mempool.clear();
    TestMemPoolEntryHelper entry;
    CMutableTransaction tx1, tx2;
    std::vector<CTxOut> outs1 = {CTxOut(value, CScript() << OP_DUP << OP_HASH160 << address << OP_EQUALVERIFY << OP_CHECKSIG)};
    tx1 = createTX(outs1);
    uint256 hashParentTx = tx1.GetHash(); // save this txid for later use
    mempool.addUnchecked(hashParentTx, entry.Fee(1000).Time(GetTime()).SpendsCoinbase(true).FromTx(tx1));
    std::vector<CTxOut> outs2;
    for(size_t i = 0; i < n; i++){
        if(script2 == CScript()){
            outs2.push_back(CTxOut(value, script1));
        } else {
            if(i < n / 2){
                outs2.push_back(CTxOut(value, script1));
            } else {
                outs2.push_back(CTxOut(value, script2));
            }
        }
    }    
    tx2 = createTX(outs2, hashParentTx);
    CTransaction transaction(tx2);
    QtumTxConverter converter(transaction, NULL);
    ExtractQtumTX qtumTx;
    BOOST_CHECK(converter.extractionQtumTransactions(qtumTx));
    std::vector<QtumTransaction> result = qtumTx.first;
    if(script2 == CScript()){
        BOOST_CHECK(result.size() == n);
    } else {
        BOOST_CHECK(result.size() == n / 2);
    }
    checkResult(isCreation, result, tx2.GetHash());
}

void runFailingTest(bool isCreation, size_t n, CScript& script1, CScript script2 = CScript()){
    mempool.clear();
    TestMemPoolEntryHelper entry;
    CMutableTransaction tx1, tx2;
    std::vector<CTxOut> outs1 = {CTxOut(value, CScript() << OP_DUP << OP_HASH160 << address << OP_EQUALVERIFY << OP_CHECKSIG)};
    tx1 = createTX(outs1);
    uint256 hashParentTx = tx1.GetHash(); // save this txid for later use
    mempool.addUnchecked(hashParentTx, entry.Fee(1000).Time(GetTime()).SpendsCoinbase(true).FromTx(tx1));
    std::vector<CTxOut> outs2;
    for(size_t i = 0; i < n; i++){
        if(script2 == CScript()){
            outs2.push_back(CTxOut(value, script1));
        } else {
            if(i < n / 2){
                outs2.push_back(CTxOut(value, script1));
            } else {
                outs2.push_back(CTxOut(value, script2));
            }
        }
    }
    tx2 = createTX(outs2, hashParentTx);
    CTransaction transaction(tx2);
    QtumTxConverter converter(transaction, NULL);
    ExtractQtumTX qtumTx;
    BOOST_CHECK(!converter.extractionQtumTransactions(qtumTx));
}

BOOST_FIXTURE_TEST_SUITE(qtumtxconverter_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(parse_txcreate){
    CScript script1 = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(int64_t(gasLimit)) << CScriptNum(int64_t(gasPrice)) << data << OP_CREATE;
    runTest(true, 1, script1);
}

BOOST_AUTO_TEST_CASE(parse_txcall){
    CScript script1 = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(int64_t(gasLimit)) << CScriptNum(int64_t(gasPrice)) << data << address << OP_CALL;
    runTest(false, 1, script1);
}

BOOST_AUTO_TEST_CASE(parse_txcall_mixed){
    CScript script1 = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(int64_t(gasLimit)) << CScriptNum(int64_t(gasPrice)) << data << address << OP_CALL;
    CScript script2 = CScript() << OP_TRUE;
    runTest(false, 1, script1, script2);
}

BOOST_AUTO_TEST_CASE(parse_txcreate_many_vout){
    CScript script1 = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(int64_t(gasLimit)) << CScriptNum(int64_t(gasPrice)) << data << OP_CREATE;
    runTest(true, 120, script1);
}
BOOST_AUTO_TEST_CASE(parse_txcreate_many_vout_mixed){
    CScript script1 = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(int64_t(gasLimit)) << CScriptNum(int64_t(gasPrice)) << data << OP_CREATE;
    CScript script2 = CScript() << OP_TRUE;
    runTest(true, 120, script1, script2);
}

BOOST_AUTO_TEST_CASE(parse_txcall_many_vout){
    CScript script1 = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(int64_t(gasLimit)) << CScriptNum(int64_t(gasPrice)) << data << address << OP_CALL;
    runTest(false, 120, script1);
}

BOOST_AUTO_TEST_CASE(parse_incorrect_txcreate_many){
    CScript script1 = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(int64_t(gasLimit)) << CScriptNum(int64_t(gasPrice)) << data << OP_CREATE;
    CScript script2 = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(int64_t(gasLimit)) << CScriptNum(int64_t(gasPrice)) << data << address << OP_CREATE;
    runFailingTest(true, 120, script1, script2);
}

BOOST_AUTO_TEST_CASE(parse_incorrect_txcreate_few){
    CScript script1 = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(int64_t(gasLimit)) << CScriptNum(int64_t(gasPrice)) << data << OP_CREATE;
    CScript script2 = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(int64_t(gasLimit)) << CScriptNum(int64_t(gasPrice)) << OP_CREATE;
    runFailingTest(true, 120, script1, script2);
}

BOOST_AUTO_TEST_CASE(parse_incorrect_txcall_many){
    CScript script1 = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(int64_t(gasLimit)) << CScriptNum(int64_t(gasPrice)) << data << address << OP_CALL;
    CScript script2 = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(int64_t(gasLimit)) << CScriptNum(int64_t(gasPrice)) << data << address << address << OP_CALL;
    runFailingTest(false, 120, script1, script2);
}

BOOST_AUTO_TEST_CASE(parse_incorrect_txcall_few){
    CScript script1 = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(int64_t(gasLimit)) << CScriptNum(int64_t(gasPrice)) << data << address << OP_CALL;
    CScript script2 = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(int64_t(gasLimit)) << CScriptNum(int64_t(gasPrice)) << data << OP_CALL;
    runFailingTest(false, 120, script1, script2);
}

BOOST_AUTO_TEST_CASE(parse_incorrect_txcall_overflow){
    CScript script1 = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(int64_t(gasLimit)) << CScriptNum(int64_t(gasPrice)) << data << address << OP_CALL;
    CScript script2 = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(int64_t(0x80841e0000000000)) << CScriptNum(int64_t(0x0100000000000000)) << data << address << OP_CALL;
    runFailingTest(false, 120, script1, script2);
}

BOOST_AUTO_TEST_SUITE_END()
