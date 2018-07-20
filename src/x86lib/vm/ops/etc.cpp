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
#include <sstream>


namespace x86Lib{

void x86CPU::op_nop(){ //0x90
	//do nothing
}

void x86CPU::op_nop_rmW(){ //0x0D
    ModRM rm(this);
    rm.ReadW();
}

void x86CPU::op_hlt(){ //0xF4
	if(freg.bits._if==0){
		throw CPUFaultException("HLT With IF=0; Nothing to do",CLIHLT_EXCP);
	}
}

void x86CPU::op_unknown(){
    std::ostringstream oss;
    oss << "Unknown opcode: 0x" << std::hex << (int)ReadCode8(0);
    throw CPUFaultException(oss.str(), UNK_IEXCP);
}
void x86CPU::op_unknownG(ModRM &rm){
    std::ostringstream oss;
    oss << "Unknown opcode: 0x" << std::hex << (int)ReadCode8(-1); //-1 to go to before modrm
    throw CPUFaultException(oss.str(), UNK_IEXCP);
}




//Segment overrides...
void x86CPU::op_pre_es_override(){ //0x26
	//nop
}

void x86CPU::op_pre_ds_override(){ //0x3E
	//nop
}

void x86CPU::op_pre_ss_override(){ //0x36
	//nop
}

void x86CPU::op_pre_cs_override(){ //0x2E
	//nop
}
void x86CPU::op_pre_fs_override(){ //0x64
    //nop
}
void x86CPU::op_pre_gs_override(){ //0x65
    //nop
}

void x86CPU::op_rep(){ //repe and repne..(different opcodes, but I make them possible to use the same function)
    //use a string_compares variable...
    uint32_t counter;
    if(AddressSize16){
        counter = Reg16(CX);
    }else{
        counter = regs32[ECX];
    }
    uint8_t repop=opbyte;
    opbyte = ReadCode8(1); //update to string op
    switch(opbyte){
        //validate opcode is string
        case 0x6C:
        case 0x6D:
        case 0xA4:
        case 0xA5: 
        case 0x6E:
        case 0x6F:
        case 0xAC:
        case 0xAD:
        case 0xAA:
        case 0xAB:
        case 0xA6:
        case 0xA7:
        case 0xAE:
        case 0xAF:
        break;
        default:
        eip++;
        op_unknown();
        return;
    }
    //note: any opcode prefix must come before REP
    if(counter == 0){
        eip++; //get past REP, and now will be at string opcode, Cycle will iterate past it (string opcodes are always 1 byte).
        return;
    }else{
        uint32_t repEIP = eip;
        eip++; //now at actual opcode (or prefix)
        (this->*Opcodes[opbyte])();
        if(AddressSize16){
            SetReg16(CX, Reg16(CX) - 1);
        }else{
            SetReg32(ECX, Reg32(ECX) - 1);
        }
        eip=beginEIP - 1; //move to eip before REP (and prefixes), so that Cycle will iterate back to REP
        if(string_compares==1){
            string_compares=0;
            if(repop==0xF2){ //repNE
                if(freg.bits.zf == 1){ //exit...
                    eip = repEIP + 1; //
                    return;
                }
            }else{
                if(freg.bits.zf == 0){ //exit...
                    eip = repEIP + 1;
                    return;
                }
            }
        }
    }
}

void x86CPU::op_lock(){ //0xF0 prefix
	#ifndef X86_MULTITHREADING
	if(IsLocked()==1){
		eip--;
		return;
	}
	#endif
	Lock();
	eip++;
    opbyte = ReadCode8(0);
	//Add strict opcode testing for 386+
	(this->*Opcodes[opbyte])();
	Unlock();
}



void x86CPU::op_cbW(){
    if(OperandSize16){
        SetReg16(AX, SignExtend8to16(Reg8(AL)));
    }else{
        regs32[EAX] = SignExtend16to32(Reg16(AX));
    }
}



void x86CPU::op_cwE(){
	if(OperandSize16){
		if(Reg16(AX)>=0x8000){
			SetReg16(DX, 0xFFFF);
		}else{
            SetReg16(DX, 0);
		}
	}else{
	    if(regs32[EAX]>=0x80000000){
	        regs32[EDX]=0xFFFFFFFF;
	    }else{
	        regs32[EDX]=0;
	    }
	}
}

void x86CPU::op_escape(){
	/**This is for FPU escape opcodes
	this uses 0xD8 to 0xDF
	**/
	throw CpuInt_excp(UNDEV_IEXCP); //throw unknown device exception
}
void x86CPU::op_wait(){
	/**Does nothing...**/
	throw CpuInt_excp(UNDEV_IEXCP);
}



/**so-called undocumented opcodes:**/

/**salc -0xD6- this is will set al on carry -no arguments
log:  if cf=0 then al=0 else al=FF
**/
void x86CPU::op_salc(){ //set al on carry
    if(freg.bits.cf==0){
        SetReg8(AL, 0);
    }else{
        SetReg8(AL, 0xFF);
    }

}

//operand size override to 16bit
void x86CPU::op_operand_override(){
    eip++; //increment past override byte
    OperandSize16 = true;
    opbyte = ReadCode8(0);
    (this->*opcodes_hosted[opbyte])();
    OperandSize16 = false;
}

//operand size override to 16bit
void x86CPU::op_address_override(){
    eip++; //increment past override byte
    AddressSize16 = true;
    opbyte = ReadCode8(0);
    (this->*opcodes_hosted[opbyte])();
    AddressSize16 = false;
}


void x86CPU::op_bound_rW_mW(){
	uint32_t idx,offset,bound0,bound1;
	ModRM rm(this);
	
	using namespace std;
	idx = Reg(rm.GetExtra());
	offset = rm.ReadOffset();
	
	if(Use32BitAddress()){
		bound0 = ReadDword(DS,offset);
		bound1 = ReadDword(DS,offset+4);
	}else{
		bound0 = ReadWord(DS,offset);
		bound1 = ReadWord(DS,offset+2);
	}

	if(idx<bound0 || idx>bound1)
		throw CpuInt_excp(BOUNDS_IEXCP);
}


};




