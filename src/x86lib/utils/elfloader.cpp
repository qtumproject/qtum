#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "elfloader.h"

#include <iostream>

using namespace std;


bool validateElf(Elf32_Ehdr* hdr){
    //check if valid ELF file

    if(hdr->e_ident[EI_MAG0] != ELFMAG0) {
        return false;
    }
    if(hdr->e_ident[EI_MAG1] != ELFMAG1) {
        return false;
    }
    if(hdr->e_ident[EI_MAG2] != ELFMAG2) {
        return false;
    }
    if(hdr->e_ident[EI_MAG3] != ELFMAG3) {
        return false;
    }
    //now check if valid to be loaded into Qtum's x86 VM
    //if(!elf_check_file(hdr)) {
    //    return false;
    //}
    if(hdr->e_ident[EI_CLASS] != ELFCLASS32) {
        return false;
    }
    if(hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        return false;
    }
    if(hdr->e_machine != EM_386) {
        return false;
    }
    if(hdr->e_ident[EI_VERSION] != EV_CURRENT) {
        return false;
    }
    if(hdr->e_type != ET_EXEC) {
        return false;
    }
    //qtum rules:
    if(hdr->e_entry != CODE_ADDRESS){
        //if entry point is not code address, then we will insert a JMP instruction to go there
        if(hdr->e_entry < CODE_ADDRESS + 5){
            cout << "Not enough room to insert JMP instruction. Please use a .text load address of at least 0x1005" << endl;
            return false;
        }
    }
    return true;
}



//code is memory buffer available for readonly data
//data is memory buffer available for readwrite data
//code is 0x1000, data is 0x100000
bool loadElf(char* code, size_t* codeSize, char* data, size_t* dataSize, char* raw, size_t size){
    if(size < sizeof(Elf32_Ehdr)){
        return false;
    }

    Elf32_Ehdr *hdr = (Elf32_Ehdr*) raw;
    if(!validateElf(hdr)){
        return false;
    }

    *codeSize = 0;
    *dataSize = 0;
    for(int i=0;i<hdr->e_phnum;i++){
        if(hdr->e_phoff + (hdr->e_phentsize * i) + sizeof(Elf32_Phdr) > size){
            cout << "Not enough room in file to load program header #" << i << endl;
            cout << "size: 0x" << hex << size << ", offset: 0x" << hex << hdr->e_phoff << 
                " sizeof header: 0x" << hex << sizeof(Elf32_Phdr) << " phentsize: 0x" << hex << hdr->e_phentsize << endl; 
            return false;
        }
        Elf32_Phdr *phdr = (Elf32_Phdr*) &raw[hdr->e_phoff + (hdr->e_phentsize * i)];
        if(phdr->p_type != PT_LOAD){
            cout << "Ignoring non PT_LOAD program section #" << i << " of type " << phdr->p_type << endl; 
            continue;
        }
        if(phdr -> p_vaddr != phdr->p_paddr){
            cout << "Program segment #" << i << " has a vaddr that does not match paddr. Unsure what to do" << endl;
        }
        if(phdr->p_offset + phdr->p_filesz > size){
            cout << "Program segment #" << i << " loads more data from file than exists" << endl;
            return false;
        }
        if(phdr->p_vaddr >= CODE_ADDRESS && phdr->p_vaddr + phdr->p_memsz < CODE_ADDRESS + MAX_CODE_SIZE){
            //code segment
            if(phdr->p_flags & PF_W){
                cout << "Program segment #" << i << "tries to load writeable data to a readonly memory area" << endl;
                return false;
            }
            size_t segsize = (phdr->p_vaddr - CODE_ADDRESS) + phdr->p_filesz;
            if(segsize > *codeSize){
                *codeSize = segsize;
            }
            memcpy(&code[phdr->p_vaddr - CODE_ADDRESS], &raw[phdr->p_offset], phdr->p_filesz);
        }else if(phdr->p_vaddr >= DATA_ADDRESS && phdr->p_vaddr + phdr->p_memsz < DATA_ADDRESS + MAX_DATA_SIZE){
            //data segment
            if(!(phdr->p_flags & PF_W)){
                cout << "Warning: Program segment #" << i << "Loads readonly data into readwrite memory" << endl;
                cout << "It may be cheaper in gas costs to relocate this data into readonly memory" << endl;
            }
            size_t segsize = (phdr->p_vaddr - DATA_ADDRESS) + phdr->p_filesz;
            if(segsize > INT32_MAX){
                cout << "data size overflow!" << endl;
                return false;
            }
            if(segsize > *dataSize){
                *dataSize = segsize;
            }
            memcpy(&data[phdr->p_vaddr - DATA_ADDRESS], &raw[phdr->p_offset], phdr->p_filesz);
        }else{
            cout << "Program segment #" << i << " loads into an invalid address or occupies more space than available" << endl;
            cout << "Address: 0x" << hex << phdr->p_vaddr << ", Size: 0x" << hex << phdr->p_memsz << endl;
            return false;
        }
        cout << "Loaded segment #" << i << " at address 0x" << hex << phdr->p_vaddr << " of size 0x" << hex << phdr->p_memsz << endl;
    }

    if(hdr->e_entry != CODE_ADDRESS){
        cout << "NOTE: inserting JMP instruction to go to ELF entry point @ 0x" << hex << hdr->e_entry << endl;
        code[0] = 0xE9; //jmp rel32
        int32_t entry = hdr->e_entry;
        entry -= CODE_ADDRESS; //make a relative address to code[0]
        //entry -= 5; //account for size of JMP rel32 instruction
        if(entry < 0){
            cout << "Entry point is negative!" << endl;
            return false;
        }
        if(entry > 1000){
            cout << "Entry point might be insane!" << endl;
            return false;
        }
        memcpy(&code[1], &entry, sizeof(uint32_t));
    }

    return true;
}


