#include "x86test.h"


TEST_CASE("etc op_bound_rW_mW", "[etc]") {
    x86Tester test;
    x86Checkpoint check = test.LoadCheckpoint();
    check.SetReg32(EBX, HIGH_SCRATCH_ADDRESS);
	check.SetHighScratch32(0,1);
	check.SetHighScratch32(4,100);
    test.Apply(check);

	bool fExcept=false;
	try{
	    test.Run(
		"mov eax, 8\n"
		"bound eax, [ebx]\n"
		"jmp _end\n");
	}catch(...){
		fExcept=true;
	}
	
    REQUIRE(fExcept == false);

}

TEST_CASE("etc op_pre_fs_override", "[etc]") {
		x86Tester test;
		test.Run(
	"mov AX, 0x01\n"
	"fs\n"
	"mov AX, 0x02\n"
	"jmp _end\n");		
	x86Checkpoint check = test.LoadCheckpoint();
    check.SetReg32(EAX, 0x02);
    test.Compare(check);
}

TEST_CASE("etc op_pre_gs_override", "[etc]") {
		x86Tester test;
		test.Run(
	"mov AX, 0x01\n"
	"gs\n"
	"mov AX, 0x02\n"
	"jmp _end\n");
		
	x86Checkpoint check = test.LoadCheckpoint();
    check.SetReg32(EAX, 0x02);
    test.Compare(check);
}

TEST_CASE("etc op_pre_cs_override", "[etc]") {
		x86Tester test;
		test.Run(
	"mov AX, 0x01\n"
	"cs\n"
	"mov AX, 0x02\n"
	"jmp _end\n");
		
	x86Checkpoint check = test.LoadCheckpoint();
    check.SetReg32(EAX, 0x02);
    test.Compare(check);
}

TEST_CASE("etc op_pre_ds_override", "[etc]") {
		x86Tester test;
		test.Run(
	"mov AX, 0x01\n"
	"ds\n"
	"mov AX, 0x02\n"
	"jmp _end\n");
		
	x86Checkpoint check = test.LoadCheckpoint();
    check.SetReg32(EAX, 0x02);
    test.Compare(check);
}

TEST_CASE("etc op_pre_es_override", "[etc]") {
		x86Tester test;
		test.Run(
	"mov AX, 0x01\n"
	"es\n"
	"mov AX, 0x02\n"
	"jmp _end\n");
		
	x86Checkpoint check = test.LoadCheckpoint();
    check.SetReg32(EAX, 0x02);
    test.Compare(check);
}

TEST_CASE("etc op_pre_ss_override", "[etc]") {
		x86Tester test;
		test.Run(
	"mov AX, 0x01\n"
	"ss\n"
	"mov AX, 0x02\n"
	"jmp _end\n");
		
	x86Checkpoint check = test.LoadCheckpoint();
    check.SetReg32(EAX, 0x02);
    test.Compare(check);
}


