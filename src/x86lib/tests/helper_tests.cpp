#include "x86test.h"

TEST_CASE("CalculatePF", "[helpers]") {
    x86CPU cpu; //no need to run instructions
    REQUIRE(cpu.freg.bits.pf == 0);
    cpu.CalculatePF(0);
    REQUIRE(cpu.freg.bits.pf == 1);
    cpu.CalculatePF(1);
    REQUIRE(cpu.freg.bits.pf == 0);
    //0000 1110 1001 1001
    cpu.CalculatePF(0xE99);
    REQUIRE(cpu.freg.bits.pf == 1);
    cpu.CalculatePF(0xFF);
    REQUIRE(cpu.freg.bits.pf == 1);
    cpu.CalculatePF(0xFFFFFF01);
    REQUIRE(cpu.freg.bits.pf == 0);
}

TEST_CASE("CalculateSF", "[helpers]"){
    x86CPU cpu; //no need to run instructions
    REQUIRE(cpu.freg.bits.sf == 0);
    //8 bit tests
    cpu.CalculateSF8(-90);
    REQUIRE(cpu.freg.bits.sf == 1);
    cpu.CalculateSF8(0);
    REQUIRE(cpu.freg.bits.sf == 0);
    cpu.CalculateSF8(1);
    REQUIRE(cpu.freg.bits.sf == 0);
    cpu.CalculateSF8(-120);
    REQUIRE(cpu.freg.bits.sf == 1);
    //16 bit tests
    cpu.CalculateSF16(-9804);
    REQUIRE(cpu.freg.bits.sf == 1);
    cpu.CalculateSF16(32000);
    REQUIRE(cpu.freg.bits.sf == 0);
    cpu.CalculateSF16(0);
    REQUIRE(cpu.freg.bits.sf == 0);
    cpu.CalculateSF16(-1);
    REQUIRE(cpu.freg.bits.sf == 1);
    //32 bit tests
    cpu.CalculateSF32(- (0x777FFFFF));
    REQUIRE(cpu.freg.bits.sf == 1);
    cpu.CalculateSF32(0);
    REQUIRE(cpu.freg.bits.sf == 0);
    cpu.CalculateSF32(-1);
    REQUIRE(cpu.freg.bits.sf == 1);
    cpu.CalculateSF32(1);
    REQUIRE(cpu.freg.bits.sf == 0);
    cpu.CalculateSF32(0x0FFFFFFF);
    REQUIRE(cpu.freg.bits.sf == 0);
}