#include "x86test.h"

TEST_CASE( "8-bit Add flags are correct", "[flags]" ) {
    x86CPU cpu;
    cpu.Add8(0, 0);
    REQUIRE((int) cpu.freg.bits.zf);
    REQUIRE((int) !cpu.freg.bits.of);
    REQUIRE((int) !cpu.freg.bits.cf);
}

TEST_CASE("asm test", "[test]") {
    x86Tester test;
    x86Checkpoint check = test.LoadCheckpoint();
    test.Run("mov eax, 0x12345678");
    check.SetReg32(EAX, 0x12345678);
    check.AdvanceEIP(5);
    test.Compare(check, true);
}

TEST_CASE("setne setnz", "[flags]") {
    x86Tester test;
    x86Checkpoint check = test.LoadCheckpoint();
    test.Run("mov eax, 1\n"
             "sub eax, 0\n"
             "setne cl\n"
             "setnz bl\n"
             "sete dl");
    check.UnsetZF();
    check.SetReg32(EAX, 1);
    check.SetReg8(CL, 1);
    check.SetReg8(BL, 1);
    check.SetReg8(DL, 0);
    test.Compare(check);
}

TEST_CASE("sete setz", "[flags]") {
    x86Tester test;
    x86Checkpoint check = test.LoadCheckpoint();
    test.Run("mov eax, 1\n"
             "sub eax, 1\n"
             "sete cl\n"
             "setz bl\n");
    check.SetZF();
    check.SetPF();
    check.SetReg8(CL, 1);
    check.SetReg8(BL, 1);
    test.Compare(check);
}

TEST_CASE("seta setnbe", "[flags]") {
    x86Tester test;
    x86Checkpoint check = test.LoadCheckpoint();
    test.Run("mov eax, 1\n"
             "cmp eax, 0\n"
             "seta bl\n"
             "setnbe cl\n");
    check.SetReg32(EAX, 1);
    check.UnsetZF();
    check.UnsetCF();
    check.SetReg8(CL, 1);
    check.SetReg8(BL, 1);
    test.Compare(check);
}

TEST_CASE("setae setnc setnb", "[flags]") {
    x86Tester test;
    x86Checkpoint check = test.LoadCheckpoint();
    test.Run("mov eax, 1\n"
             "cmp eax, 0\n"
             "setae bl\n"
             "setnb cl\n"
             "setnc dl\n");
    check.SetReg32(EAX, 1);
    check.UnsetCF();
    check.SetReg8(CL, 1);
    check.SetReg8(BL, 1);
    check.SetReg8(DL, 1);
    test.Compare(check);
}

TEST_CASE("setb setc setnae", "[flags]") {
    x86Tester test;
    x86Checkpoint check = test.LoadCheckpoint();
    test.Run("mov eax, 1\n"
             "cmp eax, 2\n"
             "setb bl\n"
             "setc cl\n"
             "setnae dl\n");
    check.SetReg32(EAX, 1);
    check.UnsetZF();
    check.SetCF();
    check.SetPF();
    check.SetAF();
    check.SetSF();
    check.SetReg8(CL, 1);
    check.SetReg8(BL, 1);
    check.SetReg8(DL, 1);
    test.Compare(check);
}

TEST_CASE("setbe setna", "[flags]") {
    x86Tester test;
    x86Checkpoint check = test.LoadCheckpoint();
    test.Run("mov eax, 1\n"
             "cmp eax, 2\n"
             "setbe bl\n"
             "cmp eax, 1\n"
             "setna cl\n");
    check.SetReg32(EAX, 1);
    check.SetReg8(CL, 1);
    check.SetReg8(BL, 1);
    check.SetPF();
    check.SetZF();
    test.Compare(check);
}

TEST_CASE("seto setno", "[flags]") {
    x86Tester test;
    x86Checkpoint check = test.LoadCheckpoint();
    test.Run("mov al, 127\n"
             "add al, 127\n"
             "seto bl\n"
             "mov al, 1\n"
             "add al, 1\n"
             "setno cl\n");
    check.SetReg32(EAX, 2);
    check.SetReg8(CL, 1);
    check.SetReg8(BL, 1);
    test.Compare(check);
}

TEST_CASE("sets setns", "[flags]") {
    x86Tester test;
    x86Checkpoint check = test.LoadCheckpoint();
    test.Run("mov al, 1\n"
             "sub al, 2\n"
             "sets bl\n"
             "mov al, 2\n"
             "sub al, 1\n"
             "setno cl\n");
    check.SetReg32(EAX, 1);
    check.SetReg8(CL, 1);
    check.SetReg8(BL, 1);
    test.Compare(check);
}

TEST_CASE("bt", "[flags]") {
    x86Tester test;
    x86Checkpoint check = test.LoadCheckpoint();
    test.Run("mov eax, 0x1111\n"
             "mov ebx, 4\n"
             "bt eax, ebx\n");
    check.SetReg32(EAX, 0x1111);
    check.SetReg32(EBX, 4);
    check.SetCF();
    test.Compare(check);
}

TEST_CASE("bts", "[flags]") {
    x86Tester test;
    x86Checkpoint check = test.LoadCheckpoint();
    test.Run("mov eax, 0x11111111\n"
             "mov ebx, 35\n"
             "bts eax, ebx\n");
    check.SetReg32(EAX, 0x11111119);
    check.SetReg32(EBX, 35);
    check.UnsetCF();
    test.Compare(check);
}


