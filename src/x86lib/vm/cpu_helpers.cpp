#include <stdint.h>
#include <x86lib.h>

using namespace x86Lib;

void x86CPU::Push(uint32_t val){
    if(Use32BitOperand()) {
        regs32[ESP] -= 4;
        WriteDword(cSS, regs32[ESP], val);
    }else{
        SetReg16(SP, Reg16(SP) - 2);
        WriteWord(cSS, Reg16(SP), val);
    }
}
uint32_t x86CPU::Pop(){
    uint32_t tmp;
    if(Use32BitOperand()) {
        tmp = ReadDword(cSS, regs32[ESP]);
        regs32[ESP] += 4;
    }else{
        tmp = ReadWord(cSS, Reg16(SP));
        SetReg16(SP, Reg16(SP) + 2);
    }
    return tmp;
}

void x86CPU::SetIndex8(){ //this just makes my code look better...
    if(freg.bits.df==0){
        SetReg16(SI, Reg(SI) + 1);
        SetReg16(DI, Reg(DI) + 1);
    }else{
        SetReg16(SI, Reg(SI) - 1);
        SetReg16(DI, Reg(DI) - 1);
    }
}

void x86CPU::SetIndex(){
    if(OperandSize16){
        SetIndex16();
    }else{
        SetIndex32();
    }
}
void x86CPU::SetIndex16(){
    if(freg.bits.df==0){
        SetReg16(SI, Reg(SI) + 2);
        SetReg16(DI, Reg(DI) + 2);
    }else{
        SetReg16(SI, Reg(SI) - 2);
        SetReg16(DI, Reg(DI) - 2);
    }
}

void x86CPU::SetIndex32(){
    if(freg.bits.df==0){
        (regs32[ESI])+=4;
        (regs32[EDI])+=4;
    }else{
        (regs32[ESI])-=4;
        (regs32[EDI])-=4;
    }
}
//according to intel manuals "[PF is] set if the least-significant byte of the result contains an even number of 1s"
void x86CPU::CalculatePF(uint32_t val){
    unsigned int i;
    unsigned int count=0;
    for(i=0;i<=7;i++){
        if((val&((1<<i)))!=0){count++;}
    }
    freg.bits.pf = (count%2) == 0;
}

//these calculate SF for the given operand size
void x86CPU::CalculateSF8(uint8_t val){
    if((val&0x80)==0){freg.bits.sf=0;}else{freg.bits.sf=1;}
}

void x86CPU::CalculateSF(uint32_t val){
    if(OperandSize16){
        CalculateSF16(val);
    }else{
        CalculateSF32(val);
    }
}
void x86CPU::CalculateSF16(uint16_t val){
    if((val&0x8000)==0){freg.bits.sf=0;}else{freg.bits.sf=1;}
}

void x86CPU::CalculateSF32(uint32_t val){
    if((val&0x80000000)==0){freg.bits.sf=0;}else{freg.bits.sf=1;}
}

void x86CPU::CalculateOF8(uint8_t result, uint8_t v1, uint8_t v2){
    //adapted from https://stackoverflow.com/questions/16845912/determining-carry-and-overflow-flag-in-6502-emulation-in-java
    //see also: http://www.righto.com/2012/12/the-6502-overflow-flag-explained.html
    //Basically this checks if result has a different sign bit than v1 and v2 
    freg.bits.of = !((v1^v2) & 0x80) && ((v1^result) & 0x80);
}
void x86CPU::CalculateOF16(uint16_t result, uint16_t v1, uint16_t v2){
    freg.bits.of = !((v1^v2) & 0x80) && ((v1^result) & 0x8000);
}
void x86CPU::CalculateOF32(uint32_t result, uint32_t v1, uint32_t v2){
    freg.bits.of = !((v1^v2) & 0x80) && ((v1^result) & 0x80000000);
}
void x86CPU::CalculateOFW(uint32_t result, uint32_t v1, uint32_t v2){
    if(OperandSize16){
        CalculateOF16(result, v1, v2);
    }else{
        CalculateOF32(result, v1, v2);
    }
}
void x86CPU::CalculateZF(uint32_t result){
    freg.bits.zf = result == 0;
}


void x86CPU::Jmp_nearW(uint32_t off){
    if(OperandSize16){
        Jmp_near16(off);
    }else{
        Jmp_near32(off);
    }
}

void x86CPU::Jmp_near32(uint32_t off){
    //I thought there would be a good way to do this, but I suppose this works..
    if((off&0x80000000)==0){ //if unsigned
        eip=eip+off;
    }else{
        eip=eip-((uint32_t)-off);
    }
    eip = W(eip);
}

void x86CPU::Jmp_near16(uint16_t off){
    //I thought there would be a good way to do this, but I suppose this works..
    if((off&0x8000)==0){ //if unsigned
        eip=eip+off;
    }else{
        eip=eip-((uint16_t)-off);
    }
    eip = W(eip);
}

void x86CPU::Jmp_near8(uint8_t off){
    //I thought there would be a good way to do this, but I suppose this works..
    if((off&0x80)==0){ //if unsigned
        eip=eip+off;
    }else{
        eip=eip-((uint8_t)-off);
    }
    eip = W(eip);
}

void x86CPU::Int16(uint8_t num){
    throw CPUFaultException("Unsupported operation (segment register modification)", UNSUPPORTED_EXCP);
}

void x86CPU::ResetSegments(){
    ES=cES;
    CS=cCS;
    SS=cSS;
    DS=cDS;
    FS=cFS;
    GS=cGS;
}

void x86CPU::SetSegments(uint8_t segm){
    ES=segm;
    CS=segm;
    SS=segm;
    DS=segm;
    FS=segm;
    GS=segm;
}

void x86CPU::Read(void* buffer, uint32_t off, size_t count, MemAccessReason reason){
    Memory->WaitLock(busmaster);
    Memory->Read(off,count,buffer, reason);
}
void x86CPU::Write(uint32_t off, void* buffer, size_t count, MemAccessReason reason){
    Memory->WaitLock(busmaster);
    Memory->Write(off,count,buffer, reason);
}

uint8_t x86CPU::ReadByte(uint8_t segm, uint32_t off, MemAccessReason reason){
    Memory->WaitLock(busmaster);
    uint8_t res=0;
    Memory->Read(off,1,&res, reason);
    return res;
}

uint16_t x86CPU::ReadWord(uint8_t segm,uint32_t off, MemAccessReason reason){
    Memory->WaitLock(busmaster);
    uint16_t res=0;
    Memory->Read(off,2,&res, reason);
    return res;
}

uint32_t x86CPU::ReadDword(uint8_t segm,uint32_t off, MemAccessReason reason){
    Memory->WaitLock(busmaster);
    uint32_t res=0;
    Memory->Read(off,4,&res, reason);
    return res;
}

uint64_t x86CPU::ReadQword(uint8_t segm,uint32_t off, MemAccessReason reason){
    Memory->WaitLock(busmaster);
    uint64_t res=0;
    Memory->Read(off,8,&res, reason);
    return res;
}

void x86CPU::WriteByte(uint8_t segm,uint32_t off,uint8_t val, MemAccessReason reason){
    Memory->WaitLock(busmaster);
    Memory->Write(off,1,&val, reason);
}

void x86CPU::WriteWord(uint8_t segm,uint32_t off,uint16_t val, MemAccessReason reason){
    Memory->WaitLock(busmaster);
    Memory->Write(off,2,&val, reason);
}

void x86CPU::WriteDword(uint8_t segm,uint32_t off,uint32_t val, MemAccessReason reason){
    Memory->WaitLock(busmaster);
    Memory->Write(off,4,&val, reason);
}

void x86CPU::WriteW(uint8_t segm, uint32_t off, uint32_t val, MemAccessReason reason){
    if(OperandSize16){
        WriteWord(segm, off, val, reason);
    }else{
        WriteDword(segm, off, val, reason);
    }
}

uint32_t x86CPU::ReadW(uint8_t segm, uint32_t off, MemAccessReason reason){
    if(OperandSize16){
        return ReadWord(segm, off, reason);
    }else{
        return ReadDword(segm, off, reason);
    }
}

void x86CPU::WriteWA(uint8_t segm, uint32_t off, uint32_t val, MemAccessReason reason){
    if(AddressSize16){
        off = off & 0xFFFF;
    }
    WriteW(segm, off, val, reason);
}

uint32_t x86CPU::ReadWA(uint8_t segm, uint32_t off, MemAccessReason reason){
    if(AddressSize16){
        off = off & 0xFFFF;
    }
    return ReadW(segm, off, reason);
}

uint8_t x86CPU::ReadByteA(uint8_t segm,uint32_t off, MemAccessReason reason){
    if(AddressSize16){
        off = off & 0xFFFF;
    }
    return ReadByte(segm, off, reason);
}

void x86CPU::WriteByteA(uint8_t segm, uint32_t off, uint8_t val, MemAccessReason reason){
    if(AddressSize16){
        off = off & 0xFFFF;
    }
    WriteByte(segm, off, val, reason);
}

uint32_t x86CPU::ReadCode32(int index){
    return ReadDword(CS, index + eip, CodeFetch);
}
uint16_t x86CPU::ReadCode16(int index){
    return ReadWord(CS, index + eip, CodeFetch);
}
uint8_t x86CPU::ReadCode8(int index){
    return ReadByte(CS, index + eip, CodeFetch);
}
uint32_t x86CPU::ReadCodeW(int index){
    if(OperandSize16){
        return ReadCode16(index);
    }else{
        return ReadCode32(index);
    }
}

void x86CPU::ReadCode(void* buf, int index, size_t count){
    Read(buf, index + eip, count, CodeFetch);
}


