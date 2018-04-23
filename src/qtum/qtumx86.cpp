#include <util.h>
#include <tinyformat.h>
#include "qtumx86.h"

#include <x86lib.h>

using namespace x86Lib;


//The data field available is only a flat data field, so we need some format for storing
//Code, data, and options.
//Thus, it is prefixed with 4 uint32 integers.
//1st, the size of options, 2nd the size of code, 3rd the size of data
//4th is unused (for now) but is kept for padding and alignment purposes

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


const ContractEnvironment& x86ContractVM::getEnv() {
    return env;
}

#define CODE_ADDRESS 0x1000
#define MAX_CODE_SIZE 0x10000
#define DATA_ADDRESS 0x100000
#define MAX_DATA_SIZE 0x10000

bool x86ContractVM::execute(ContractOutput &output, ContractExecutionResult &result, bool commit)
{
    if(output.OpCreate) {
        const uint8_t *code;
        const uint8_t *data;
        const uint8_t *options;
        ContractMapInfo *map;
        map = parseContractData(output.data.data(), &code, &data, &options);
        if (map->optionsSize != 0) {
            LogPrintf("Options specified in x86 contract, but none exist yet!");
            return false;
        }
        MemorySystem memory;
        ROMemory codeMemory(map->codeSize, "code");
        RAMemory dataMemory(map->dataSize, "data");
        //TODO how is .bss loaded!?

        //init memory
        memcpy(codeMemory.GetMemory(), code, map->codeSize);
        memcpy(dataMemory.GetMemory(), data, map->dataSize);

        MemorySystem memsys;
        memsys.Add(CODE_ADDRESS, CODE_ADDRESS + MAX_CODE_SIZE, &codeMemory);
        memsys.Add(DATA_ADDRESS, DATA_ADDRESS + MAX_DATA_SIZE, &dataMemory);

        QtumHypervisor qtumhv(*this);

        x86CPU cpu;
        cpu.Memory = &memsys;
        cpu.Hypervisor = &qtumhv;
        try{
            cpu.Exec(output.gasLimit);
        }
        catch(CPUFaultException err){
            LogPrintf("CPU Panic! Message: %s, code: %x, opcode: %s, hex: %x", err.desc, err.code, cpu.GetLastOpcodeName(), cpu.GetLastOpcode());
            return false;
        }
        catch(MemoryException *err){
            LogPrintf("Memory error! address: %x, opcode: %s, hex: %x", err->address, cpu.GetLastOpcodeName(), cpu.GetLastOpcode());
            return false;
        }
        LogPrintf("Execution successful!");


    }else{
        LogPrintf("Call currently not implemented");
        return false;
    }



    return true;
}


void QtumHypervisor::HandleInt(int number, x86Lib::x86CPU &vm)
{
    if(number == 0xF0){
        //exit code
        vm.Stop();
    }
    if(number != QtumEndpoint::QtumSystem){
        LogPrintf("Invalid interrupt endpoint received");
        vm.Int(QTUM_SYSTEM_ERROR_INT);
        return;
    }
    switch(vm.GetRegister32(EAX)){
        case QtumSystemCall::BlockHeight:
            vm.SetReg32(EAX, contractVM.getEnv().blockNumber);
            break;
    }
    return;
}
