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
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#undef X86LIB_BUILD //so we don't need special makefile flags for this specific file.
#include <x86lib.h>

#include <elfloader.h>

using namespace std;
using namespace x86Lib;

class MapMemory : public MemoryDevice{
    bool range(uint32_t val, uint32_t min, uint32_t len){
        return val >= min && val < min+len;
    }
public:
    virtual void Read(uint32_t address,int count,void *buffer){
        //nothing yet
        memset(buffer, 0, count);
    }
    virtual void Write(uint32_t address,int count,void *buffer){
        //note memmap prefix is already subtracted out
        if(range(address, 0, 4)) {
            cout << hex << *((uint32_t *) buffer) << endl;
        }else if(range(address, 4, 8)){
            char tmp[5];
            tmp[4]=0;
            memcpy(tmp, buffer, 4);
            cout << tmp << endl;
        }else{
            cout << "unknown memmap" << endl;
        }
    }
};

MemorySystem Memory;
RAMemory *scratchMem;

volatile bool int_cause;
volatile uint8_t int_cause_number=33;

void DumpMemory(){
	cout << "dumping memory!" << endl;
	uint32_t i;
	FILE *fh;
	fh=fopen("mem_dump.bin","w");
	uint8_t *tmp = new uint8_t[0x100000];
	Memory.Read(0x100000, 0x100000, tmp, Internal);
	for(i=0;i<=0x100000;i++){
		fputc(tmp[i],fh);
	}
	fclose(fh);
	delete[] tmp;
}

void WritePort(uint16_t port,uint32_t val){
	/*Not going to try to emulate actual hardware...but rather just give us some useful
	functions to try out...*/
	switch(port){
		case 0: //print ascii char of val
		cout << (char) val << flush;
		break;
		case 1: //print value of byte
		cout << hex << (int)(uint8_t)val << flush;
		break;
		case 2: //print value of word
		cout << hex << (int)(uint16_t)val << flush;
		break;
		case 3: //print value of dword
		cout << hex << (int)(uint32_t)val << flush;
		break;
		case 4: //cause an interrupt
		int_cause=1;
		break;

		case 0xF0: //exit with val
		cout << "exiting with code " << val << endl;
		exit(val);
		break;
		case 0xF3: /*Dump memory to external file*/
		DumpMemory();
		break;

		case 0x30: //prints value
		putchar(val);
		break;

		default:
		cout << "undefined port" << endl;
		break;
	}


}

uint32_t ReadPort(uint16_t port){
	switch(port){
		case 0x30:
		uint8_t tmp;
		cin.read((char*)&tmp,1);
		return tmp;
		break;

		default:
		cout << "undefined port" << endl;
		return 0;
		break;
	}
}

void port_read(x86CPU *thiscpu,uint16_t port,int size,void *buffer){
	uint32_t val;
	val=ReadPort(port);
	if(size==1){
		*(uint8_t*)buffer=val;
	}else if(size==2){
		*(uint16_t*)buffer=val;
	}else{
		throw;
	}
}
void port_write(x86CPU *thiscpu,uint16_t port,int size,void *buffer){
	uint32_t val;
	if(size==1){
		val=*(uint8_t*)buffer;
	}else if(size==2){
		val=(uint16_t)*(uint16_t*)buffer;
	}else if(size == 4){
		val=(uint32_t)*(uint32_t*)buffer;
	}else{
		throw;
	}
	WritePort(port,val);
}

class PCPorts : public PortDevice{
	public:
	~PCPorts(){}
	virtual void Read(uint16_t port,int size,void *buffer){
		port_read(NULL,port,size,buffer);
	}
	virtual void Write(uint16_t port,int size,void *buffer){
		port_write(NULL,port,size,buffer);
	}
};

PCPorts ports;

x86CPU *cpu;

void each_opcode(x86CPU *thiscpu){
	if(int_cause){
		int_cause=false;
		cpu->Int(int_cause_number);
	}
}


struct ContractMapInfo {
    //This structure is CONSENSUS-CRITICAL
    //Do not add or remove fields nor reorder them!
    uint32_t optionsSize;
    uint32_t codeSize;
    uint32_t dataSize;
    uint32_t reserved;
} __attribute__((__packed__));

static ContractMapInfo* parseContractData(const uint8_t* contract, const uint8_t** outputCode, const uint8_t** outputData, const uint8_t** outputOptions){
    ContractMapInfo *map = (ContractMapInfo*) contract;
    *outputOptions = &contract[sizeof(ContractMapInfo)];
    *outputCode = &contract[sizeof(ContractMapInfo) + map->optionsSize];
    *outputData = &contract[sizeof(ContractMapInfo) + map->optionsSize + map->codeSize];
    return map;
}

bool singleStep=false;
bool singleStepShort=false;
bool onlyAssemble=false;

int main(int argc, char* argv[]){
	if(argc < 2){
		cout << "./x86test {program.elf | program.bin} [-singlestep, -singlestep-short, -assemble]" << endl;
		return 1;
	}
	if(argc > 2){
		if(strcmp(argv[2], "-singlestep") == 0){
			singleStep = true;
		}
		if(strcmp(argv[2], "-singlestep-short") == 0){
			singleStep = true;
			singleStepShort=true;
		}
		if(strcmp(argv[2], "-assemble") == 0){
			//Assemble elf file into flat data suitable for Qtum blockchain
			onlyAssemble = true;
		}
	}

	//init_memory(argv[1]);
	PortSystem Ports;
	ROMemory coderom(0x1000, "code");
	RAMemory config(0x1000, "config");
	RAMemory scratch(0x100000, "scratch");
	RAMemory stack(0x1000, "stack");
	scratchMem = &scratch;
	
	Memory.Add(0x5, 0xFFF, &config);
	Memory.Add(0x1000, 0xFFFFF, &coderom);
	Memory.Add(0x100000, 0x1FFFFF, &scratch);
	Memory.Add(0x200000, 0x201000, &stack);

	int maxSize=0x10000;
	char* fileData=new char[maxSize];
	string fileName = argv[1];
	ifstream file(fileName.c_str(), ios::binary);
	if(!file){
		cout << "file " << argv[1] << " does not exist" << endl;
		exit(1);
	}
	int fileLength = file.tellg();
	file.seekg(0, std::ios::end);
	fileLength = (uint32_t) (((long)file.tellg()) - (long) fileLength);
	file.seekg(0, std::ios::beg);
	file.read(fileData, maxSize);

	memset(coderom.GetMemory(), 0x66, 0x1000);
	memset(scratch.GetMemory(), 0x00, 0x100000);

	bool doElf = false;
	string::size_type extensionIndex;
	extensionIndex = fileName.rfind('.');
	if(extensionIndex != string::npos){
		string extension = fileName.substr(extensionIndex + 1);
		if(extension == "elf"){
			doElf = true;
		}
	} //else no extension

	size_t codesize=0;
	size_t datasize=0;

	if(doElf){
		//load ELF32 file
		cout << "Found .elf file extension. Attempting to load ELF file" << endl;
		if(!loadElf(coderom.GetMemory(), &codesize, scratch.GetMemory(), &datasize, fileData, fileLength)){
			cout << "error loading ELF" << endl;
			return -1;
		}
	}else{
		//load BIN file (no option to load data with bin files)
		cout << "Attempting to load BIN file. Warning: It is not possible to load data with this" << endl;
		memcpy(coderom.GetMemory(), fileData, fileLength);
		datasize = 0;
		codesize = fileLength;
	}

	if(onlyAssemble){
		//don't execute anything, just dump the flat memory to a file
		int totalSize = 16 + codesize + datasize;
		cout << "code: " << codesize << " data: " << datasize << endl;
		char *out = new char[totalSize];
		ContractMapInfo map;
		map.optionsSize = 0;
		map.codeSize = codesize;
		map.dataSize = datasize;
		map.reserved = 0;
		memset(out, 0, totalSize);
		memcpy(&out[0], &map.optionsSize, sizeof(uint32_t));
		memcpy(&out[4], &map.codeSize, sizeof(uint32_t));
		memcpy(&out[8], &map.dataSize, sizeof(uint32_t));
		memcpy(&out[12], &map.reserved, sizeof(uint32_t));
		memcpy(&out[16], &coderom.GetMemory()[0], codesize);
		memcpy(&out[16 + codesize], scratch.GetMemory(), datasize);

		for(int i=0;i<totalSize;i++){
			cout << hex << setfill('0') << setw(2) << (int)(uint8_t)out[i];
		}
		cout << endl;
		delete[] out;
		return 0;
	}

	Ports.Add(0,0xFFFF,(PortDevice*)&ports);
	cpu=new x86CPU();
	cpu->Memory=&Memory;
	cpu->Ports=&Ports;
	cout << "Loaded! Beginning execution..." << endl;

	
	for(;;){
		try{
			if(!singleStep){
				cpu->Exec(1000);
				if(int_cause){
					int_cause=false;
					cpu->Int(int_cause_number);
				}
			}else{
				cpu->Exec(1);
				cout <<"OPCODE: " << cpu->GetLastOpcodeName() << "; hex: 0x" << hex << cpu->GetLastOpcode() << endl;
				if(singleStepShort){
					cout << "EIP: 0x" << cpu->GetLocation() << endl;
				}else{
					cpu->DumpState(cout);
					cout << "-------------------------------" << endl;
				}
				if(int_cause){
					int_cause=false;
					cpu->Int(int_cause_number);
				}
			}
		}
		catch(CPUFaultException err){
			cout << "CPU Panic!" <<endl;
			cout << "Message: " << err.desc << endl;
			cout << "Code: 0x" << hex << err.code << endl;
			cout <<"OPCODE: " << cpu->GetLastOpcodeName() << "; hex: 0x" << hex << cpu->GetLastOpcode() << endl;
			cpu->DumpState(cout);
			cout << endl;
			return 1;
		}
		catch(MemoryException *err){
			cout << "Memory Error!" <<endl;
			cout << "Address: 0x" << hex << err->address << endl;
			cout <<"OPCODE: " << cpu->GetLastOpcodeName() << "; hex: 0x" << hex << cpu->GetLastOpcode() << endl;
			cpu->DumpState(cout);
			cout << endl;
			throw;
			//return 1;
		}
	}
	return 0;
}













