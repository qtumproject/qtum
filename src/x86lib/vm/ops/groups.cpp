/**
Copyright (c) 2007 - 2009 Jordan "Earlz/hckr83" Earls  <http://www.Earlz.biz.tm>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This file is part of the x86Lib project.
**/
#include <x86lib.h>
namespace x86Lib{
using namespace std;


void x86CPU::op_group_80(){
	ModRM rm(this);
    opcodeExtra = rm.GetExtra();
	switch(rm.GetExtra()){
		case 5:
		op_sub_rm8_imm8(rm);
		break;
		case 0:
		op_add_rm8_imm8(rm);
		break;
		case 2:
		op_adc_rm8_imm8(rm);
		break;
		case 7:
		op_cmp_rm8_imm8(rm);
		break;
		case 1:
		op_or_rm8_imm8(rm);
		break;
		case 6:
		op_xor_rm8_imm8(rm);
		break;
		case 3:
		op_sbb_rm8_imm8(rm);
		break;
		case 4:
		op_and_rm8_imm8(rm);
		break;

		default:
		eip--; //to get actual opcode
		throw CpuInt_excp(UNK_IEXCP);
		break;
	}
	eip++; //for imm8

}

void x86CPU::op_group_81(){
	ModRM rm(this);
    opcodeExtra = rm.GetExtra();
	switch(rm.GetExtra()){
		case 5:
		op_sub_rmW_immW(rm);
		break;
		case 0:
		op_add_rmW_immW(rm);
		break;
		case 2:
		op_adc_rmW_immW(rm);
		break;
		case 7:
		op_cmp_rmW_immW(rm);
		break;
		case 1:
		op_or_rmW_immW(rm);
		break;
		case 6:
		op_xor_rmW_immW(rm);
		break;
		case 3:
		op_sbb_rmW_immW(rm);
		break;
		case 4:
		op_and_rmW_immW(rm);
		break;

		default:
		eip--;
		throw CpuInt_excp(UNK_IEXCP);
		break;
	}
	eip+=OperandSize(); //these each have immW
}

void x86CPU::op_group_82(){
	ModRM rm(this);
    opcodeExtra = rm.GetExtra();
	switch(rm.GetExtra()){
		case 0:
		op_add_rm8_imm8(rm);
		break;
		case 1:
		op_or_rm8_imm8(rm);
		break;
		case 2:
		op_adc_rm8_imm8(rm);
		break;
		case 3:
		op_sbb_rm8_imm8(rm);
		break;
		case 4:
		op_and_rm8_imm8(rm);
		break;
		case 5:
		op_sub_rm8_imm8(rm);
		break;
		case 6:
		op_xor_rm8_imm8(rm);
		break;
		case 7:
		op_cmp_rm8_imm8(rm);
		break;

		default:
		eip--;
		throw CpuInt_excp(UNK_IEXCP);
		break;
	}
	eip+=OperandSize(); 
}

void x86CPU::op_group_83() {
    ModRM rm(this);
    opcodeExtra = rm.GetExtra();
    switch (rm.GetExtra()) {
        case 0:
            op_add_rmW_imm8(rm);
            break;
        case 2:
            op_adc_rmW_imm8(rm);
            break;
        case 5:
            op_sub_rmW_imm8(rm);
            break;
        case 7:
            op_cmp_rmW_imm8(rm);
            break;
        case 1:
            op_or_rmW_imm8(rm);
            break;
        case 6:
            op_xor_rmW_imm8(rm);
            break;
        case 3:
            op_sbb_rmW_imm8(rm);
            break;
        case 4:
            op_and_rmW_imm8(rm);
            break;

        default:
            eip--;
            throw CpuInt_excp(UNK_IEXCP);
            break;
    }
    eip++;
}


void x86CPU::op_group_8F(){
	ModRM rm(this);
    opcodeExtra = rm.GetExtra();
	switch(rm.GetExtra()){
		case 0:
		op_pop_rmW(rm);
		break;

		default:
		eip--;
		throw CpuInt_excp(UNK_IEXCP);
		break;
	}
}

void x86CPU::op_group_F6(){
	ModRM rm(this);
    opcodeExtra = rm.GetExtra();
	switch(rm.GetExtra()){
		case 6:
		op_div_rm8(rm);
		break;
		case 7:
		op_idiv_rm8(rm);
		break;
		case 4:
		op_mul_rm8(rm);
		break;
		case 5:
		op_imul_rm8(rm);
		break;
		case 3:
		op_neg_rm8(rm);
		break;
		case 0:
		op_test_rm8_imm8(rm);
		eip++;
		break;
		case 2:
		op_not_rm8(rm);
		break;

		default:
		eip--;
		throw CpuInt_excp(UNK_IEXCP);
		break;
	}
}

void x86CPU::op_group_F7(){
	ModRM rm(this);
    opcodeExtra = rm.GetExtra();
	switch(rm.GetExtra()){
		case 6:
		if(OperandSize16){
			op16_div_rm16(rm);
		}else{
			op32_div_rm32(rm);
		}
		break;
		case 7:
		if(OperandSize16){
			op16_idiv_rm16(rm);
		}else{
			op32_idiv_rm32(rm);
		}
		break;
		case 4:
		if(OperandSize16){
			op16_mul_rm16(rm);
		}else{
			op32_mul_rm32(rm);
		}
		break;
		case 5:
		if(OperandSize16){
			op16_imul_rm16(rm);
		}else{
			op32_imul_rm32(rm);
		}
		break;
		case 3:
		op_neg_rmW(rm);
		break;
		case 0:
		op_test_rmW_immW(rm);
		eip+=OperandSize();
		break;
		case 2:
		op_not_rmW(rm);
		break;

		default:
		eip--;
		throw CpuInt_excp(UNK_IEXCP);
		break;
	}
}


void x86CPU::op_group_FF(){
	ModRM rm(this);
    opcodeExtra = rm.GetExtra();
	switch(rm.GetExtra()){
		case 4:
		rm.setJumpBehavior();
		op_jmp_rmW(rm);
		break;
		case 5:
		rm.setJumpBehavior();
		op_jmp_mF(rm);
		break;
		case 6:
		op_push_rmW(rm);
		break;
		case 0:
		op_inc_rmW(rm);
		break;
		case 1:
		op_dec_rmW(rm);
		break;
		case 2:
		rm.setJumpBehavior();
		op_call_rmW(rm);
		break;
		case 3:
		rm.setJumpBehavior();
		op_call_mF(rm);
		break;

		default:
		eip--;
		throw CpuInt_excp(UNK_IEXCP);
		break;
	}
}

void x86CPU::op_group_FE(){
	ModRM rm(this);
    opcodeExtra = rm.GetExtra();
	switch(rm.GetExtra()){
		case 0:
		op_inc_rm8(rm);
		break;
		case 1:
		op_dec_rm8(rm);
		break;

		default:
		eip--;
		throw CpuInt_excp(UNK_IEXCP);
		break;
	}
}

void x86CPU::op_group_D0(){
	ModRM rm(this);
    opcodeExtra = rm.GetExtra();
	switch(rm.GetExtra()){
		case 7:
		op_sar_rm8_1(rm);
		break;
		case 4:
		op_shl_rm8_1(rm);
		break;
		case 5:
		op_shr_rm8_1(rm);
		break;
		case 0:
		op_rol_rm8_1(rm);
		break;
		case 1:
		op_ror_rm8_1(rm);
		break;
		case 3:
		op_rcr_rm8_1(rm);
		break;
		case 2:
		op_rcl_rm8_1(rm);
		break;
		default:
		eip--;
		throw CpuInt_excp(UNK_IEXCP);
		break;
	}
}

void x86CPU::op_group_D1(){
	ModRM rm(this);
    opcodeExtra = rm.GetExtra();
	switch(rm.GetExtra()){
		case 7:
		op_sar_rmW_1(rm);
		break;
		case 4:
		op_shl_rmW_1(rm);
		break;
		case 5:
		op_shr_rmW_1(rm);
		break;
		case 0:
		op_rol_rmW_1(rm);
		break;
		case 1:
		op_ror_rmW_1(rm);
		break;
		case 3:
		op_rcr_rmW_1(rm);
		break;
		case 2:
		op_rcl_rmW_1(rm);
		break;


		default:
		eip--;
		throw CpuInt_excp(UNK_IEXCP);
		break;
	}
}


void x86CPU::op_group_D2(){
	ModRM rm(this);
    opcodeExtra = rm.GetExtra();
	switch(rm.GetExtra()){
		case 7:
		op_sar_rm8_cl(rm);
		break;
		case 4:
		op_shl_rm8_cl(rm);
		break;
		case 5:
		op_shr_rm8_cl(rm);
		break;
		case 0:
		op_rol_rm8_cl(rm);
		break;
		case 1:
		op_ror_rm8_cl(rm);
		break;
		case 3:
		op_rcr_rm8_cl(rm);
		break;
		case 2:
		op_rcl_rm8_cl(rm);
		break;


		default:
		eip--;
		throw CpuInt_excp(UNK_IEXCP);
		break;
	}
}

void x86CPU::op_group_D3(){
	ModRM rm(this);
    opcodeExtra = rm.GetExtra();
	switch(rm.GetExtra()){
		case 7:
		op_sar_rmW_cl(rm);
		break;
		case 4:
		op_shl_rmW_cl(rm);
		break;
		case 5:
		op_shr_rmW_cl(rm);
		break;
		case 0:
		op_rol_rmW_cl(rm);
		break;
		case 1:
		op_ror_rmW_cl(rm);
		break;
		case 3:
		op_rcr_rmW_cl(rm);
		break;
		case 2:
		op_rcl_rmW_cl(rm);
		break;


		default:
		eip--;
		throw CpuInt_excp(UNK_IEXCP);
		break;
	}
}

void x86CPU::op_group_C0(){
    ModRM rm(this);
    opcodeExtra = rm.GetExtra();
    switch(rm.GetExtra()){
        case 7:
		op_sar_rm8_imm8(rm);
		break;
		case 4:
		op_shl_rm8_imm8(rm);
		break;
		case 5:
		op_shr_rm8_imm8(rm);
		break;
		case 0:
		op_rol_rm8_imm8(rm);
		break;
		case 1:
		op_ror_rm8_imm8(rm);
		break;
		case 3:
		op_rcr_rm8_imm8(rm);
		break;
		case 2:
		op_rcl_rm8_imm8(rm);
        break;

        default:
        eip--;
		throw CpuInt_excp(UNK_IEXCP);
        break;
    }
	eip++; //for imm8
}

void x86CPU::op_group_C1(){
    ModRM rm(this);
    opcodeExtra = rm.GetExtra();
    switch(rm.GetExtra()){
        case 7:
		op_sar_rmW_imm8(rm);
		break;
		case 4:
		op_shl_rmW_imm8(rm);
		break;
		case 5:
		op_shr_rmW_imm8(rm);
		break;
		case 0:
		op_rol_rmW_imm8(rm);
		break;
		case 1:
		op_ror_rmW_imm8(rm);
		break;
		case 3:
		op_rcr_rmW_imm8(rm);
		break;
		case 2:
		op_rcl_rmW_imm8(rm);
        break;

        default:
        eip--;
		throw CpuInt_excp(UNK_IEXCP);
        break;
    }
	eip++; //for imm8
}


};



