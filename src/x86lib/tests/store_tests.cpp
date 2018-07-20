#include "x86test.h"

TEST_CASE("op_enter", "[store]") {
    x86Tester test;
    x86Checkpoint check = test.LoadCheckpoint();
    int stack = check.Reg32(ESP);
    test.Assemble(//"mov eax, 0\n"
                  //"add ax, 0x1234\n"
                  "enter 0x800,0x0\n"
                  "leave\n"
                  "enter 0x400,1\n"
                  "leave\n"
                  "enter 0x800,15\n"
                  "leave\n");
    test.Run(1);
    REQUIRE(test.Check().Reg32(ESP) == stack - 4 - 0x800);
    test.Run(1);
    REQUIRE(test.Check().Reg32(ESP) == stack);
    test.Run(1);
    REQUIRE(test.Check().Reg32(ESP) == stack - 4 - 0x400);
    test.Run(1);
    REQUIRE(test.Check().Reg32(ESP) == stack);
    test.Run(1);
    REQUIRE(test.Check().Reg32(ESP) == stack - 4 - 0x800);
    test.Run(1);
    REQUIRE(test.Check().Reg32(ESP) == stack);
}
