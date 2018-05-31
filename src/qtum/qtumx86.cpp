#include <util.h>
#include <tinyformat.h>
#include <algorithm>
#include "qtumx86.h"

#include <x86lib.h>
#include <validation.h>

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
#define STACK_ADDRESS 0x200000
#define MAX_STACK_SIZE 1024 * 8

#define TX_DATA_ADDRESS 0xD0000000
#define TX_DATA_ADDRESS_END 0xF0000000

#define TX_CALL_DATA_ADDRESS 0x210000

bool x86ContractVM::execute(ContractOutput &output, ContractExecutionResult &result, bool commit)
{
    //default results
    result.usedGas = output.gasLimit;
    result.refundSender = 0;
    result.status = ContractStatus::CODE_ERROR;
    const uint8_t *code;
    const uint8_t *data;
    const uint8_t *options;
    ContractMapInfo *map;
    std::vector<uint8_t> bytecode;
    if(output.OpCreate) {
        map = parseContractData(output.data.data(), &code, &data, &options);
        if (map->optionsSize != 0) {
            LogPrintf("Options specified in x86 contract, but none exist yet!");
            return false;
        }
    }else {

        pdeltaDB->readByteCode(output.address, bytecode);
        map = parseContractData(bytecode.data(), &code, &data, &options);
    }

    MemorySystem memory;
    ROMemory codeMemory(MAX_CODE_SIZE, "code");
    RAMemory dataMemory(MAX_DATA_SIZE, "data");
    RAMemory stackMemory(MAX_STACK_SIZE, "stack");
    std::vector<uint8_t> txData = buildAdditionalData(output);
    PointerROMemory txDataMemory(txData.data(), txData.size(), "txdata");

    //TODO how is .bss loaded!?

    //zero memory for consensus
    memset(codeMemory.GetMemory(), 0, MAX_CODE_SIZE);
    memset(dataMemory.GetMemory(), 0, MAX_DATA_SIZE);
    memset(stackMemory.GetMemory(), MAX_STACK_SIZE, 0);

    //init memory
    memcpy(codeMemory.GetMemory(), code, map->codeSize);
    memcpy(dataMemory.GetMemory(), data, map->dataSize);

    MemorySystem memsys;
    memsys.Add(CODE_ADDRESS, CODE_ADDRESS + MAX_CODE_SIZE, &codeMemory);
    memsys.Add(DATA_ADDRESS, DATA_ADDRESS + MAX_DATA_SIZE, &dataMemory);
    memsys.Add(STACK_ADDRESS, STACK_ADDRESS + MAX_STACK_SIZE, &stackMemory);
    memsys.Add(TX_DATA_ADDRESS, TX_DATA_ADDRESS_END, &txDataMemory);

    PointerROMemory callDataMemory(output.data.data(), output.data.size(), "call-data");
    if(!output.OpCreate){
        //load call data into memory space if not create
        memsys.Add(TX_CALL_DATA_ADDRESS, TX_CALL_DATA_ADDRESS + output.data.size(), &callDataMemory);
    }

    QtumHypervisor qtumhv(*this, output);

    x86CPU cpu;
    cpu.Memory = &memsys;
    cpu.Hypervisor = &qtumhv;
    try{
        cpu.Exec(output.gasLimit);
    }
    catch(CPUFaultException err){
        LogPrintf("CPU Panic! Message: %s, code: %x, opcode: %s, hex: %x", err.desc, err.code, cpu.GetLastOpcodeName(), cpu.GetLastOpcode());
        result.usedGas = output.gasLimit;
        result.refundSender = output.value;
        return false;
    }
    catch(MemoryException *err){
        LogPrintf("Memory error! address: %x, opcode: %s, hex: %x", err->address, cpu.GetLastOpcodeName(), cpu.GetLastOpcode());
        result.usedGas = output.gasLimit;
        result.refundSender = output.value;
        return false;
    }
    LogPrintf("Execution successful!");
    if(output.OpCreate){
        //no error, so save to database
        pdeltaDB->writeByteCode(output.address, output.data);
    }else{
        //later, store a receipt or something
    }

    result.usedGas = std::min((uint64_t)1000, output.gasLimit);
    result.refundSender = 0;
    result.status = ContractStatus::SUCCESS;

    return true;
}

struct TxDataABI{
    uint32_t size;
    uint32_t callDataSize;
    UniversalAddressABI sender;
}  __attribute__((__packed__));

const std::vector<uint8_t> x86ContractVM::buildAdditionalData(ContractOutput &output) {
    std::vector<uint8_t> data;
    data.resize(0x1000);
    uint8_t* p=data.data();
    int i=0;
    *((uint32_t*)&p[i]) = 0x1000; //data size
    i+=4;
    *((uint32_t*)&p[i]) = output.data.size();
    i+=4;
    UniversalAddressABI sender = output.sender.toAbi();
    *((UniversalAddressABI*)&p[i]) = sender;
    i += 33;
    return data;
}

void QtumHypervisor::HandleInt(int number, x86Lib::x86CPU &vm)
{
    if(number == 0xF0){
        //exit code
        vm.Stop();
        return;
    }
    if(number != QtumEndpoint::QtumSystem){
        LogPrintf("Invalid interrupt endpoint received");
        vm.Int(QTUM_SYSTEM_ERROR_INT);
        return;
    }
    uint32_t status = 0;
    switch(vm.GetRegister32(EAX)){
        case QtumSystemCall::PreviousBlockTime:
            status = contractVM.getEnv().blockTime;
            break;
        case QtumSystemCall ::BlockCreator:
        {
            auto creator = contractVM.getEnv().blockCreator.toAbi();
            vm.WriteMemory(vm.Reg32(EBX), sizeof(creator), &creator);
        }
        case QtumSystemCall ::BlockDifficulty:
            vm.WriteMemory(vm.Reg32(EBX), sizeof(uint64_t), (void*) &contractVM.getEnv().difficulty);
            break;
        case QtumSystemCall::BlockGasLimit:
            vm.WriteMemory(vm.Reg32(EBX), sizeof(uint64_t), (void*) &contractVM.getEnv().gasLimit);
            break;
        case QtumSystemCall::BlockHeight:
            status = contractVM.getEnv().blockNumber;
            break;
        case QtumSystemCall::IsCreate:
            status = output.OpCreate ? 1 : 0;
            break;
        case 0xFFFF0001:
            //internal debug printf
            //Remove before production!
            //ecx is string length
            //ebx is string pointer
            char* msg = new char[vm.GetRegister32(ECX) + 1];
            vm.ReadMemory(vm.GetRegister32(EBX), vm.GetRegister32(ECX), msg, Data);
            msg[vm.GetRegister32(ECX)] = 0; //null termination
            LogPrintf("Contract message: ");
            LogPrintf(msg);
            status = 0;
            delete[] msg;
            break;
    }
    vm.SetReg32(EAX, status);
    return;
}
