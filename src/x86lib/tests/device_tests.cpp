#include "x86lib.h"
#include "x86test.h"


using namespace std;
using namespace x86Lib;



TEST_CASE("Memory device manager test", "[Memory]" ){

	MemorySystem Memory;

	RAMemory m0(0x1000, "m0");
	RAMemory m1(0x10000, "m1");
	RAMemory m2(0x100000, "m2");

	unsigned int m0_len = 0xFFF;
	unsigned int m1_len = 0xFFFF;
	unsigned int m2_len = 0xFFFFF;

    bool fExcp;

	fExcp = false;
	try{
		Memory.Add(0x00, m0_len, &m0);
		Memory.Add(0x10000,0x10000 + m1_len, &m1);
		Memory.Add(0x100000, 0x100000 + m2_len, &m2);
	}catch( ... ) {
		fExcp = true;
	}
	REQUIRE(fExcp == false);
	

    unsigned char cWrite,cRead;
	unsigned long address;
	address = 0x00;
    for(unsigned int i=0;i<m0_len;i++){
	  cWrite = i;
	  Memory.Write(address+i, 1, &cWrite);
      Memory.Read(address+i, 1, &cRead);
	  REQUIRE(cRead == cWrite);
	}

	address = 0x10000;
    for(unsigned int i=0;i<m1_len;i++){
		cWrite = i;
		Memory.Write(address+i, 1, &cWrite);
		Memory.Read(address+i, 1, &cRead);
		REQUIRE(cRead == cWrite);
	}

	address = 0x100000;
    for(unsigned int i=0;i<m2_len;i++){
		cWrite = i;
		Memory.Write(address+i, 1, &cWrite);
		Memory.Read(address+i, 1, &cRead);
		REQUIRE(cRead == cWrite);
	}


	fExcp = false;
	try{
		Memory.Remove(0x10000, 0x1FFFF);
		Memory.Remove(&m0);
		Memory.Remove(&m2);
	}catch( ... ) {
		fExcp = true;
	}
	REQUIRE(fExcp == false);
	
	fExcp = false;
	try{
		Memory.Add(0x00, m0_len, &m0);
		Memory.Add(0x10000, 0x10000 + m1_len, &m1);
		Memory.Add(0x100000, 0x100000 + m2_len, &m2);
	}catch( ... ) {
		fExcp = true;
	}
	REQUIRE(fExcp == false);


	address = 0x00;
    for(unsigned int i=0;i<m0_len;i++){
		cWrite = i;
		Memory.Write(address+i, 1, &cWrite);
		Memory.Read(address+i, 1, &cRead);
		REQUIRE(cRead == cWrite);
	}
	
	address = 0x10000;
    for(unsigned int i=0;i<m1_len;i++){
		cWrite = i;
		Memory.Write(address+i, 1, &cWrite);
		Memory.Read(address+i, 1, &cRead);
		REQUIRE(cRead == cWrite);
	}

	address = 0x100000;
    for(unsigned int i=0;i<m2_len;i++){
		cWrite = i;
		Memory.Write(address+i, 1, &cWrite);
		Memory.Read(address+i, 1, &cRead);
		REQUIRE(cRead == cWrite);
	}

	fExcp = false;
	try{
		Memory.RangeFree(0x00,0x1FFFFF);
	}catch( ... ) {
		fExcp = true;
	}
	REQUIRE(fExcp == false);


}




