/**
Copyright (c) 2007 - 2010 Jordan "Earlz/hckr83" Earls  <http://www.Earlz.biz.tm>
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
#include <iostream>
#include <x86lib.h>
#include <sstream>


namespace x86Lib{
//The lack of indentation for namespaces is intentional...
using namespace std;


x86CPU::x86CPU(uint32_t cpu_level_,uint32_t flags){
	if(cpu_level_==0){
		cpu_level=CPU286_REAL; //The default CPU right now..
	}else{
		cpu_level=cpu_level_;
	}
	Init();
}
x86CPU::x86CPU(x86SaveData &load_data,uint32_t flags){
	Init();
	LoadState(load_data);
}

void x86CPU::Init(){
	Reset();
}


void x86CPU::Reset(){
	/**Initialize register pointers**/

	busmaster=0;
	//assumes pmem and ports are still the same...
	InitOpcodes();
    Opcodes = opcodes_hosted; //for smart contracts, don't use this as a full VM, just straight to 32bit mode
    Opcodes_ext = opcodes_hosted_ext;
	uint32_t i;
	for(i=0;i<8;i++){
		regs32[i]=0;
	}
	for(i=0;i<7;i++){
		seg[i]=0;
	}
	ResetSegments();
	eip=0x1000;
	seg[cCS]=0;
	freg.data=0;
    regs32[ESP] = 0x200800; //set stack to reasonable address for Qtum
	string_compares=0;
	int_pending=0;
	cli_count=0;
    OperandSize16=false;
    AddressSize16=false;
    DoStop=false;
    PrefixCount = 0;
}


void x86CPU::SaveState(x86SaveData *save){
	uint32_t i;
	for(i=0;i<8;i++){
		save->regs32[i]=regs32[i];
	}
	for(i=0;i<7;i++){
		save->seg[i]=seg[i];
	}
	save->freg=freg;
	save->eip=eip;
	save->seg_route[cES]=ES;
	save->seg_route[cCS]=CS;
	save->seg_route[cDS]=DS;
	save->seg_route[cSS]=SS;
	save->seg_route[cFS]=FS;
	save->seg_route[cGS]=GS;
}

void x86CPU::LoadState(x86SaveData &load){
	uint32_t i;
	Reset();
	for(i=0;i<8;i++){
		regs32[i]=load.regs32[i];
	}
	for(i=0;i<7;i++){
		seg[i]=load.seg[i];
	}
	freg=load.freg;
	eip=load.eip;
	ES=load.seg_route[cES];
	CS=load.seg_route[cCS];
	DS=load.seg_route[cDS];
	SS=load.seg_route[cSS];
	GS=load.seg_route[cGS];
	FS=load.seg_route[cFS];
}




void x86CPU::DumpState(ostream &output){
	output << "EAX: "<< hex << regs32[EAX] <<endl;
	output << "ECX: "<< hex << regs32[ECX] <<endl;
	output << "EDX: "<< hex << regs32[EDX] <<endl;
	output << "EBX: "<< hex << regs32[EBX] <<endl;
	output << "ESP: "<< hex << regs32[ESP] <<endl;
	output << "EBP: "<< hex << regs32[EBP] <<endl;
	output << "ESI: "<< hex << regs32[ESI] <<endl;
	output << "EDI: "<< hex << regs32[EDI] <<endl;

	output << "CS: " << hex << seg[cCS] << endl;
	output << "SS: " << hex << seg[cSS] << endl;
	output << "DS: " << hex << seg[cDS] << endl;
	output << "ES: " << hex << seg[cES] << endl;
	output << "FS: " << hex << seg[cFS] << endl;
	output << "GS: " << hex << seg[cGS] << endl;
	output << "EIP: " << hex << eip << endl;

	output << "--Flags:" <<hex << freg.data << endl;
	output << "CF: " << (int)freg.bits.cf << endl;
	output << "PF: " << (int)freg.bits.pf << endl;
	output << "AF: " << (int)freg.bits.af << endl;
	output << "ZF: " << (int)freg.bits.zf << endl;
	output << "SF: " << (int)freg.bits.sf << endl;
	output << "TF: " << (int)freg.bits.tf << endl;
	output << "IF: " << (int)freg.bits._if << endl;
	output << "DF: " << (int)freg.bits.df << endl;
	output << "OF: " << (int)freg.bits.of << endl;
}

void x86CPU::Int(uint8_t num){ //external interrupt
	int_pending=1;
	int_number=num;
}

bool x86CPU::IntPending(){
	return int_pending;
}



int x86CPU::CheckInterrupts(){
	//possible bug here. What if we have a pop SS without an interrupt waiting? Won't interrupts be forever ignored?
	if(int_pending==0){return 0;} //quickly get out of this, this is in the main Cycle loop, so needs to be very speedy...
	if((int_pending==1) && (int_number==2)){ //NMI
		eip--;
		Int16(2);
		eip++;
		int_pending=0;
		return 1;
	}
	if(freg.bits._if==0){
		if(cli_count>1){
			cli_count--;
		}else{
			if(cli_count!=0){
				freg.bits._if=1;
			}
		}
	}else{
		if(int_pending==1){
			eip--;
			Int16(int_number);
			eip++;
			int_pending=0;
			return 1;
		}
	}
	return 0;
}

void x86CPU::Exec(int cyclecount){
	int i=0;
	bool done=false;
	while(!done){
		try{
			for(;i<cyclecount;i++){
				Cycle();
                if(DoStop){
                    DoStop=false;
                    return;
                }
			}
		}
		catch(CpuInt_excp err){
			err.code&=0x00FF;
			switch(err.code){
				case 0: //division by zero
				case 1: //debug exception
				case 2: //NMI
				case 3: //breakpoint
				case 4: //overflow

				case 7: //device unavailable
				Int16(err.code);
				eip++; //undo the decrement by Int
				break;
                case 6: //unknown opcode
                    throw CPUFaultException("Unknown opcode",(err.code|0xF000)|TRIPLE_FAULT_EXCP);
				case 5: //(186+ bounds check)
				if(cpu_level >= CPU186){
					Int16(err.code);
					eip++;
					break;
				}
				default:
				throw CPUFaultException("16bit Faults",(err.code|0xF000)|TRIPLE_FAULT_EXCP);
			}
		}
		catch(MemoryException err){
            std::ostringstream oss;
            oss << "Memory error at 0x" << std::hex << err.address;
			throw CPUFaultException(oss.str(),TRIPLE_FAULT_EXCP);
		}
		if(i>=cyclecount){
			done=true;
		}
	}
}

void x86CPU::op_ext_0F(){
    eip++;
    opbyte = ReadCode8(0);
    (this->*opcodes_hosted_ext[opbyte])();
}

void x86CPU::Cycle(){
#ifdef ENABLE_OPCODE_CALLBACK
	if(EachOpcodeCallback!=NULL){
		(*EachOpcodeCallback)(this);
	}
#endif
	CheckInterrupts();
    wherebeen.push_back((uint32_t)eip);
    //note this bit for 0x0F checking could probably be better in this very critical loop
    beginEIP = eip;
    opbyte = ReadCode8(0);
    opcodeExtra = -1;
    if(opbyte == 0x0F){
        //two byte opcode
        eip++;
        #ifdef QTUM_DEBUG
        lastOpcode = (0x0F << 8) || opbyte;
        lastOpcodeStr = opcodes_hosted_ext_str[opbyte];
        #endif
        opbyte = ReadCode8(0);
        (this->*Opcodes_ext[opbyte])(); //if in 32-bit mode, then go to 16-bit opcode
    }else {
        #ifdef QTUM_DEBUG
        lastOpcode = opbyte;
        lastOpcodeStr = opcodes_hosted_str[opbyte];
        #endif
        //operate on the this class with the opcode functions in this class
        (this->*Opcodes[opbyte])();
    }
	eip=eip+1;
}







void x86CPU::InstallOp(uint8_t num,opcode func, opcode *opcode_table){
    if(opcode_table){
        opcode_table[num]=func;
    }else {
        Opcodes[num] = func;
    }
}


void x86CPU::op_ext_group(){
	ModRM rm(this);
    opcodeExtra = rm.GetExtra();
	(this->*opcodes_hosted_ext_group[opbyte][rm.GetExtra()])(rm);
}

void x86CPU::op_na(){
    throw new CpuInt_excp(13); //opcode unsupported so throw a GPF
}

//NOTE: This can only be used for group opcodes in the 0x0F extended opcodes set
void x86CPU::InstallExtGroupOp(uint8_t opcode, uint8_t r_op, groupOpcode func){
	opcodes_hosted_ext[opcode] = &x86CPU::op_ext_group;
	opcodes_hosted_ext_group[opcode][r_op] = func;
}

#define STRINGIFY(x) #x
#define op(n, f) InstallOp(n, &x86CPU::f, opcodes_hosted); opcodes_hosted_str[n]=STRINGIFY(f)
#define opx(n, f) InstallOp(n, &x86CPU::f, opcodes_hosted_ext); opcodes_hosted_ext_str[n]=STRINGIFY(f)
#define opxg(n, i, f) InstallExtGroupOp(n, i, &x86CPU::f); opcodes_hosted_ext_group_str[n][i]=STRINGIFY(f)

void x86CPU::InitOpcodes(){
	Opcodes=opcodes_hosted;
    Opcodes_ext=opcodes_hosted_ext;

    //init all to unknown
    for(int i=0;i<256;i++){
        op(i, op_unknown);
        for(int j=0;j<8;j++){
            opxg(i, j, op_unknownG);
        }
        opx(i, op_unknown);
    }

    op(0x00, op_add_rm8_r8);
    op(0x01, op_add_rmW_rW);
    op(0x02, op_add_r8_rm8);
    op(0x03, op_add_rW_rmW);
    op(0x04, op_add_al_imm8);
    op(0x05, op_add_axW_immW);
    op(0x06, op_push_es);
    op(0x07, op_pop_es);
    op(0x08, op_or_rm8_r8);
    op(0x09, op_or_rmW_rW);
    op(0x0A, op_or_r8_rm8);
    op(0x0B, op_or_rW_rmW);
    op(0x0C, op_or_al_imm8);
    op(0x0D, op_or_axW_immW);
    op(0x0E, op_push_cs);
    op(0x0F, op_ext_0F); //this is usually handled in Cycle, but for prefixes etc, we need this
    //0x0F reserved for 2 byte opcodes
    op(0x10, op_adc_rm8_r8);
    op(0x11, op_adc_rmW_rW);
    op(0x12, op_adc_r8_rm8);
    op(0x13, op_adc_rW_rmW);
    op(0x14, op_adc_al_imm8);
    op(0x15, op_adc_axW_immW);
    op(0x16, op_push_ss);
    op(0x17, op_pop_ss);
    op(0x18, op_sbb_rm8_r8);
    op(0x19, op_sbb_rmW_rW);
    op(0x1A, op_sbb_r8_rm8);
    op(0x1B, op_sbb_rW_rmW);
    op(0x1C, op_sbb_al_imm8);
    op(0x1D, op_sbb_axW_immW);
    op(0x1E, op_push_ds);
    op(0x1F, op_pop_ds);
    op(0x20, op_and_rm8_r8);
    op(0x21, op_and_rmW_rW);
    op(0x22, op_and_r8_rm8);
    op(0x23, op_and_rW_rmW);
    op(0x24, op_and_al_imm8);
    op(0x25, op_and_axW_immW);
    op(0x26, op_pre_es_override);
    op(0x27, op_daa);
    op(0x28, op_sub_rm8_r8);
    op(0x29, op_sub_rmW_rW);
    op(0x2A, op_sub_r8_rm8);
    op(0x2B, op_sub_rW_rmW);
    op(0x2C, op_sub_al_imm8);
    op(0x2D, op_sub_axW_immW);
    op(0x2E, op_pre_cs_override);
    op(0x2F, op_das);
    op(0x30, op_xor_rm8_r8);
    op(0x31, op_xor_rmW_rW);
    op(0x32, op_xor_r8_rm8);
    op(0x33, op_xor_rW_rmW);
    op(0x34, op_xor_al_imm8);
    op(0x35, op_xor_axW_immW);
    op(0x36, op_pre_ss_override);
    op(0x37, op_aaa);
    op(0x38, op_cmp_rm8_r8);
    op(0x39, op_cmp_rmW_rW);
    op(0x3A, op_cmp_r8_rm8);
    op(0x3B, op_cmp_rW_rmW);
    op(0x3C, op_cmp_al_imm8);
    op(0x3D, op_cmp_axW_immW);
    op(0x3E, op_pre_ds_override);
    op(0x3F, op_aas);
    for(int i=0;i<8;i++){
    	op(0x40+i, op_inc_rW);
    	op(0x48+i, op_dec_rW);
    	op(0x50+i, op_push_rW);
    	op(0x58+i, op_pop_rW);
    }
    op(0x60, op_pushaW); //186
    op(0x61, op_popaW); //186
    op(0x62, op_bound_rW_mW); //186
    //op(0x63, op_arpl_rmW_rW); //286 (priv?)
    op(0x64, op_pre_fs_override); //386
    op(0x65, op_pre_gs_override); //386
    //66 is SSE2 escape. 67 is UD
    op(0x66, op_operand_override);
    op(0x67, op_address_override);
    op(0x68, op_push_immW);
    op(0x69, op_imul_rW_rmW_immW); //186 (note: uses /r for rW)
    op(0x6A, op_push_imm8);
    op(0x6B, op_imul_rW_rmW_imm8); //186 (note: uses /r for rW, imm8 is sign extended)
    op(0x6C, op_insb_m8_dx); //186
    op(0x6D, op_insW_mW_dx); //186
    op(0x6E, op_outsb_dx_m8); //186
    op(0x6F, op_outsW_dx_mW); //186
    for(int i=0;i<16;i++){
    	op(0x70+i, op_jcc_rel8);
    }
    op(0x80, op_group_80);
    op(0x81, op_group_81);
    op(0x82, op_group_82);
    op(0x83, op_group_83); // /1, /4, and /6 is 386
    op(0x84, op_test_rm8_r8);
    op(0x85, op_test_rmW_rW);
    op(0x86, op_xchg_rm8_r8);
    op(0x87, op_xchg_rmW_rW);
    op(0x88, op_mov_rm8_r8);
    op(0x89, op_mov_rmW_rW);
    op(0x8A, op_mov_r8_rm8);
    op(0x8B, op_mov_rW_rmW);
    op(0x8C, op_mov_rm16_sr);
    op(0x8D, op_lea);
    op(0x8E, op_mov_sr_rm16);
    op(0x8F, op_group_8F); //pop_rmW is only opcode included here
    op(0x90, op_nop); //nop is xchg_ax_ax
    for(int i=1;i<8;i++){
    	op(0x90+i, op_xchg_axW_rW);
    }
    op(0x98, op_cbW); //cbw/cwde
    op(0x99, op_cwE); //cwd/cdq
    op(0x9A, op_call_immF);
    op(0x9B, op_wait);
    op(0x9C, op_pushf);
    op(0x9D, op_popf);
    op(0x9E, op_sahf);
    op(0x9F, op_lahf);
    op(0xA0, op_mov_al_m8);
    op(0xA1, op_mov_axW_mW);
    op(0xA2, op_mov_m8_al);
    op(0xA3, op_mov_mW_axW);
    op(0xA4, op_movsb);
    op(0xA5, op_movsW);
    op(0xA6, op_cmpsb);
    op(0xA7, op_cmpsW);
    op(0xA8, op_test_al_imm8);
    op(0xA9, op_test_axW_immW);
    op(0xAA, op_stosb);
    op(0xAB, op_stosW);
    op(0xAC, op_lodsb);
    op(0xAD, op_lodsW);
    op(0xAE, op_scasb);
    op(0xAF, op_scasW);
    for(int i=0;i<8;i++){
    	op(0xB0+i, op_mov_r8_imm8);
    	op(0xB8+i, op_mov_rW_immW);
    }
    op(0xC0, op_group_C0); //186
    // C0 group: _rm8_imm8; rol, ror, rcl, rcr, shl/sal, shr, sal/shl(?), sar
    op(0xC1, op_group_C1); //186
    // C1 group: _rmW_imm8; rol, ror, rcl, rcr, shl/sal, shr, sal/shl, sar
    op(0xC2, op_retn_imm16); //???
    op(0xC3, op_retn);
    op(0xC4, op_les);
    op(0xC5, op_lds);
    op(0xC6, op_mov_rm8_imm8);
    op(0xC7, op_mov_rmW_immW);
    op(0xC8, op_enter); //186
    op(0xC9, op_leave); //186
    op(0xCA, op_retf_imm16); //???
    op(0xCB, op_retf);
    op(0xCC, op_int3);
    op(0xCD, op_int_imm8);
    op(0xCE, op_into);
    op(0xCF, op_iret);
    op(0xD0, op_group_D0);
    // D0 group: _rm8_1; rol, ror, rcl, rcr, shl/sal, shr, sal/shl, sar
    op(0xD1, op_group_D1);
    // D1 group: _rmW_1; rol, ror, rcl, rcr, shl/sal, shr, sal/shl, sar
    op(0xD2, op_group_D2);
    // D2 group: _rm8_cl; rol, ror, rcl, rcr, shl/sal, shr, sal/shl, sar
    op(0xD3, op_group_D3);
    // D3 group: _rmW_cl; rol, ror, rcl, rcr, shl/sal, shr, sal/shl, sar
    op(0xD4, op_aam_imm8); //also known as AMX (AAM without 0x0A argument is undocumented)
    op(0xD5, op_aad_imm8); //also known as ADX (AAD without 0x0A argument is undocumented)
    op(0xD6, op_salc); //undocumented
    op(0xD7, op_xlatb);
    //op(0xD8, opfpu_group_D8); //FPU only
    // D8 group: _st_m32; fadd, fmul, fcom, fcomp, fsub, fsubr, fdiv, fdivr
    //op(0xD9, opfpu_group_D9);
    // D9 group: many many FPU-only opcodes
    //op(0xDA, opfpu_group_DA);
    //op(0xDB, opfpu_group_DB);
    //op(0xDC, opfpu_group_DC);
    //op(0xDD, opfpu_group_DD);
    //op(0xDE, opfpu_group_DE);
    //op(0xDF, opfpu_group_DF);

    op(0xE0, op_loopcc_rel8); //loopnz
    op(0xE1, op_loopcc_rel8); //loopz
    op(0xE2, op_loopcc_rel8); //loop
    op(0xE3, op_jcxzW_rel8);
    op(0xE4, op_in_al_imm8);
    op(0xE5, op_in_axW_imm8);
    op(0xE6, op_out_imm8_al);
    op(0xE7, op_out_imm8_axW);
    op(0xE8, op_call_relW);
    op(0xE9, op_jmp_relW);
    op(0xEA, op_jmp_immF);
    op(0xEB, op_jmp_rel8);
    op(0xEC, op_in_al_dx);
    op(0xED, op_in_axW_dx);
    op(0xEE, op_out_dx_al);
    op(0xEF, op_out_dx_axW);
    op(0xF0, op_lock);
    op(0xF1, op_int1); //ICEBP -- 386
    op(0xF2, op_rep); //repnz
    op(0xF3, op_rep); //repz
    op(0xF4, op_hlt);
    op(0xF5, op_cmc);
    op(0xF6, op_group_F6);
    op(0xF7, op_group_F7);
    op(0xF8, op_clc);
    op(0xF9, op_stc);
    op(0xFA, op_cli);
    op(0xFB, op_sti);
    op(0xFC, op_cld);
    op(0xFD, op_std);
    op(0xFE, op_group_FE);
    op(0xFF, op_group_FF);


    //note: All 0x0F "extended" opcodes begin at 286+, so compatibility will only be noted if above 286 level

    
    opx(0xBE, op_movsx_rW_rm8); //386

//    opxg(0x00, 0, op_naG); //sldt
//    opxg(0x00, 1, op_naG); //str
//    opxg(0x00, 2, op_naG); //lldt
//    opxg(0x00, 3, op_naG); //ltr
//    opxg(0x00, 4, op_verr);
//    opxg(0x00, 5, op_verw);
//    opxg(0x01, 0, op_naG); //sgdt (this opcode also includes some P4+ opcodes for VMX)
//    opxg(0x01, 1, op_naG); //sidt and other opcodes
//    opxg(0x01, 2, op_naG); //lgdt and others
//    opxg(0x01, 3, op_naG); //lidt
//    opxg(0x01, 4, op_naG); //smsw_rm16_msw
//    opxg(0x01, 5, op_naG); //lmsw_msw_rm16
//    opx(0x02, op_lar_rW_rmW);
//    opx(0x03, op_lsl_rW_rmW);
//    opx(0x06, op_na); //clts
//    opx(0x08, op_na); //invd
//    opx(0x09, op_na); //wbinvd
    opx(0x0B, op_unknown); //UD2 official unsupported opcode
    opx(0x0D, op_nop_rmW); //???? Why does this take an argument!?
    // opx 0x10-0x17 is P3+

    //follows is a long set of NOP opcodes. These are used by compilers for optimization and alignment purposes I guess?
    //https://stackoverflow.com/questions/4798356/amd64-nopw-assembly-instruction
    //https://reverseengineering.stackexchange.com/questions/11971/nop-with-argument-in-x86-64


    //opcodes 0-4 are P4+ opcodes
//    for(int i=4;i<8;i++){
//        opxg(0x18, i, op_nopG_rmW); //Pentium Pro
//    }
//    for(int i=0x19;i<0x20;i++){
//        opx(i, op_nop_rmW); //Pentium Pro
//    }
//
//    for(int i=0x20;i<0x24;i++){
//        //mov_r32_cr, mov_r32_dr, mov_cr_r32, mov_dr_r32 -- All ring0 opcodes
//        opx(i, op_na);
//    }
//    opx(0x30, op_na); //wrmsr
//    opx(0x31, op_na); //rdtsc
//    opx(0x32, op_na); //rdmsr
//    opx(0x33, op_na); //rdpmc
//    //massive gap of Core only instructions
//    for(int i=0;i<16;i++){
//        opx(0x40 + i, op_cmovcc_rW_rmW); //Pentium Pro
//    }


    //another large swath of P3+ and MMX instructions
    for(int i=0;i<16;i++){
        opx(0x80+i, op_jcc_relW); //386
        opx(0x90+i, op_setcc_rm8); //386
    }

    opx(0xA0, op_push_fs); //386
    opx(0xA1, op_pop_fs); // 386
//    opx(0xA2, op_cpuid); //486
    opx(0xA3, op_bt_rmW_rW); //386
    opx(0xA4, op_shld_rmW_rW_imm8); //386
    opx(0xA5, op_shld_rmW_rW_cl); //386
    opx(0xA8, op_push_gs); //386
    opx(0xA9, op_pop_gs); //386
//    opx(0xAA, op_rsm); //386
    opx(0xAB, op_bts_rmW_rW); //386
    opx(0xAC, op_shrd_rmW_rW_imm8); //386
    opx(0xAD, op_shrd_rmW_rW_cl); //386
//    //0xAE is all P2+ opcodes
//    opx(0xAF, op_imul_rW_rmW); //386
//    opx(0xB0, op_cmpxchg_rm8_al_r8); //486
//    opx(0xB1, op_op_cmpxchg_rmW_axW_rW); //486
//    opx(0xB2, op_na); //lss (unsupported)
//    opx(0xB3, op_btr_rmW_rW); //386
//    opx(0xB4, op_na); //lfs (unsupported)
//    opx(0xB5, op_na); //lgs (unsupported)
//    opx(0xB6, op_movzx_rW_rm8); //386
//    opx(0xB7, op_movzx_rW_rm16); //386 (correct? shouldn't it be r32?)
//    opx(0xB9, op_unknown); //another official undefined opcode (technically this one takes an RM argument?)
//    opxg(0xBA, 4, op_bt_rmW_imm8); //group BA is 386
//    opxg(0xBA, 5, op_bts_rmW_imm8);
//    opxg(0xBA, 6, op_btr_rmW_imm8);
//    opxg(0xBA, 7, op_btc_rmW_imm8);
//    opx(0xBB, op_btc_rmW_rW); //386
//    opx(0xBC, op_bsf_rW_rmW); //386
//    opx(0xBD, op_bsr_rW_rmW); //386
//    opx(0xBF, op_movsx_rW_rm16); //386
//    opx(0xC0, op_xadd_rm8_r8); //486
//    opx(0xC1, op_xadd_rmW_rW); //486
//    opxg(0xC7, 1, op_cmpxchg8b); //P1 (the rest of this group is P4+)
//    for(int i=0;i<8;i++){
//        opx(0xC8 + i, op_bswap_rW);
//    }





/*
	for(int i=0;i<=7;i++){
		InstallOp(0xB0+i,&x86CPU::op_mov_r8_imm8);
		InstallOp(0x58+i,&x86CPU::op_pop_rW);
		InstallOp(0x50+i,&x86CPU::op_push_rW);
		InstallOp(0x40+i,&x86CPU::op_inc_rW);
		InstallOp(0x48+i,&x86CPU::op_dec_rW);
		InstallOp(0xD8+i,&x86CPU::op_escape);
		InstallOp(0x90+i,&x86CPU::op_xchg_axW_rW);
		InstallOp(0xB8+i,&x86CPU::op_mov_rW_immW);
	}
	InstallOp(0xF4,&x86CPU::op_hlt);
	InstallOp(0x90,&x86CPU::op_nop);
	InstallOp(0xEB,&x86CPU::op_jmp_rel8);
	InstallOp(0x2C,&x86CPU::op_sub_al_imm8);
	InstallOp(0x2D,&x86CPU::op_sub_axW_immW);
	InstallOp(0x8E,&x86CPU::op_mov_sr_rm16);
	InstallOp(0x8C,&x86CPU::op_mov_rm16_sr);
	InstallOp(0x68,&x86CPU::op_push_immW);
	InstallOp(0x07,&x86CPU::op_pop_es);
	InstallOp(0x17,&x86CPU::op_pop_ss);
	InstallOp(0x1F,&x86CPU::op_pop_ds);
	InstallOp(0x06,&x86CPU::op_push_es);
	InstallOp(0x0E,&x86CPU::op_push_cs);
	InstallOp(0x16,&x86CPU::op_push_ss);
	InstallOp(0x1E,&x86CPU::op_push_ds);
	InstallOp(0x89,&x86CPU::op_mov_rmW_rW);
	InstallOp(0x8B,&x86CPU::op_mov_rW_rmW);
	InstallOp(0xE8,&x86CPU::op_call_relW);
	InstallOp(0xC3,&x86CPU::op_retn);
	InstallOp(0xE2,&x86CPU::op_loopcc_rel8);
	InstallOp(0x26,&x86CPU::op_pre_es_override);
	InstallOp(0x3E,&x86CPU::op_pre_ds_override);
	InstallOp(0x36,&x86CPU::op_pre_ss_override);
	InstallOp(0x2E,&x86CPU::op_pre_cs_override);
	InstallOp(0xA5,&x86CPU::op_movsW);
	InstallOp(0xA4,&x86CPU::op_movsb);
	InstallOp(0xF8,&x86CPU::op_clc);
	InstallOp(0xFC,&x86CPU::op_cld);
	InstallOp(0xFA,&x86CPU::op_cli);
	InstallOp(0xF9,&x86CPU::op_stc);
	InstallOp(0xFD,&x86CPU::op_std);
	InstallOp(0xFB,&x86CPU::op_sti);
	InstallOp(0xF2,&x86CPU::op_rep);
	InstallOp(0xF3,&x86CPU::op_rep); //different, but handled by the same function...
	InstallOp(0xE6,&x86CPU::op_out_imm8_al);
	InstallOp(0xE7,&x86CPU::op_out_imm8_axW);
	InstallOp(0x9A,&x86CPU::op_call_immF);
	InstallOp(0xCB,&x86CPU::op_retf);
	//can't override these opcodes
	//InstallOp(0xCD,&x86CPU::op_int_imm8);
	//InstallOp(0xCF,&x86CPU::op_iret);
	//InstallOp(0xCC,&x86CPU::op_int3);
	//InstallOp(0xCE,&x86CPU::op_into);
	InstallOp(0xE4,&x86CPU::op_in_al_imm8);
	InstallOp(0xE5,&x86CPU::op_in_axW_imm8);
	InstallOp(0x04,&x86CPU::op_add_al_imm8);
	InstallOp(0x05,&x86CPU::op_add_axW_immW);
	InstallOp(0x28,&x86CPU::op_sub_rm8_r8);
	InstallOp(0x80,&x86CPU::op_group_80);
	InstallOp(0x29,&x86CPU::op_sub_rmW_rW);
	InstallOp(0x2A,&x86CPU::op_sub_r8_rm8);
	InstallOp(0x2B,&x86CPU::op_sub_rW_rmW);
	InstallOp(0x81,&x86CPU::op_group_81);
	InstallOp(0x00,&x86CPU::op_add_rm8_r8);
	InstallOp(0x01,&x86CPU::op_add_rmW_rW);
	InstallOp(0x02,&x86CPU::op_add_r8_rm8);
	InstallOp(0x03,&x86CPU::op_add_rW_rmW);
	InstallOp(0xA0,&x86CPU::op_mov_al_m8);
	InstallOp(0xA1,&x86CPU::op_mov_axW_mW);
	InstallOp(0x88,&x86CPU::op_mov_rm8_r8);
	InstallOp(0x8A,&x86CPU::op_mov_r8_rm8);
	InstallOp(0xA2,&x86CPU::op_mov_m8_al);
	InstallOp(0xA3,&x86CPU::op_mov_mW_axW);
	InstallOp(0xC6,&x86CPU::op_mov_m8_imm8);
	InstallOp(0xC7,&x86CPU::op_mov_mW_immW);
	InstallOp(0x38,&x86CPU::op_cmp_rm8_r8);
	InstallOp(0x39,&x86CPU::op_cmp_rmW_rW);
	InstallOp(0x3A,&x86CPU::op_cmp_r8_rm8);
	InstallOp(0x3B,&x86CPU::op_cmp_rW_rmW);
	InstallOp(0x3C,&x86CPU::op_cmp_al_imm8);
	InstallOp(0x3D,&x86CPU::op_cmp_axW_immW);
	InstallOp(0x83,&x86CPU::op_group_83);
	InstallOp(0xFF,&x86CPU::op_group_FF);
	InstallOp(0xE9,&x86CPU::op_jmp_relW);
	InstallOp(0xEA,&x86CPU::op_jmp_immF);
	InstallOp(0x6A,&x86CPU::op_push_imm8);
	InstallOp(0x8F,&x86CPU::op_group_8F);
	InstallOp(0xD6,&x86CPU::op_salc);
	InstallOp(0xF5,&x86CPU::op_cmc);
	InstallOp(0x98,&x86CPU::op_cbw);
	InstallOp(0x37,&x86CPU::op_aaa);
	InstallOp(0x27,&x86CPU::op_daa);
	InstallOp(0x2F,&x86CPU::op_das);
	InstallOp(0x3F,&x86CPU::op_aas);
	InstallOp(0xD5,&x86CPU::op_aad_imm8);
	InstallOp(0xD4,&x86CPU::op_aam_imm8);
	InstallOp(0xFE,&x86CPU::op_group_FE);
	InstallOp(0xF6,&x86CPU::op_group_F6);
	InstallOp(0xF7,&x86CPU::op_group_F7);
	InstallOp(0x99,&x86CPU::op_cwE);
	InstallOp(0x20,&x86CPU::op_and_rm8_r8);
	InstallOp(0x21,&x86CPU::op_and_rmW_rW);
	InstallOp(0x22,&x86CPU::op_and_r8_rm8);
	InstallOp(0x23,&x86CPU::op_and_rW_rmW);
	InstallOp(0x24,&x86CPU::op_and_al_imm8);
	InstallOp(0x25,&x86CPU::op_and_axW_immW);
	InstallOp(0x08,&x86CPU::op_or_rm8_r8);
	InstallOp(0x09,&x86CPU::op_or_rmW_rW);
	InstallOp(0x0A,&x86CPU::op_or_r8_rm8);
	InstallOp(0x0B,&x86CPU::op_or_rW_rmW);
	InstallOp(0x0C,&x86CPU::op_or_al_imm8);
	InstallOp(0x0D,&x86CPU::op_or_axW_immW);
	InstallOp(0xA6,&x86CPU::op_cmpsb);
	InstallOp(0xA7,&x86CPU::op_cmpsW);
	InstallOp(0xE3,&x86CPU::op_jcxzW_rel8);
	InstallOp(0x14,&x86CPU::op_adc_al_imm8);
	InstallOp(0x15,&x86CPU::op_adc_axW_immW);
	InstallOp(0x10,&x86CPU::op_adc_rm8_r8);
	InstallOp(0x11,&x86CPU::op_adc_rmW_rW);
	InstallOp(0x12,&x86CPU::op_adc_r8_rm8);
	InstallOp(0x13,&x86CPU::op_adc_rW_rmW);
	InstallOp(0x9E,&x86CPU::op_sahf);
	InstallOp(0x9F,&x86CPU::op_lahf);
	InstallOp(0xE1,&x86CPU::op_loopcc_rel8);
	InstallOp(0xE0,&x86CPU::op_loopcc_rel8);
	InstallOp(0xC5,&x86CPU::op_lds);
	InstallOp(0xC4,&x86CPU::op_les);
	InstallOp(0x8D,&x86CPU::op_lea);
	InstallOp(0xF0,&x86CPU::op_lock);
	InstallOp(0x30,&x86CPU::op_xor_rm8_r8);
	InstallOp(0x31,&x86CPU::op_xor_rmW_rW);
	InstallOp(0x32,&x86CPU::op_xor_r8_rm8);
	InstallOp(0x33,&x86CPU::op_xor_rW_rmW);
	InstallOp(0x34,&x86CPU::op_xor_al_imm8);
	InstallOp(0x35,&x86CPU::op_xor_axW_immW);
	InstallOp(0x1C,&x86CPU::op_sbb_al_imm8);
	InstallOp(0x1D,&x86CPU::op_sbb_axW_immW);
	InstallOp(0x19,&x86CPU::op_sbb_rmW_rW);
	InstallOp(0x1A,&x86CPU::op_sbb_r8_rm8);
	InstallOp(0x1B,&x86CPU::op_sbb_rW_rmW);
	InstallOp(0x18,&x86CPU::op_sub_rm8_r8);
	InstallOp(0x84,&x86CPU::op_test_rm8_r8);
	InstallOp(0x85,&x86CPU::op_test_rmW_rW);
	InstallOp(0xA8,&x86CPU::op_test_al_imm8);
	InstallOp(0xA9,&x86CPU::op_test_axW_immW);
	InstallOp(0x86,&x86CPU::op_xchg_rm8_r8);
	InstallOp(0x87,&x86CPU::op_xchg_rmW_rW);
	InstallOp(0xD2,&x86CPU::op_group_D2);
	InstallOp(0xD3,&x86CPU::op_group_D3);
	InstallOp(0xD0,&x86CPU::op_group_D0);
	InstallOp(0xD1,&x86CPU::op_group_D1);
	InstallOp(0xAC,&x86CPU::op_lodsb);
	InstallOp(0xAD,&x86CPU::op_lodsW);
	InstallOp(0xAE,&x86CPU::op_scasb);
	InstallOp(0xAF,&x86CPU::op_scasW);
	InstallOp(0x9B,&x86CPU::op_wait);
	InstallOp(0xD7,&x86CPU::op_xlatb);
	InstallOp(0xEC,&x86CPU::op_in_al_dx);
	InstallOp(0xED,&x86CPU::op_in_axW_dx);
	InstallOp(0xEE,&x86CPU::op_out_dx_al);
	InstallOp(0xEF,&x86CPU::op_out_dx_axW);
	InstallOp(0xAA,&x86CPU::op_stosb);
	InstallOp(0xAB,&x86CPU::op_stosW);

    for(int i=0;i<16;i++){
        InstallOp(0x70+i, &x86CPU::op_jcc_rel8, opcodes_hosted);
        InstallOp(0x80+i, &x86CPU::op_jcc_relW, opcodes_hosted_ext);
    }
*/
    //two byte extended opcodes (new as of i286)
    InstallOp(0xB6,&x86CPU::op_movzx_rW_rm8, opcodes_hosted_ext);
    InstallOp(0xB7,&x86CPU::op_movzx_r32_rmW, opcodes_hosted_ext);





}


bool x86CPU::IsLocked(){
	return Memory->IsLocked();
}

void x86CPU::Lock(){
	//nothing...
	Memory->Lock();
	busmaster=1;
}

void x86CPU::Unlock(){
	//still nothing...
	Memory->Unlock();
	busmaster=0;
}


void x86CPU::ReadMemory(uint32_t address, uint32_t size, void* buffer, MemAccessReason reason){
    Memory->WaitLock(busmaster);
    Memory->Read(address, size, buffer, reason);
}
void x86CPU::WriteMemory(uint32_t address, uint32_t size, void* buffer, MemAccessReason reason){
    Memory->WaitLock(busmaster);
    Memory->Write(address, size, buffer, reason);
}


};






