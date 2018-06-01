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
   documentation and/or other materials provided with the digstribution.
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

#ifndef X86LIB
#define X86LIB
#include <iostream>
#include <vector>
#include <stdint.h>
#include <string>
#include <cstring>
#include <sstream>

#ifdef X86LIB_BUILD
#include <x86lib_internal.h>
#endif

//! The main namespace of x86Lib
namespace x86Lib{

#ifdef X86LIB_BUILD
#include <config.h>
#endif

//!	8086 CPU level
static const uint32_t CPU086=1;
//!	186 CPU level
static const uint32_t CPU186=2|CPU086;
//!	286 real mode only CPU level
static const uint32_t CPU286_REAL=4|CPU186; //Only support real mode instructions
//!	286 CPU level
static const uint32_t CPU286=8|CPU286_REAL;
//!	386 real mode only CPU level
static const uint32_t CPU386_REAL=16|CPU286_REAL; //Only Support real mode instructions
//!	386 CPU level
static const uint32_t CPU386=32|CPU386_REAL|CPU286;
//!	486 CPU level
static const uint32_t CPU486=64|CPU386;
//!	Pentium(586) CPU level
static const uint32_t CPU_PENTIUM=128|CPU486;
//!	Pentium Pro CPU level
static const uint32_t CPU_PPRO=256|CPU_PENTIUM;
//!	Default CPU level
/*!	CPU_DEFAULT will use the CPU with the most complete emulation
*/
static const uint32_t CPU_DEFAULT=0; //this is actually changed internally..






/**Exceptions...**/
//!	Exception code for an infinite halt
static const uint32_t CLIHLT_EXCP=1; //cli/hlt...nothing to do
//!	Exception code for memory access exception
static const uint32_t MEM_ACCESS_EXCP=3; //Memory Access Error...(can actually be page fault, or GPF, or stack fault...
//!	Exception code for triple fault
/*!	This code is always OR'd with another code
	so that you can tell what caused the triple fault.
*/
static const uint32_t TRIPLE_FAULT_EXCP=0x10000; //Triple fault...This should be ORd with the last exception



//! CPU Panic exception
/*!	This exception is thrown out of x86CPU if a fatal CPU error occurs,
	such as a triple fault.
*/
class CPUFaultException : public std::runtime_error { //used for fatal CPU errors, such as triple fault..

	public:
	/*!
	\param desc_ A text description of the error
	\param code_ An exception code
	*/
	CPUFaultException(std::string desc_,uint32_t code_) : std::runtime_error(desc_) {
		desc=desc_;
		code=code_;
	}
    virtual ~CPUFaultException() throw() {}
	//!A text description of the error
	std::string desc;
	//!An exception code
	uint32_t code;
};

//!	Memory error exception
/*!	This should only be thrown out of the PhysMemory class
	It is thrown out to tell x86CPU a memory exception has occured.
	This does not always result in a triple fault.
	/sa PhysMemory
*/
class MemoryException : public std::runtime_error { //Exclusively for the Memory Classes, these are caught and then a more appropriate excp is thrown
	public:
	/*!
	\param address_ The address at which had problems being read or written
	*/
	MemoryException(uint32_t address_) : std::runtime_error("Memory Error") {
        std::cout << "EXCEPTION: memory error @ 0x" << std::hex << address_ << std::endl; 
		address=address_;
	}
    virtual ~MemoryException() throw() {}
	uint32_t address;
};

class x86CPU;
/**This will be used for memory mapped devices(including memory itself)**/
class MemoryDevice{
	public:
	virtual void Read(uint32_t address,int count,void *buffer)=0;
	virtual void Write(uint32_t address,int count,void *data)=0;
	virtual int Readable(uint32_t address,int count){
		return 1;
		//This is optional. It is currently not used in the CPU code
	}
	virtual int Writeable(uint32_t address,int count){
		return 1;
		//This is optional. It is currently not used in the CPU code
	}
	virtual inline ~MemoryDevice()=0;
};

inline MemoryDevice::~MemoryDevice(){}


class PortDevice{
	public:
	virtual void Read(uint16_t address,int count,void *buffer)=0;
	virtual void Write(uint16_t address,int count,void *data)=0;
	virtual inline ~PortDevice()=0;
};

inline PortDevice::~PortDevice(){}

class InterruptHypervisor{
public:
    virtual void HandleInt(int number, x86CPU &vm)=0;
};

typedef struct DeviceRange
{
	union
	{
		class MemoryDevice *memdev;
		class PortDevice *portdev;
	};
	uint32_t high;
	uint32_t low;
}DeviceRange_t;

typedef enum _MemAccessReason {
    CodeFetch = 0,
    Data,
    Internal
} MemAccessReason;

class MemorySystem{
	std::vector<DeviceRange_t> memorySystemVector;
	protected:
	//! Intended to be used to mark if the address space is locked.
	volatile uint32_t locked; 
	public:
	MemorySystem();
	void Add(uint32_t low,uint32_t high,MemoryDevice *memdev);
	void Remove(uint32_t low,uint32_t high);
	void Remove(MemoryDevice *memdev);
	int RangeFree(uint32_t low,uint32_t high);
	void Read(uint32_t address,int count,void *buffer, MemAccessReason reason = Data);
	void Write(uint32_t address,int count,void *data, MemAccessReason reason = Data);
	//! Tells if memory is locked
	/*!
	\return 1 if memory is locked, 0 if not locked.
	*/
	bool IsLocked(){return locked;}
	//!	Locks the address space
	void Lock(){
		while(locked==1){}
		locked=1;
	}
	//! Unlocks the address space
	void Unlock(){
		locked=0;
	}
	void WaitLock(int haslock){
		if(haslock==0){
			while(locked>0){}
		}
	}
};

class PortSystem{
	DeviceRange_t *list;
	int count;
	public:
	PortSystem();

	void Add(uint16_t low,uint16_t high,PortDevice *portdev);
	void Remove(uint16_t low,uint16_t high);
	void Remove(PortDevice *portdev);
	int RangeFree(uint32_t low,uint32_t high);
	void Read(uint16_t address,int count,void *buffer);
	void Write(uint16_t address,int count,void *data);
};


class RAMemory : public MemoryDevice{
    protected:
    char *ptr;
    uint32_t size;
    std::string id;
    public:
    RAMemory(uint32_t size_, std::string id_){
        size = size_;
        id = id_;
        ptr = new char[size];;
        std::memset(ptr, 0, size);
    }
    virtual ~RAMemory(){
        delete[] ptr;
    }
    virtual void Read(uint32_t address,int count,void *buffer){
	if(address + count > size){
	    throw new MemoryException(address);
	}
        std::memcpy(buffer,&ptr[address],count);
    }
    virtual void Write(uint32_t address,int count,void *buffer){
	if(address + count > size){
	    throw new MemoryException(address);
	}
        std::memcpy(&ptr[address],buffer,count);
    }
    virtual char* GetMemory(){
        return ptr;
    }
};

class ROMemory : public RAMemory{
    public:
    ROMemory(uint32_t size_, std::string id_)
        : RAMemory(size_, id_){
    }

    virtual void Write(uint32_t address,int count,void *buffer){
        throw new MemoryException(address);
    }
};

class PointerMemory : public MemoryDevice{
    protected:
    uint8_t *ptr;
    uint32_t size;
    std::string id;
    public:
    PointerMemory(uint8_t* mem, uint32_t size_, std::string id_){
        size = size_;
        id = id_;
        ptr = mem;
    }
    virtual void Read(uint32_t address,int count,void *buffer){
	if(address + count > size){
	    throw new MemoryException(address);
	}
        std::memcpy(buffer,&ptr[address],count);
    }
    virtual void Write(uint32_t address,int count,void *buffer){
	if(address + count > size){
	    throw new MemoryException(address);
	}
        std::memcpy(&ptr[address],buffer,count);
    }
    virtual char* GetMemory(){
        return (char*) ptr;
    }
};

class PointerROMemory : public PointerMemory{
    public:
    PointerROMemory(uint8_t* mem, uint32_t size_, std::string id_) : 
    	PointerMemory(mem, size_, id_) {

        size = size_;
        id = id_;
        ptr = mem;
    }
    virtual void Write(uint32_t address,int count,void *buffer){
        throw new MemoryException(address);
    }
};



typedef union {
    struct{
        unsigned char cf:1;
        unsigned char r0:1;
        unsigned char pf:1;
        unsigned char r1:1;
        unsigned char af:1;
        unsigned char r2:1;
        unsigned char zf:1;
        unsigned char sf:1;
        unsigned char tf:1;
        unsigned char _if:1;
        unsigned char df:1;
        unsigned char of:1;
        unsigned char iopl:2; //not yet used
        unsigned char nt:1;
        unsigned char r3:1;
        unsigned int upper:16;
    }__attribute__((packed))bits; //this is a better representation of flags(much easier to use)
    uint32_t data;
} FLAGS;


//! The struct used to save the current state of x86CPU
struct x86SaveData{
	//! General registers
	uint32_t regs32[8];
	//! Segment registers
	uint16_t seg[7];
	//! Segment register routing(in case of segment overrides)
	uint8_t seg_route[7];
	//! Instruction pointer
	uint32_t eip;
	//! Flags register
	FLAGS freg;
};

};

//32 bit register macros
static const int EAX=0;
static const int ECX=1;
static const int EDX=2;
static const int EBX=3;
static const int ESP=4;
static const int EBP=5;
static const int ESI=6;
static const int EDI=7;


//16 bit register macros
static const int AX=0;
static const int CX=1;
static const int DX=2;
static const int BX=3;
static const int SP=4;
static const int BP=5;
static const int SI=6;
static const int DI=7;

//8 bit register macros
static const int AL=0;
static const int CL=1;
static const int DL=2;
static const int BL=3;
static const int AH=4;
static const int CH=5;
static const int DH=6;
static const int BH=7;

//segment registers constants(the defaults)
static const int cES=0;
static const int cCS=1;
static const int cSS=2;
static const int cDS=3;
static const int cFS=4;
static const int cGS=5;
static const int cIS=6; //this is an imaginary segment only used for direct segment overrides
//for instance it would be used in mov [1000:bx],ax


namespace x86Lib{

class ModRM;

typedef void (x86Lib::x86CPU::*opcode)();
typedef void (x86Lib::x86CPU::*groupOpcode)(ModRM &rm);
typedef struct{
     unsigned char rm:3;
     unsigned char extra:3;
     unsigned char mod:2;
}
__attribute__((packed))mod_rm; //this struct is a described mod r/m byte..

typedef struct{
    unsigned char base:3;
    unsigned char index:3;
    unsigned char ss:2;
}__attribute__((packed))scaleindex; //this struct is a described SIB scale index byte

//Note, this will re-cache op_cache, so do not use op_cache afterward
//Also, eip should be on the modrm byte!
//On return, it is on the last byte of the modrm block, so no advancement needed unelss there is an immediate
//Also, this will advance EIP upon exiting the opcode(deconstruction)
class ModRM{
	protected:
	bool use_ss;
	bool op_specific;
	x86CPU *this_cpu;
    bool jumpBehavior;
	private:
	mod_rm modrm;
    scaleindex sib;
	uint16_t GetRegD(); //This returns the register displacement value
    uint32_t GetRegD32(); //This returns the register displacement value
	uint16_t GetDisp();
    uint32_t GetDisp32();
    uint32_t GetSIBDisp();
    uint32_t ReadOffset32(); //This is only used by LEA. It will obtain the offset and not dereference it...
    uint8_t ReadByte32();
    uint16_t ReadWord32();
    uint32_t ReadDword32();
    void WriteByte32(uint8_t byte);
    void WriteWord32(uint16_t word);
    void WriteDword32(uint32_t dword);
	public:

	ModRM(x86CPU* this_cpu_);
	~ModRM();
	//The r suffix means /r, which means for op_specific=1, use general registers
	uint8_t ReadByte();
	uint16_t ReadWord();
	uint32_t ReadDword();
	uint32_t ReadW();
    uint32_t ReadA(); //should be used for addresses
	void WriteByte(uint8_t byte);
	void WriteWord(uint16_t word);
    void WriteDword(uint32_t dword);
    void WriteW(uint32_t dword);
	uint8_t GetLength(); //This returns how many total bytes the modrm block consumes
	uint8_t GetExtra(); //Get the extra fied from mod_rm
	uint32_t ReadOffset(); //This is only used by LEA. It will obtain the offset and not dereference it...
    uint8_t Imm8();
    uint16_t Imm16();
    uint32_t Imm32();
    uint32_t ImmW();
    //This tells ModRM to not increment EIP in the destruction of this object
    void setJumpBehavior(){
        jumpBehavior = true;
    }
};

//!	The main CPU control class
/*!	This class is the complete CPU. That being said, it is quite big
	and has many functions. It completely emulates the x86 line of CPUs
*/
class x86CPU{
    private:
	friend class ModRM;
	uint32_t regs32[8];
	uint16_t seg[7];
	uint32_t eip;
	FLAGS freg;
    //These variables should be used instead of cES etc when the segment register can not be overridden
	uint8_t ES;
	uint8_t CS;
	uint8_t SS;
	uint8_t DS;
	uint8_t FS;
	uint8_t GS;
	bool string_compares; //TODO can jump to string instruction and corrupt this
	uint8_t cli_count; //Whenever this is 1, an STI is done.
	volatile bool int_pending;
	volatile uint8_t int_number;
	uint32_t cpu_level;
	volatile bool busmaster;
	void Init();
    bool OperandSize16;
    bool AddressSize16;
    bool DoStop;
    int PrefixCount; //otherwise, someone could make some ridiculously long prefix chain (apparently Intel CPUs don't error until you get past 14)
    //this is the EIP pointed to when execution of the current opcode began in Cycle()
    //This is used when needing to go back to the current opcode while including all prefixes
    uint32_t beginEIP;
    uint8_t opbyte; //current opcode (updated in cycle() and prefix opcodes)
    int opcodeExtra; //current opcode (updated in cycle() and prefix opcodes)
	protected:
	//! Do one CPU opcode
	/*! This should be put in the main loop, as this is what makes the CPU work.
	*/
	void Cycle();
	opcode opcodes_hosted[256];
    //2 byte opcodes beginning with 0x0F
    opcode opcodes_hosted_ext[256];
	opcode *Opcodes; //current opcode mode
    opcode *Opcodes_ext; //current extended opcode mode
    groupOpcode opcodes_hosted_ext_group[256][8];
    std::string opcodes_hosted_str[256];
    std::string opcodes_hosted_ext_str[256];
    std::string opcodes_hosted_ext_group_str[256][8];
    std::string lastOpcodeStr;
    uint32_t lastOpcode;

	/*!
	\return 0 if no interrupts are pending
	 */
	int CheckInterrupts();

    uint32_t ReadCode32(int index);
    uint16_t ReadCode16(int index);
    uint8_t ReadCode8(int index);
    uint32_t ReadCodeW(int index);
    void ReadCode(void* buf, int index, size_t count);

	public:
	MemorySystem *Memory;
	PortSystem *Ports;
    InterruptHypervisor *Hypervisor;

    std::string GetLastOpcodeName(){
        if(opcodeExtra != -1){
            std::stringstream s;
            s << lastOpcodeStr << "/" << opcodeExtra;
            return s.str();
        }else{
            return lastOpcodeStr;
        }
    }
    uint32_t GetLastOpcode(){
        return lastOpcode;
    }

	/*!
	\param cpu_level The CPU level to use(default argument is default level)
	\param flags special flags to control CPU (currently, there is none)
	*/
	x86CPU(uint32_t cpu_level=0 ,uint32_t flags=0);
	/*!
	\param save The x86SaveData class to restore the cpu to
	\param flags special flags to control CPU (currently, there is none)
	*/
	x86CPU(x86SaveData &save,uint32_t flags=0);

	//!Runs the CPU for the specified cyclecount.
	void Exec(int cyclecount);
	
	//! Dump CPU state
	/*! This will dump cpu state to output. This is mainly used for debugging, as it is not flexible.
	\param output output stream which to use.
	*/
	void DumpState(std::ostream &output);
	//! Cause a CPU interrupt
	/*! This will cause a CPU interrupt(unless interrupt flag is cleared)
		Note! This does not resolve IRQs! This takes normal interrupt numbers(0-255)
	\param num Interrupt number
	*/
	void Int(uint8_t num);
	//! Saves CPU state
	/*! This will completely save the CPU state of the current x86CPU class
	\param save_data_buffer This should be a free memory area the size of x86SaveData
	*/
	void SaveState(x86SaveData *save_data_buffer);
	//!Loads CPU state
	/*! This will completely reset and reload the cpu state.
	\param load_data where the x86SaveData is located
	*/
	void LoadState(x86SaveData &load_data);

	//!Completely resets the CPU
	void Reset();
	//~x86CPU();
	//!Locks the PhysMemory in use, and declares this CPU as busmaster
	void Lock();
	//!Unlocks the PhysMemory in use
	void Unlock();
	//! Tells if PhysMemory in use is locked
	/*!
	\return 1 if PhysMemory in use is locked, otherwise returns 0
	*/
	bool IsLocked();
	/*Added after inital multi-branch switch over*/
	//!Checks if an interrupt is on the stack waiting to be answered.
	/*!
	\return 1 if an interrupt is waiting to be answered by the CPU, else, 0.
	*/
	bool IntPending();

    bool Use32BitOperand(){
        return !OperandSize16;
    }
    bool Use32BitAddress(){
        return !AddressSize16;
    }

    void ReadMemory(uint32_t address, uint32_t size, void* buffer, MemAccessReason reason = Data);
    void WriteMemory(uint32_t address, uint32_t size, void* buffer, MemAccessReason reason = Data);

	void Stop(){DoStop=true;}
    uint32_t GetLocation(){
        return eip;
    }
    void SetLocation(uint32_t pos){
        eip = pos;
    }

    //provided mainly for slightly easier debugging
    uint8_t ReadMachineByte(uint32_t address){
        uint8_t res;
        Memory->Read(address, 1, &res, Internal);
        return res;
    }

    uint32_t GetRegister32(int reg){
        if(reg > 7){
            return 0;
        }
        return regs32[reg];
    }
    void SetRegister32(int reg, uint32_t val){
        if(reg > 7){
            return;
        }
        regs32[reg] = val;
    }
    std::vector<uint32_t> wherebeen;
    void Read(void* buffer, uint32_t off, size_t count, MemAccessReason reason = Data);
    void Write(uint32_t off, void* buffer, size_t count, MemAccessReason reason = Data);

    /*End public interface*/
	#ifdef X86LIB_BUILD
	private:
	#include <opcode_def.h>
	#endif

	inline uint32_t A(uint32_t a){
		if(AddressSize16){
			return a & 0xFFFF;
		}
		return a;
	}
	inline uint32_t W(uint32_t val){
		if(OperandSize16){
			return val & 0xFFFF;
		}
		return val;
	}

	inline uint32_t ImmW(){
		if(OperandSize16){
			eip+=2;
			return (uint32_t) ReadCode16(1-2);
		}
		eip+=4;
		return ReadCode32(1-4);
	}
	inline uint32_t ImmA(){
		if(AddressSize16){
			eip+=2;
			return (uint32_t) ReadCode16(1-2);
		}
		eip+=4;
		return ReadCode32(1-4);
	}

	inline uint32_t DispW(){
		if(AddressSize16){
			eip+=2;
			return (uint32_t) ReadCode16(1-2);
		}
		eip+=4;
		return ReadCode32(1-4);
	}


    inline uint8_t Reg8(int which){
        if(which < 4){
            //0-3 is low bytes; al, cl, dl, bl
            return regs32[which] & 0xFF;
        }else{
            //4-7 is high bytes; ah, ch, dh, bh
            return (regs32[which - 4] & 0xFF00) >> 8;
        }
    }
    inline uint16_t Reg16(int which){
        uint32_t tmp = regs32[which];
        return (tmp & 0xFFFF);
    }

    inline void SetReg8(int which, uint8_t val){
        if(which < 4){
            //0-3 is low bytes; al, cl, dl, bl
            regs32[which] = (regs32[which] & 0xFFFFFF00) | val;
        }else{
            //4-7 is high bytes; ah, ch, dh, bh
            regs32[which - 4] = (regs32[which - 4] & 0xFFFF00FF) | (val << 8);
        }
    }
    inline void SetReg16(int which, uint16_t val){
        regs32[which] = (regs32[which] & 0xFFFF0000) | val;
    }
    //no real need for 32bit versions, but for consistency..
    inline void SetReg32(int which, uint32_t val){
        regs32[which] = val;
    }
    inline uint32_t Reg32(int which){
        return regs32[which];
    }

    inline uint32_t Reg(int which){
        if(OperandSize16){
            return Reg16(which);
        }else{
            return regs32[which];
        }
    }
	inline uint32_t RegA(int which){
		if(AddressSize16){
            return Reg16(which);
		}else{
			return regs32[which];
		}
	}
	inline void SetReg(int which, uint32_t val){
		if(OperandSize16){
            SetReg16(which, val);
		}else{
			regs32[which] = val;
		}
	}
	inline int OperandSize(){
		return OperandSize16 ? 2 : 4;
	}

};



}


#endif

