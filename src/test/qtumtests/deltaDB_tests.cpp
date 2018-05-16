#include <boost/test/unit_test.hpp>
#include <qtumtests/test_utils.h>
#include <script/standard.h>
#include <qtum/qtumtransaction.h>


namespace deltaDBTest{


BOOST_FIXTURE_TEST_SUITE(deltaDB_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(byteCode_read_write_test){
	size_t nCacheSize=8;
	bool fWipe=false,ret;
	DeltaDB* pDeltaDB= new DeltaDB(nCacheSize,false,fWipe);
	UniversalAddress addr(X86,valtype(ParseHex("0c4c1d7375918557df2ef8f1d1f0b2329cb248a1")));
	valtype byteCode = valtype(ParseHex("bf5f1283000000000000000000000000c4c1d7375918557df2ef8f1d1f0b2329cb248a150000000000000000000000000000000000000000000000000000000000000002"));
	valtype byteCode2;
	
	ret = pDeltaDB->writeByteCode(addr, byteCode);
	BOOST_CHECK(ret);
	ret = pDeltaDB->readByteCode(addr, byteCode2);	
    BOOST_CHECK(byteCode2==byteCode);

    delete pDeltaDB;	
}

BOOST_AUTO_TEST_CASE(state_read_write_test){
	size_t nCacheSize=8;
	bool fWipe=false,ret;
	DeltaDB* pDeltaDB= new DeltaDB(nCacheSize,false,fWipe);
	UniversalAddress addr(X86,valtype(ParseHex("0c4c1d7375918557df2ef8f1d1f0b2329cb248a0")));
	valtype v = valtype(ParseHex("bf5f1e32000000000000000000000000c4c1d7375918557df2ef8f1d1f0b2329cb248a150000000000000000000000000000000000000000000000000000000000000002"));
	valtype v2;
	uint256 w(ParseHex("1c4c1d7375918557df2ef8f1d1f0b2329cb248a10c4c1d7370c4c1d73748a1488"));
	uint256 r;
	unsigned int blk;

	ret = pDeltaDB->writeState(addr, valtype(ParseHex("3c4c1d737591848a9")),v);
	BOOST_CHECK(ret);	
	ret = pDeltaDB->readState(addr, valtype(ParseHex("3c4c1d737591848a9")),v2);
	BOOST_CHECK(ret);	
    BOOST_CHECK(v2==v);
	
	ret = pDeltaDB->writeUpdatedKey(addr, valtype(ParseHex("7c4c1d737591848a3")), 111222, w);
	BOOST_CHECK(ret);
	ret = pDeltaDB->readUpdatedKey(addr, valtype(ParseHex("7c4c1d737591848a3")), blk, r);
	BOOST_CHECK(ret);
    BOOST_CHECK(blk==111222);
    BOOST_CHECK(r==w);
	
    delete pDeltaDB;
}

BOOST_AUTO_TEST_CASE(change_log_read_write_test){
	size_t nCacheSize=8;
	bool fWipe=false,ret;
	DeltaDB* pDeltaDB= new DeltaDB(nCacheSize,false,fWipe);
	UniversalAddress addr(X86,valtype(ParseHex("0c4c1d7375918557df2ef8f1d1f0b2329cb24851")));
	valtype v = valtype(ParseHex("bf5f1e83000000000000000000000000c4c1d7375918557df2ef8f1d1f0b2329cb248a150000000000000000000000000000000000000000000000000000000000000002"));
	valtype v2;
	valtype key = valtype(ParseHex("1c4c1d737591848a1"));
	uint256 h(ParseHex("0c4c1d7375918557df2ef8f1d1f0b2329cb248a10c4c1d7370c4c1d73748a1483"));
	uint64_t it=233333;
	uint64_t it2;
	unsigned int blk_num=111222,blk_num2;
	uint256 blk_hash(ParseHex("3333111115918557df2ef8f1d1f0b2329cb248a10c4c111170c4c1d73748a1485"));;
	uint256 blk_hash2;
	uint256 txid(ParseHex("1111111115918557df2ef8f1d1f0b2329cb248a10c4c111170c4c1d73748a1481"));
	uint256 txid2;
	unsigned int vout=3,vout2;

	ret = pDeltaDB->writeRawKey(addr, key,v);
	BOOST_CHECK(ret);	
	ret = pDeltaDB->readRawKey(addr, key,v2);
	BOOST_CHECK(ret);	
    BOOST_CHECK(v2==v);
	
	ret = pDeltaDB->writeCurrentIterator(addr, key, it);
	BOOST_CHECK(ret);
	ret = pDeltaDB->readCurrentIterator(addr, key, it2);
	BOOST_CHECK(ret);
    BOOST_CHECK(it==it2);

	ret = pDeltaDB->writeStateWithIterator(addr, key, it, v);
	BOOST_CHECK(ret);
	ret = pDeltaDB->readStateWithIterator(addr, key, it, v2);
	BOOST_CHECK(ret);
    BOOST_CHECK(v2==v);

	ret = pDeltaDB->writeInfoWithIterator(addr, key, it, blk_num, blk_hash, txid, vout);
	BOOST_CHECK(ret);
	ret = pDeltaDB->readInfoWithIterator(addr, key, it, blk_num2, blk_hash2, txid2, vout2);
	BOOST_CHECK(ret);
	BOOST_CHECK(blk_num2==blk_num && blk_hash2==blk_hash && txid2==txid && vout2==vout);

	ret = pDeltaDB->writeOldestIterator(addr, key, it, blk_num, blk_hash);
	BOOST_CHECK(ret);
	ret = pDeltaDB->readOldestIterator(addr, key, it2, blk_num2, blk_hash2);
	BOOST_CHECK(ret);
	BOOST_CHECK(it2==it && blk_num2==blk_num && blk_hash2==blk_hash);
		
    delete pDeltaDB;
}

BOOST_AUTO_TEST_SUITE_END()


}

