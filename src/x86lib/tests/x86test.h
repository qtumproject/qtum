#ifndef X86TEST_H
#define X86TEST_H

#include "catch.hpp"
#include <stdint.h>
#include <string>
#include "catch.hpp"

#define private public
#define protected public
#include "x86lib.h"
#include "x86lib_internal.h"
#undef public
#undef protected

using namespace std;
using namespace x86Lib;

#define STACK_SIZE 128
#define STACK_START 64
#define SCRATCH_SIZE 128
#define CODE_SIZE 1024

#define CODE_ADDRESS 0x1000
#define STACK_ADDRESS 0x2000 //stack begins at top
#define SCRATCH_ADDRESS 0x3000
//IMPORTANT: note address lines up for 16-bit. This is important for not causing memory errors in testing
#define HIGH_SCRATCH_ADDRESS 0xFABF3000


struct x86Checkpoint{
    x86Checkpoint(){
        memset(stack, 0, STACK_SIZE);
        memset(scratch, 0, SCRATCH_SIZE);
        memset(highscratch, 0, SCRATCH_SIZE);
    }
    x86SaveData regs;
    uint8_t stack[STACK_SIZE];
    uint8_t scratch[SCRATCH_SIZE];
    uint8_t highscratch[SCRATCH_SIZE];

    uint32_t Stack32(int pos){
        uint32_t tmp;
        memcpy(&tmp, &stack[regs.regs32[ESP] - STACK_ADDRESS + pos], 4);
        return tmp;
    }
    uint16_t Stack16(int pos){
        uint16_t tmp;
        memcpy(&tmp, &stack[regs.regs32[ESP] - STACK_ADDRESS + pos], 2);
        return tmp;
    }
    void SetStack32(int pos, uint32_t val){
        memcpy(&stack[regs.regs32[ESP] - STACK_ADDRESS + pos], &val, 4);
    }
    void SetStack16(int pos, uint16_t val){
        memcpy(&stack[regs.regs32[ESP] -STACK_ADDRESS + pos], &val, 2);
    }
    uint32_t Scratch32(int pos){
        uint32_t tmp;
        memcpy(&tmp, &scratch[pos], 4);
        return tmp;
    }
    uint16_t Scratch16(int pos){
        uint16_t tmp;
        memcpy(&tmp, &scratch[pos], 2);
        return tmp;
    }
    void SetScratch32(int pos, uint32_t val){
        memcpy(&scratch[pos], &val, 4);
    }
    void SetScratch16(int pos, uint16_t val){
        memcpy(&scratch[pos], &val, 2);
    }

    uint32_t HighScratch32(int pos){
        uint32_t tmp;
        memcpy(&tmp, &highscratch[pos], 4);
        return tmp;
    }
    uint16_t HighScratch16(int pos){
        uint16_t tmp;
        memcpy(&tmp, &highscratch[pos], 2);
        return tmp;
    }
    void SetHighScratch32(int pos, uint32_t val){
        memcpy(&highscratch[pos], &val, 4);
    }
    void SetHighScratch16(int pos, uint16_t val){
        memcpy(&highscratch[pos], &val, 2);
    }

    uint8_t Reg8(int which){
        if(which < 4){
            //0-3 is low bytes; al, cl, dl, bl
            return regs.regs32[which] & 0xFF;
        }else{
            //4-7 is high bytes; ah, ch, dh, bh
            return (regs.regs32[which - 4] & 0xFF00) >> 8;
        }
    }
    uint16_t Reg16(int which){
        uint32_t tmp = regs.regs32[which];
        return (tmp & 0xFFFF);
    }

    void SetReg8(int which, uint8_t val){
        if(which < 4){
            //0-3 is low bytes; al, cl, dl, bl
            regs.regs32[which] = (regs.regs32[which] & 0xFFFFFF00) | val;
        }else{
            //4-7 is high bytes; ah, ch, dh, bh
            regs.regs32[which] = (regs.regs32[which - 4] & 0xFFFF00FF) | (val << 8);
        }
    }
    void SetReg16(int which, uint16_t val){
        regs.regs32[which] = (regs.regs32[which] & 0xFFFF0000) | val;
    }
    //no real need for 32bit versions, but for consistency..
    void SetReg32(int which, uint32_t val){
        regs.regs32[which] = val;
    }
    uint32_t Reg32(int which){
        return regs.regs32[which];
    }
    void AddReg32(int which, int val){
        regs.regs32[which] += val;
    }
    uint32_t GetEIP(){
        return regs.eip;
    }
    void SetEIP(uint32_t eip){
        regs.eip = eip;
    }
    void AdvanceEIP(int amount){
        regs.eip += amount;
    }
    //flags
    void SetCF(){
        regs.freg.bits.cf = 1;
    }
    void SetOF(){
        regs.freg.bits.of = 1;
    }
    void SetPF(){
        regs.freg.bits.pf = 1;
    }
    void SetAF(){
        regs.freg.bits.af = 1;
    }
    void SetZF(){
        regs.freg.bits.zf = 1;
    }
    void SetSF(){
        regs.freg.bits.sf = 1;
    }
    void SetIF(){
        regs.freg.bits._if = 1;
    }
    //unsets
    void UnsetCF(){
        regs.freg.bits.cf = 0;
    }
    void UnsetOF(){
        regs.freg.bits.of = 0;
    }
    void UnsetPF(){
        regs.freg.bits.pf = 0;
    }
    void UnsetAF(){
        regs.freg.bits.af = 0;
    }
    void UnsetZF(){
        regs.freg.bits.zf = 0;
    }
    void UnsetSF(){
        regs.freg.bits.sf = 0;
    }
    void UnsetIF(){
        regs.freg.bits._if = 0;
    }
};

class TestPorts : public PortDevice{
    x86CPU *cpu;
public:
    TestPorts(x86CPU *cpu_){
        cpu = cpu_;
    }
    ~TestPorts(){}
    virtual void Read(uint16_t port,int size,void *buffer){
        if(port == 0xff){
            cpu->SetLocation(cpu->GetLocation() - 2); //set EIP to before this IN/OUT instruction
            cpu->Stop();
        }
    }
    virtual void Write(uint16_t port,int size, const void *buffer){
        if(port == 0xff){
            cpu->SetLocation(cpu->GetLocation() - 2);
            cpu->Stop();
        }
    }
};

class x86Tester{
    x86CPU cpu;
    PortSystem ports;
    MemorySystem memory;
    ROMemory *coderom;
    RAMemory *scratchram;
    RAMemory *stackram;
    RAMemory *highscratchram;
    TestPorts *testports;

    x86Checkpoint cache;
    bool cacheValid;
public:
    x86Tester();
    ~x86Tester(){
    }
    //Assembles the given code as assembly
    void Assemble(string code);
    //loads a raw binary file
    void LoadFile(string file);
    //Loads the current x86 state into the checkpoint field
    x86Checkpoint LoadCheckpoint();
    x86Checkpoint& Check();
    //Loads the checkpoint data into the x86 VM
    void Apply(x86Checkpoint& checkpoint);
    void Compare(x86Checkpoint &check, bool checkeip=false, bool checkmemory=false);
    //Runs the x86 VM for the specified number of instructions
    void Run(int count=1000);
    void Run(string code, int count=1000){
        Assemble(code);
        Run(count);
    }


};


#endif

