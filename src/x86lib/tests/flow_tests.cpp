#include "x86test.h"

TEST_CASE("op_jmp_relW", "[flow]"){
    x86Tester test;
    x86Checkpoint check = test.LoadCheckpoint();
    int offset = check.GetEIP();
    test.Assemble(
"jmp WORD test1\n" //0: 66 e9 xx xx
"nop\n" //4: 90
"test4:\n"
"nop\n" //5: 90
"test2:\n"
"jmp DWORD test4\n" //6: e9 xx xx xx xx
"nop\n" //B: 90
"nop\n" //C: 90
"test1:\n"
"jmp DWORD test3\n" //D: e9 xx xx xx xx
"test3:\n"
"nop\n" //12: 90
"jmp WORD test2\n" //13: 66 e9 xx xx
); 
    test.Run(1);
    check.SetEIP(offset + 0x0D);
    test.Compare(check, true);
    test.Run(1);
    check.SetEIP(offset + 0x12);
    test.Compare(check, true);
    test.Run(1);
    check.SetEIP(offset + 0x13);
    test.Compare(check, true);
    test.Run(1);
    check.SetEIP(offset + 0x06);
    test.Compare(check, true);
    test.Run(1);
    check.SetEIP(offset + 0x05);
    test.Compare(check, true);
    test.Run(1);
    check.SetEIP(offset + 0x06);
    test.Compare(check, true);
}

TEST_CASE("op_call_relW", "[flow]"){
    x86Tester test;
    x86Checkpoint tmp = test.LoadCheckpoint();
    int offset = tmp.GetEIP();
    int stack = tmp.Reg32(ESP);
    test.Assemble(
"call WORD test1\n" //0: 66 OP xx xx
"nop\n" //4: 90
"test4:\n"
"nop\n" //5: 90
"test2:\n"
"call DWORD test4\n" //6: OP xx xx xx xx
"nop\n" //B: 90
"nop\n" //C: 90
"test1:\n"
"call DWORD test3\n" //D: OP xx xx xx xx
"test3:\n"
"nop\n" //12: 90
"call WORD test2\n" //13: 66 OP xx xx
); 
    //we use a weird test pattern here because it's cumbersome to use the Set pattern
    test.Run(1); //call test1
    REQUIRE(test.Check().GetEIP() == offset + 0x0D);
    REQUIRE(test.Check().Reg32(ESP) == stack - 2);
    //note: push first decrements ESP by 4, and then puts data into [ESP]
    REQUIRE(test.Check().Stack16(0) == offset + 4);

    test.Run(1); // call test3
    REQUIRE(test.Check().GetEIP() == offset + 0x12);
    REQUIRE(test.Check().Reg32(ESP) == stack - 6);
    REQUIRE(test.Check().Stack32(0) == offset + 0x12);
    
    test.Run(1); //nop
    REQUIRE(test.Check().GetEIP() == offset + 0x13);
    REQUIRE(test.Check().Reg32(ESP) == stack - 6);
    
    test.Run(1); //call test2
    REQUIRE(test.Check().GetEIP() == offset + 0x06);
    REQUIRE(test.Check().Reg32(ESP) == stack - 8);
    REQUIRE(test.Check().Stack16(0) == offset + 0x17);
    
    test.Run(1); //call test4
    REQUIRE(test.Check().GetEIP() == offset + 0x05);
    REQUIRE(test.Check().Reg32(ESP) == stack - 12);
    REQUIRE(test.Check().Stack32(0) == offset + 0x0B);
    
    test.Run(1); //nop
    REQUIRE(test.Check().GetEIP() == offset + 0x06);
    REQUIRE(test.Check().Reg32(ESP) == stack - 12);

    test.Run(1); //call test4
    REQUIRE(test.Check().GetEIP() == offset + 0x05);
    REQUIRE(test.Check().Reg32(ESP) == stack - 16);
    REQUIRE(test.Check().Stack32(0) == offset + 0x0B);
}

TEST_CASE("op_call_rmW", "[flow]"){
    x86Tester test;
    x86Checkpoint tmp = test.LoadCheckpoint();
    int offset = tmp.GetEIP();
    int stack = tmp.Reg32(ESP);
    test.Apply(tmp);
    test.Run(
"mov eax, test1\n" //0: b8 0c 10 00 00
"call eax\n" //5: ff d0
"jmp _end\n" //7: eb 08
"nop\n"
"nop\n"
"nop\n"
"test1: \n"
"mov eax, 0x12345678\n"
); 
    REQUIRE(test.Check().Reg32(ESP) == stack - 4);
    REQUIRE(test.Check().Reg32(EAX) == 0x12345678);
    //note: push first decrements ESP by 4, and then puts data into [ESP]
    REQUIRE(test.Check().Stack32(0) == offset + 7);

    test.Apply(tmp);
    test.Run(
"mov eax, test1\n" //0: b8 0c 10 00 00
"o16 call eax\n" //5: 66 ff d0
"jmp _end\n" //8: eb 08
"nop\n"
"nop\n"
"nop\n"
"test1: \n"
"mov eax, 0x12345678\n"
); 
    REQUIRE(test.Check().Reg32(ESP) == stack - 2);
    REQUIRE(test.Check().Reg32(EAX) == 0x12345678);
    REQUIRE(test.Check().Stack16(0) == offset + 8);
}


TEST_CASE("op_retn", "[flow]"){
    x86Tester test;
    x86Checkpoint check = test.LoadCheckpoint();
    int offset = check.GetEIP();
    int stack = check.Reg32(ESP);
    check.AddReg32(ESP, -4);
    check.SetStack32(0, 0xFFFF1234);
    test.Apply(check);
    test.Run("ret 4", 1);
    REQUIRE(test.Check().GetEIP() == 0xFFFF1234);
    REQUIRE(test.Check().Reg32(ESP) == stack + 4);
    //reset and try short mode
    test.Apply(check);
    test.Run("o16 ret 4", 1);
    REQUIRE(test.Check().GetEIP() == 0x1234);
    REQUIRE(test.Check().Reg32(ESP) == stack + 2);
}
