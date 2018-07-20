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


static bool jcc(int condition, volatile FLAGS &f){
    switch(condition){ //see http://www.ousob.com/ng/iapx86/ng10482.php for a good reference
        case 0:
            return f.bits.of;
        case 1:
            return !f.bits.of;
        case 2:
            return f.bits.cf;
        case 3:
            return !f.bits.cf;
        case 4:
            return f.bits.zf;
        case 5:
            return !f.bits.zf;
        case 6:
            return f.bits.cf | f.bits.zf;
        case 7:
            return !f.bits.cf & !f.bits.zf;
        case 8:
            return f.bits.sf;
        case 9:
            return !f.bits.sf;
        case 10:
            return f.bits.pf;
        case 11:
            return !f.bits.pf;
        case 12:
            return f.bits.sf!=f.bits.of;
        case 13:
            return f.bits.sf==f.bits.of;
        case 14:
            return (f.bits.sf!=f.bits.of) | f.bits.zf;
        case 15:
            return (f.bits.sf==f.bits.of) & !f.bits.zf;
        default:
            throw new CPUFaultException("This code should not be reached", 0xFFFF);
    }
}

void x86CPU::op_jcc_rel8(){
    int cc = opbyte-0x70;
    uint8_t rel = ReadCode8(1);
    if(jcc(cc, freg)){
        eip += 2;
        Jmp_near8(rel);
        eip--;
    }else{
        eip++;
    }
}

void x86CPU::op_jcc_relW(){
    int cc = opbyte-0x80;
    uint32_t rel = ReadCodeW(1);
    if(jcc(cc, freg)){
        eip += OperandSize() + 1;
        Jmp_nearW(rel);
        eip--;
    }else{
        eip+=OperandSize(); //1 to move past current opcode
    }
}

void x86CPU::op_setcc_rm8(){
    int cc = opbyte-0x90;
    ModRM rm8(this);
    if(jcc(cc, freg)){ //use the same flags with jcc
        rm8.WriteByte(1); 
    }else{
        rm8.WriteByte(0);
    }
}


void x86CPU::op_clc(){
	freg.bits.cf=0;
}

void x86CPU::op_cld(){
	freg.bits.df=0;
}

void x86CPU::op_cli(){
	freg.bits._if=0;
}

void x86CPU::op_stc(){
	freg.bits.cf=1;
}

void x86CPU::op_std(){
	freg.bits.df=1;
}

void x86CPU::op_sti(){
	freg.bits._if=1;
}


void x86CPU::op_cmc(){
    freg.bits.cf=freg.bits.cf^1;
}

void x86CPU::op_lahf(){
	SetReg8(AH, freg.data & 0xFF);
}
void x86CPU::op_sahf(){
    freg.data = (freg.data & 0xFFFFFF00) | Reg8(AH);
}











};



