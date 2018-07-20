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



void x86CPU::op_jmp_rel8(){
    uint8_t rel = ReadCode8(1);
    eip += 2;
	Jmp_near8(rel);
	eip--;
}

void x86CPU::op_jmp_relW(){
    uint32_t rel = ReadCodeW(1);
    eip += OperandSize() + 1;
	//+ operand to move past immediate, and +1 to move past current opcode
    Jmp_nearW(rel);
    eip--;
}

void x86CPU::op_jmp_immF(){ //far jmp
    uint32_t rel = ReadCodeW(1);
	seg[cCS]=0; //*(uint16_t*)&op_cache[2]; //I always forget that they are reversed...
	eip = rel;
	eip--; //eip will be incremented in Cycle
}

void x86CPU::op_jmp_rmW(ModRM &rm){
    eip=rm.ReadW(); //absolute address...
    eip--;
}

void x86CPU::op_jmp_mF(ModRM &rm){
	seg[cCS]=0; //*(uint16_t*)&op_cache[2];
	eip = ReadWA(DS, rm.ReadA(), CodeFetch);
	eip--;
}



void x86CPU::op_jcxzW_rel8(){
    uint8_t rel = ReadCode8(1);
	if(Reg(ECX)==0){
        eip++;
		Jmp_near8(rel);
        eip--;
	}else{
        eip++; //relative jump should happen at the beginning of the next opcode
    }
}




void x86CPU::op_call_relW(){
    uint32_t tmp = ReadCodeW(1);
	//increment eip to move beyond relW and then +1 to increment past current opcode byte
	eip+=(OperandSize16 ? 2 : 4) + 1;
	Push(eip); //+ (OperandSize16 ? 2 : 4) + 1);
	Jmp_nearW(tmp);
    eip--;
}

void x86CPU::op_retn(){ 
	eip=Pop();
	eip--; //because in Cycle, we increment. EIP should point to beginning of instruction after the CALL
}

void x86CPU::op_loopcc_rel8(){ //handles loop, loopne, and loope
	//loopne is 0xE0, loope is 0xE1, loop is 0xE2
    SetReg(ECX, Reg(ECX)-1);
    uint8_t rel = ReadCode8(1);
    eip+=2;
    if((Reg(ECX)!=0) &&
     		(opbyte == 0xE2 || (freg.bits.zf==(opbyte & 0x1)))){
        Jmp_near8(rel);
        eip--;
    }
}


void x86CPU::op_call_immF() { //far call
    Push(0); //seg[cCS]);
    //1 to move beyond current opcode, then + operand for EIP, and + 2 for segment
	Push(eip + OperandSize() + 2 + 1);
	seg[cCS]=0; //*(uint16_t*)&op_cache[2]; //I always forget that they are reversed...
	eip=ReadCodeW(1);
	eip--; //eip will be incremented in Cycle
}
void x86CPU::op_retf(){
	eip=Pop();
	seg[cCS]=0; //Pop16();
}

void x86CPU::op_retf_imm16(){
    uint16_t imm = ReadCode16(1);
    op_retf();
    //surprisingly this is not subject to operand-size differences
    regs32[ESP]+=imm;
}
void x86CPU::op_retn_imm16(){
    uint16_t imm = ReadCode16(1);
    op_retn();
    //surprisingly this is not subject to operand-size differences
    regs32[ESP]+=imm;
}

void x86CPU::op_int_imm8(){
    int num = ReadCode8(1);
    eip+=2; //set to next opcode
    Hypervisor->HandleInt(num, *this);
    eip--; //decrement once to compensate for eip increment in Cycle()
}

void x86CPU::op_iret(){
	//TODO mark fault handled
    eip=Pop();
    Pop(); //throw away CS value
	seg[cCS]=0; //Pop16();
    //Qtum ignores top 16 bits for iret
    freg.data = (freg.data & 0xFFFF0000) | Pop();
}

void x86CPU::op_int3(){
	Int16(3);
}

void x86CPU::op_int1(){
    Int16(1);
}

void x86CPU::op_into(){
	if(freg.bits.of==1){
		Int16(4);
	}
}
void x86CPU::op_call_rmW(ModRM &rm){ //call
	Push(eip+rm.GetLength());
	eip=rm.ReadW();
	eip--; //eip will be incremented in Cycle
}

void x86CPU::op_call_mF(ModRM &rm){ //far call
    Push(0); //seg[cCS]);
	Push(eip + rm.GetLength());
	uint32_t m = ReadWA(DS,rm.ReadA(),CodeFetch); //last 2 bytes (byte 5 and 6) is segment in 32bit mode, but is not used so ignore
	seg[cCS]=0; //*(uint16_t*)&op_cache[2]; //I always forget that they are reversed...
	eip=W(m);
	eip--; //eip will be incremented in Cycle

}

void x86CPU::op_bt_rmW_rW(){
    ModRM rm(this);
    uint32_t bitBase = rm.ReadW();
    uint32_t bitOffset = Reg(rm.GetExtra());
    if(OperandSize16) {
        bitOffset %= 16;
    }else{
        bitOffset %= 32;
    }
    freg.bits.cf = (bitBase & (1<<bitOffset)) > 0;
}

void x86CPU::op_bts_rmW_rW(){
    ModRM rm(this);
    uint32_t bitBase = rm.ReadW();
    uint32_t bitOffset = Reg(rm.GetExtra());
    if(OperandSize16) {
        bitOffset %= 16;
    }else{
        bitOffset %= 32;
    }
    freg.bits.cf = (bitBase & (1<<bitOffset)) > 0;
    rm.WriteW((bitBase | (1<<bitOffset)));
}

};


