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
#include <x86lib.h>
namespace x86Lib{
using namespace std;






void x86CPU::op_mov_r8_imm8(){ //0xB0+r
	SetReg8(opbyte-0xB0, ReadCode8(1));
	eip++;
}

void x86CPU::op_mov_rW_immW(){ //0xB8+r
    SetReg(opbyte-0xB8, ReadCodeW(1));
	eip+=OperandSize();
}

void x86CPU::op_mov_sr_rm16(){ //0x8E
    ModRM rm(this);
    //need ModRM for parsing, but otherwise it's a no-op
}

void x86CPU::op_mov_rm16_sr(){ //0x8C
    ModRM rm(this);
    rm.WriteWord(0);
}


void x86CPU::op_mov_rW_rmW(){
	ModRM rm(this);
	SetReg(rm.GetExtra(), rm.ReadW());
}

void x86CPU::op_mov_rmW_rW(){
	ModRM rm(this);
	rm.WriteW(Reg(rm.GetExtra()));
}

void x86CPU::op_mov_al_m8(){
    SetReg8(AL, ReadByteA(DS, ImmA()));
}
void x86CPU::op_mov_axW_mW(){
    SetReg(AX, ReadWA(DS, ImmA()));
}

void x86CPU::op_mov_rm8_r8(){
	ModRM rm(this);
	rm.WriteByte(Reg8(rm.GetExtra()));
}

void x86CPU::op_mov_r8_rm8(){
	ModRM rm(this);
	SetReg8(rm.GetExtra(), rm.ReadByte());
}
void x86CPU::op_mov_m8_al(){
    WriteByte(DS, ImmA(), Reg8(AL));
}

void x86CPU::op_mov_mW_axW(){
    WriteWA(DS, ImmA(), Reg(AX));
}

void x86CPU::op_mov_rm8_imm8(){
	ModRM rm(this);
	
	//eventually fix this so that if r is used, then invalid opcode...
	rm.WriteByte(ReadByte(cCS,eip+rm.GetLength()));
	eip++;
}

void x86CPU::op_mov_rmW_immW(){
    ModRM rm(this);
    rm.WriteW(ReadW(cCS,eip+rm.GetLength()));
    eip+=OperandSize();
}

void x86CPU::op_lds(){
    throw new CpuInt_excp(GPF_IEXCP);
}

void x86CPU::op_les(){
    throw new CpuInt_excp(GPF_IEXCP);
}

void x86CPU::op_lea(){
	ModRM rm(this);
	SetReg(rm.GetExtra(), rm.ReadOffset());
}



void x86CPU::op_push_imm8(){
	Push(ReadCode8(1));
    eip++;
}
void x86CPU::op_push_rmW(ModRM &rm){
	Push(rm.ReadW());
}

void x86CPU::op_push_immW(){ //0x68
    Push(ImmW());
}

void x86CPU::op_push_rW(){ //0x50+reg
	Push(Reg(opbyte-0x50));
}

void x86CPU::op_push_es(){
    Push(0);
}

void x86CPU::op_push_cs(){
    Push(0);
}

void x86CPU::op_push_ds(){
    Push(0);
}

void x86CPU::op_push_ss(){
    Push(0);
}

void x86CPU::op_push_fs(){
    Push(0);
}

void x86CPU::op_push_gs(){
    Push(0);
}

void x86CPU::op_pop_rmW(ModRM &rm){
	rm.WriteW(Pop());
}

void x86CPU::op_pop_rW(){ //0x58+reg
	SetReg(opbyte-0x58, Pop());
}

void x86CPU::op_pop_es(){
    Pop();
}

void x86CPU::op_pop_ss(){
    Pop();
}

void x86CPU::op_pop_ds(){
    Pop();
}

void x86CPU::op_pop_fs(){
    Pop();
}

void x86CPU::op_pop_gs(){
    Pop();
}

void x86CPU::op_out_imm8_al(){
    uint8_t tmp = Reg8(AL);
	Ports->Write(ReadCode8(1),1, &tmp);
	eip++;
}

void x86CPU::op_out_imm8_axW(){
    uint32_t tmp = Reg(AX);
    if(OperandSize16){
        Ports->Write(ReadCode8(1),2, (void*) &tmp);
    }else{
        Ports->Write(ReadCode8(1),4, (void*) &tmp);
    }
	eip++;
}

void x86CPU::op_out_dx_al(){
    uint8_t tmp = Reg8(AL);
	Ports->Write(Reg16(DX),1, (void*)&tmp);
}

void x86CPU::op_out_dx_axW(){
    uint32_t tmp = Reg(AX);
    if(OperandSize16){
        Ports->Write(Reg16(DX),2,(void*) &tmp);
    }else{
        Ports->Write(Reg16(DX),4,(void*) &tmp);
    }
}

void x86CPU::op_in_al_imm8(){
    uint8_t tmp;
	Ports->Read(ReadCode8(1),1,(void*) &tmp);
    SetReg8(AL, tmp);
	eip++;
}

void x86CPU::op_in_axW_imm8(){
    uint32_t tmp;
    if(OperandSize16){
    	Ports->Read(ReadCode8(1),2,(void*) &tmp);
    }else{
        Ports->Read(ReadCode8(1),4,(void*) &tmp);
    }
    SetReg(AX, tmp);
	eip++;
}

void x86CPU::op_in_al_dx(){
    uint8_t tmp;
	Ports->Read(Reg16(DX),1,(void*) &tmp);
    SetReg8(AL, tmp);
}

void x86CPU::op_in_axW_dx(){
    uint32_t tmp;
    if(OperandSize16){
    	Ports->Read(Reg16(DX),2,(void*) &tmp);
    }else{
        Ports->Read(Reg16(DX),4,(void*) &tmp);
    }
    SetReg(AX, tmp);
}



void x86CPU::op_xchg_rm8_r8(){
	#ifndef X86_MULTITHREADING
	if(IsLocked()==1){
		eip--;
		return;
	}
	#endif
	Lock();
	ModRM rm(this);
	uint8_t tmp=Reg8(rm.GetExtra());
	SetReg8(rm.GetExtra(), rm.ReadByte());
	rm.WriteByte(tmp);
	Unlock();
}
void x86CPU::op_xchg_rmW_rW(){
	#ifndef X86_MULTITHREADING
	if(IsLocked()==1){
		eip--;
		return;
	}
	#endif
	Lock();
	ModRM rm(this);
	uint32_t tmp=Reg(rm.GetExtra());
    SetReg(rm.GetExtra(), rm.ReadW());
	rm.WriteW(tmp);
	Unlock();
}

void x86CPU::op_xchg_axW_rW(){ //0x90+r
	uint32_t tmp=Reg(AX);
	SetReg(AX, Reg(opbyte-0x90));
	SetReg(opbyte-0x90, tmp);
}

void x86CPU::op_xlatb(){
    SetReg8(AL, ReadByteA(DS, RegA(BX) + (Reg8(AL))));
}

void x86CPU::op_movzx_rW_rm8(){
    ModRM rm(this);
    SetReg(rm.GetExtra(), rm.ReadByte());
}
void x86CPU::op_movzx_r32_rmW(){
    ModRM rm(this);
    regs32[rm.GetExtra()] = rm.ReadWord();
}

void x86CPU::op_pushaW(){
    uint32_t tmp;
    tmp = Reg(SP);
    Push(Reg(AX));
    Push(Reg(CX));
    Push(Reg(DX));
    Push(Reg(BX));
    Push(tmp);
    Push(Reg(BP));
    Push(Reg(SI));
    Push(Reg(DI));
}

void x86CPU::op_popaW(){
    uint32_t ofs=0;
    if(OperandSize16){
       ofs = 2;
    }else{
       ofs = 4;
    }

    SetReg(DI,Pop());
    SetReg(SI,Pop());
    SetReg(BP,Pop());
    SetReg(SP,Reg(SP)+ofs);
    SetReg(BX,Pop());
    SetReg(DX,Pop());
    SetReg(CX,Pop());
    SetReg(AX,Pop());
}

void x86CPU::op_enter(){
    uint16_t size = ReadCode16(1);
    eip+=2;

    uint8_t nestingLevel = ReadCode8(1) % 32;
    eip+=1;

    Push(Reg(EBP));
    uint32_t frameTemp = Reg(ESP);

    for (int i = 1; i < nestingLevel; ++i) {
        if (OperandSize16) {
            SetReg(EBP, Reg(EBP) - 2);
        }
        else {
            SetReg(EBP, Reg(EBP) - 4);
        }
        Push(Reg(EBP));
    }
    if (nestingLevel > 0) {
        Push(frameTemp);
    }

    SetReg(EBP, frameTemp);
    SetReg(ESP, Reg(EBP) - size);
}

void x86CPU::op_leave(){
    SetReg(ESP, Reg(EBP));
    SetReg(EBP, Pop());
}

void x86CPU::op_movsx_rW_rm8(){
    ModRM rm(this);
    SetReg(rm.GetExtra(), SignExtend8to32(rm.ReadByte()));
}

void x86CPU::op_pushf(){
    Push(freg.data);
}

void x86CPU::op_popf(){
    freg.data = Pop();
}

};



