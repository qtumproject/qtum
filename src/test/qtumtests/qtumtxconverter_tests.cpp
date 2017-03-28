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

BOOST_FIXTURE_TEST_SUITE(qtumtxconverter_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(parse_txcreate){

    mempool.clear();

    TestMemPoolEntryHelper entry;
    CMutableTransaction tx1, tx2;
    tx1.vin.resize(1);
    tx1.vin[0].scriptSig = CScript() << OP_1;
    tx1.vin[0].prevout.hash = uint256();
    tx1.vin[0].prevout.n = 0;
    tx1.vout.resize(1);
    tx1.vout[0].nValue = value;
    tx1.vout[0].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << address << OP_EQUALVERIFY << OP_CHECKSIG;

    uint256 hashParentTx = tx1.GetHash(); // save this txid for later use
    mempool.addUnchecked(hashParentTx, entry.Fee(1000).Time(GetTime()).SpendsCoinbase(true).FromTx(tx1));

    tx2.vin.resize(1);
    tx2.vin[0].scriptSig = CScript() << OP_1;
    tx2.vin[0].prevout.hash = hashParentTx;
    tx2.vin[0].prevout.n = 0;
    tx2.vout.resize(1);
    tx2.vout[0].nValue = value;
    tx2.vout[0].scriptPubKey = CScript() << CScriptNum(1) << CScriptNum(int64_t(gasLimit)) << CScriptNum(int64_t(gasPrice)) << data << OP_CREATE;

    CTransaction transaction(tx2);
    QtumTxConverter converter(transaction, NULL);
    std::vector<QtumTransaction> result = converter.extractionQtumTransactions();

    BOOST_CHECK(result.size() == 1);
    BOOST_CHECK(result[0].isCreation());
    BOOST_CHECK(result[0].data() == data);
    BOOST_CHECK(result[0].value() == value);
    BOOST_CHECK(result[0].gasPrice() == gasPrice);
    BOOST_CHECK(result[0].gas() == gasLimit * gasPrice);
    BOOST_CHECK(result[0].sender() == dev::Address(address));
    BOOST_CHECK(result[0].receiveAddress() == dev::Address());
    BOOST_CHECK(result[0].getNVout() == 0);
    BOOST_CHECK(result[0].getHashWith() == uintToh256(tx2.GetHash()));
}

BOOST_AUTO_TEST_CASE(parse_txcall){

    mempool.clear();

    TestMemPoolEntryHelper entry;
    CMutableTransaction tx1, tx2;
    tx1.vin.resize(1);
    tx1.vin[0].scriptSig = CScript() << OP_1;
    tx1.vin[0].prevout.hash = uint256();
    tx1.vin[0].prevout.n = 0;
    tx1.vout.resize(1);
    tx1.vout[0].nValue = value;
    tx1.vout[0].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << address << OP_EQUALVERIFY << OP_CHECKSIG;

    uint256 hashParentTx = tx1.GetHash(); // save this txid for later use
    mempool.addUnchecked(hashParentTx, entry.Fee(1000).Time(GetTime()).SpendsCoinbase(true).FromTx(tx1));

    tx2.vin.resize(1);
    tx2.vin[0].scriptSig = CScript() << OP_1;
    tx2.vin[0].prevout.hash = hashParentTx;
    tx2.vin[0].prevout.n = 0;
    tx2.vout.resize(1);
    tx2.vout[0].nValue = value;
    tx2.vout[0].scriptPubKey = CScript() << CScriptNum(1) << CScriptNum(int64_t(gasLimit)) << CScriptNum(int64_t(gasPrice)) << data << address << OP_CALL;

    CTransaction transaction(tx2);
    QtumTxConverter converter(transaction, NULL);
    std::vector<QtumTransaction> result = converter.extractionQtumTransactions();

    BOOST_CHECK(result.size() == 1);
    BOOST_CHECK(!result[0].isCreation());
    BOOST_CHECK(result[0].data() == data);
    BOOST_CHECK(result[0].value() == value);
    BOOST_CHECK(result[0].gasPrice() == gasPrice);
    BOOST_CHECK(result[0].gas() == gasLimit * gasPrice);
    BOOST_CHECK(result[0].sender() == dev::Address(address));
    BOOST_CHECK(result[0].receiveAddress() == dev::Address(address));
    BOOST_CHECK(result[0].getNVout() == 0);
    BOOST_CHECK(result[0].getHashWith() == uintToh256(tx2.GetHash()));
}

BOOST_AUTO_TEST_CASE(parse_txcreate_many_vout){

    mempool.clear();

    TestMemPoolEntryHelper entry;
    CMutableTransaction tx1, tx2;
    tx1.vin.resize(1);
    tx1.vin[0].scriptSig = CScript() << OP_1;
    tx1.vin[0].prevout.hash = uint256();
    tx1.vin[0].prevout.n = 0;
    tx1.vout.resize(1);
    tx1.vout[0].nValue = value;
    tx1.vout[0].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << address << OP_EQUALVERIFY << OP_CHECKSIG;

    uint256 hashParentTx = tx1.GetHash(); // save this txid for later use
    mempool.addUnchecked(hashParentTx, entry.Fee(1000).Time(GetTime()).SpendsCoinbase(true).FromTx(tx1));

    tx2.vin.resize(1);
    tx2.vin[0].scriptSig = CScript() << OP_1;
    tx2.vin[0].prevout.hash = hashParentTx;
    tx2.vin[0].prevout.n = 0;
    tx2.vout.resize(113);
    for(size_t i = 0; i < tx2.vout.size(); i++){
        tx2.vout[i].nValue = value;
        tx2.vout[i].scriptPubKey = CScript() << CScriptNum(1) << CScriptNum(int64_t(gasLimit)) << CScriptNum(int64_t(gasPrice)) << data << OP_CREATE;
    }

    CTransaction transaction(tx2);
    QtumTxConverter converter(transaction, NULL);
    std::vector<QtumTransaction> result = converter.extractionQtumTransactions();

    BOOST_CHECK(result.size() == tx2.vout.size());
    for(size_t i = 0; i < tx2.vout.size(); i++){
        BOOST_CHECK(result[i].isCreation());
        BOOST_CHECK(result[i].data() == data);
        BOOST_CHECK(result[i].value() == value);
        BOOST_CHECK(result[i].gasPrice() == gasPrice);
        BOOST_CHECK(result[i].gas() == gasLimit * gasPrice);
        BOOST_CHECK(result[i].sender() == dev::Address(address));
        BOOST_CHECK(result[i].receiveAddress() == dev::Address());
        BOOST_CHECK(result[i].getNVout() == i);
        BOOST_CHECK(result[i].getHashWith() == uintToh256(tx2.GetHash()));
    }
}

BOOST_AUTO_TEST_CASE(parse_txcall_many_vout){

    mempool.clear();

    TestMemPoolEntryHelper entry;
    CMutableTransaction tx1, tx2;
    tx1.vin.resize(1);
    tx1.vin[0].scriptSig = CScript() << OP_1;
    tx1.vin[0].prevout.hash = uint256();
    tx1.vin[0].prevout.n = 0;
    tx1.vout.resize(1);
    tx1.vout[0].nValue = value;
    tx1.vout[0].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << address << OP_EQUALVERIFY << OP_CHECKSIG;

    uint256 hashParentTx = tx1.GetHash(); // save this txid for later use
    mempool.addUnchecked(hashParentTx, entry.Fee(1000).Time(GetTime()).SpendsCoinbase(true).FromTx(tx1));

    tx2.vin.resize(1);
    tx2.vin[0].scriptSig = CScript() << OP_1;
    tx2.vin[0].prevout.hash = hashParentTx;
    tx2.vin[0].prevout.n = 0;
    tx2.vout.resize(113);
    for(size_t i = 0; i < tx2.vout.size(); i++){
        tx2.vout[i].nValue = value;
        tx2.vout[i].scriptPubKey = CScript() << CScriptNum(1) << CScriptNum(int64_t(gasLimit)) << CScriptNum(int64_t(gasPrice)) << data << address << OP_CALL;
    }

    CTransaction transaction(tx2);
    QtumTxConverter converter(transaction, NULL);
    std::vector<QtumTransaction> result = converter.extractionQtumTransactions();

    BOOST_CHECK(result.size() == tx2.vout.size());
    for(size_t i = 0; i < tx2.vout.size(); i++){
        BOOST_CHECK(!result[i].isCreation());
        BOOST_CHECK(result[i].data() == data);
        BOOST_CHECK(result[i].value() == value);
        BOOST_CHECK(result[i].gasPrice() == gasPrice);
        BOOST_CHECK(result[i].gas() == gasLimit * gasPrice);
        BOOST_CHECK(result[i].sender() == dev::Address(address));
        BOOST_CHECK(result[i].receiveAddress() == dev::Address(address));
        BOOST_CHECK(result[i].getNVout() == i);
        BOOST_CHECK(result[i].getHashWith() == uintToh256(tx2.GetHash()));
    }
}

BOOST_AUTO_TEST_SUITE_END()
