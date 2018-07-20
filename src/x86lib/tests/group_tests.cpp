#include "x86test.h"



TEST_CASE("etc op_group_82_ADD", "[etc]") {
    x86Tester test;

	bool fExcept=false;
	try{
	    test.Run(
		"MOV DL, 0x00\n"
		"ADD DL, 0x55\n"
		"jmp _end\n");
	}catch(...){
		fExcept=true;
	}
	
    REQUIRE(fExcept == false);
    x86Checkpoint check = test.LoadCheckpoint();
    check.SetReg32(EDX, 0x55);
    check.UnsetZF();
    check.UnsetCF();
    check.UnsetOF();
    test.Compare(check);

}

TEST_CASE("etc op_group_82_SUB", "[etc]") {
    x86Tester test;

	bool fExcept=false;
	try{
	    test.Run(
		"MOV DL, 0x55\n"
		"SUB DL, 0x00\n"
		"jmp _end\n");
	}catch(...){
		fExcept=true;
	}
	
    REQUIRE(fExcept == false);
    x86Checkpoint check = test.LoadCheckpoint();
    check.SetReg32(EDX, 0x55);
    check.UnsetZF();
    check.UnsetCF();
    check.UnsetOF();
    test.Compare(check);

}

TEST_CASE("etc op_group_82_AND", "[etc]") {
    x86Tester test;

	bool fExcept=false;
	try{
	    test.Run(
		"MOV DL, 0xFF\n"
		"AND DL, 0x55\n"
		"jmp _end\n");
	}catch(...){
		fExcept=true;
	}
	
    REQUIRE(fExcept == false);
    x86Checkpoint check = test.LoadCheckpoint();
    check.SetReg32(EDX, 0x55);
    check.UnsetZF();
    check.UnsetCF();
    check.UnsetOF();
    test.Compare(check);

}

TEST_CASE("etc op_group_82_OR", "[etc]") {
    x86Tester test;

	bool fExcept=false;
	try{
	    test.Run(
		"MOV DL, 0xFF\n"
		"OR DL, 0x55\n"
		"jmp _end\n");
	}catch(...){
		fExcept=true;
	}
	
    REQUIRE(fExcept == false);
    x86Checkpoint check = test.LoadCheckpoint();
    check.SetReg32(EDX, 0xFF);
    check.UnsetZF();
    check.UnsetCF();
    check.UnsetOF();
    test.Compare(check);

}

TEST_CASE("etc op_group_82_CMP", "[etc]") {
    x86Tester test;

	bool fExcept=false;
	try{
	    test.Run(
		"MOV DL, 0x56\n"
		"CMP DL, 0x55\n"
		"jmp _end\n");
	}catch(...){
		fExcept=true;
	}
	
    REQUIRE(fExcept == false);
    x86Checkpoint check = test.LoadCheckpoint();
    check.UnsetZF();
    check.UnsetCF();
    check.UnsetOF();
    test.Compare(check);

}

TEST_CASE("etc op_group_82_XOR", "[etc]") {
    x86Tester test;

	bool fExcept=false;
	try{
	    test.Run(
		"MOV DL, 0xFF\n"
		"XOR DL, 0x01\n"
		"jmp _end\n");
	}catch(...){
		fExcept=true;
	}
	
    REQUIRE(fExcept == false);
    x86Checkpoint check = test.LoadCheckpoint();
    check.SetReg32(EDX, 0xFE);
    check.UnsetZF();
    check.UnsetCF();
    check.UnsetOF();
    test.Compare(check);

}

TEST_CASE("etc op_group_82_SBB", "[etc]") {
    x86Tester test;

	bool fExcept=false;
	try{
	    test.Run(
		"MOV DL, 0x21\n"
		"SBB DL, 0x01\n"
		"jmp _end\n");
	}catch(...){
		fExcept=true;
	}
	
    REQUIRE(fExcept == false);
    x86Checkpoint check = test.LoadCheckpoint();
    check.SetReg32(EDX, 0x20);
    check.UnsetZF();
    check.UnsetCF();
    check.UnsetOF();
    test.Compare(check);

}


