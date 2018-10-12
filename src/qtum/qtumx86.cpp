#include <util.h>
#include <tinyformat.h>
#include <algorithm>
#include "qtumx86.h"

#include <x86lib.h>
#include <validation.h>

using namespace x86Lib;
//TODO: don't use a global for this so we can execute things in parallel
DeltaDB* pdeltaDB = nullptr;
DeltaDBWrapper* pdeltaWrapper = nullptr;

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
    result.commitState = false;
    result.status = ContractStatus::CodeError();
    const uint8_t *code;
    const uint8_t *data;
    const uint8_t *options;
    ContractMapInfo *map;
    std::vector<uint8_t> bytecode;
    if(output.OpCreate) {
        if(output.data.size() <= sizeof(ContractMapInfo)){
            result.status = ContractStatus::CodeError("Contract bytecode is not big enough to be valid");
            return false;
        }
        map = parseContractData(output.data.data(), &code, &data, &options);
        if (map->optionsSize != 0) {
            result.status = ContractStatus::CodeError("Option data is specified, but no options are valid yet for x86");
            return false;
        }
    }else {
        db.readByteCode(output.address, bytecode);
        if(bytecode.size() <= sizeof(ContractMapInfo)){
            result.status = ContractStatus::CodeError("Contract bytecode is not big enough to be valid");
            return false;
        }
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

    QtumHypervisor qtumhv(*this, output, db);

    x86CPU cpu;
    cpu.Memory = &memsys;
    cpu.Hypervisor = &qtumhv;
    cpu.setGasLimit(output.gasLimit);
    cpu.addGasUsed(50000); //base execution cost
    try{
        cpu.Exec(INT32_MAX);
    }
    catch(CPUFaultException err){
        std::string msg;
        msg = tfm::format("CPU Panic! Message: %s, code: %x, opcode: %s, hex: %x, location: %x\n", err.desc, err.code, cpu.GetLastOpcodeName(), cpu.GetLastOpcode(), cpu.GetLocation());
        result.modifiedData = db.getLatestModifiedState();
        result.status = ContractStatus::CodeError(msg);
        result.usedGas = output.gasLimit;
        result.refundSender = output.value;
        return false;
    }
    catch(MemoryException *err){
        std::string msg;
        msg = tfm::format("Memory error! address: %x, opcode: %s, hex: %x, location: %x\n", err->address, cpu.GetLastOpcodeName(), cpu.GetLastOpcode(), cpu.GetLocation());
        result.modifiedData = db.getLatestModifiedState();
        result.status = ContractStatus::CodeError(msg);
        result.usedGas = output.gasLimit;
        result.refundSender = output.value;
        result.returnValues = qtumhv.getEffects().returnValues;
        return false;
    }
    HypervisorEffect effects = qtumhv.getEffects();
    result.modifiedData = db.getLatestModifiedState();
    result.usedGas = (uint64_t)cpu.getGasUsed();
    result.refundSender = 0;
    result.returnValues = qtumhv.getEffects().returnValues;

    if(cpu.gasExceeded()){
        LogPrintf("Execution ended due to OutOfGas. Gas used: %i\n", cpu.getGasUsed());
        result.status = ContractStatus::OutOfGas();
        result.refundSender = output.value;
        result.commitState = false;
        //potentially possible for usedGas to be greater than gasLimit, so set to gasLimit to prevent insanity
        result.usedGas = output.gasLimit;
        return false;
    }
    if(effects.exitCode == 0) {
        LogPrintf("Execution successful!\n");
        if (output.OpCreate) {
            //no error, so save to database
            db.writeByteCode(output.address, output.data);
        } else {
            //later, store a receipt or something
        }
        result.status = ContractStatus::Success();
        result.commitState = true;
        return true;
    }else{
        LogPrintf("Execution ended with error: %i\n", effects.exitCode);
        result.usedGas = output.gasLimit;
        result.refundSender = output.value; //refund all
        result.status = ContractStatus::ReturnedError(std::to_string(effects.exitCode));
        result.commitState = false;
        return false;
    }
    return false;
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
    i+=4; //4
    *((uint32_t*)&p[i]) = output.data.size();
    i+=4; //8
    UniversalAddressABI sender = output.sender.toAbi();
    *((UniversalAddressABI*)&p[i]) = sender;
    i += 33; //41
    return data;
}

void QtumHypervisor::HandleInt(int number, x86Lib::x86CPU &vm)
{
    //available registers:
    //status: EAX
    //arguments (in order) : EBX, ECX, EDX, ESI, EDI, EBP
    if(number == 0xF0){
        //exit code
        effects.exitCode = vm.Reg32(EAX);
        vm.Stop();
        return;
    }
    if(number != QtumEndpoint::QtumSystem){
        LogPrintf("Invalid interrupt endpoint received");
        vm.Int(QTUM_SYSTEM_ERROR_INT);
        return;
    }
    uint32_t status = 0;
    //todo: estimate gas cost first, so that if out of gas no operation is needed
    int64_t gasCost = 1;

    vm.SetReg32(EAX, status);
    return;
}

uint32_t QtumHypervisor::BlockCreator(uint32_t syscall, x86Lib::x86CPU &vm){
    auto creator = contractVM.getEnv().blockCreator.toAbi();
    vm.WriteMemory(vm.Reg32(EBX), sizeof(creator), &creator);
    return 0;
}
uint32_t QtumHypervisor::BlockDifficulty(uint32_t syscall,x86Lib::x86CPU& vm){
    //ebx = block difficulty (64 bit integer)
    vm.WriteMemory(vm.Reg32(EBX), sizeof(uint64_t), (void*) &contractVM.getEnv().difficulty);
    return 0;
}
uint32_t QtumHypervisor::BlockHeight(uint32_t syscall, x86Lib::x86CPU &vm){
    //eax = block height
    return contractVM.getEnv().blockNumber;
}
uint32_t QtumHypervisor::GetBlockHash(uint32_t syscall, x86Lib::x86CPU &vm){
    return 0; //todo
}
uint32_t QtumHypervisor::IsCreate(uint32_t syscall, x86Lib::x86CPU &vm){
    //eax = 1 if contract creation is in progress
    return output.OpCreate ? 1 : 0;
}
uint32_t QtumHypervisor::SelfAddress(uint32_t syscall, x86Lib::x86CPU &vm){
    //ebx = address (address/33 byte buffer)
    UniversalAddressABI selfAddr = output.address.toAbi();
    vm.WriteMemory(vm.Reg32(EBX), sizeof(selfAddr), &selfAddr);
    return 0;
}
uint32_t QtumHypervisor::PreviousBlockTime(uint32_t syscall, x86Lib::x86CPU &vm){
    return contractVM.getEnv().blockTime;
}
uint32_t QtumHypervisor::UsedGas(uint32_t syscall, x86Lib::x86CPU &vm){
    return 0;
}
uint32_t QtumHypervisor::AddReturnData(uint32_t syscall, x86Lib::x86CPU &vm){
    //Adds a key value pair for the return data
    //ebx = key, ecx = key size
    //edx = value, esi = value size
    //edi = (key type) << 4 | value type 
    //eax = 0

    uint8_t keytype = (vm.GetRegister32(EDI) & 0xF0) >> 4;
    uint8_t valuetype = vm.GetRegister32(EDI) & 0x0F;

    //todo put in some memory limits
    uint32_t keysize = vm.GetRegister32(ECX) + 1;
    uint8_t* key = new uint8_t[keysize];
    key[0] = keytype;
    vm.ReadMemory(vm.GetRegister32(EBX), vm.GetRegister32(ECX), &key[1], MemAccessReason::Syscall);

    uint32_t valuesize = vm.GetRegister32(ESI) + 1;
    uint8_t* value = new uint8_t[valuesize];
    value[0] = valuetype;
    vm.ReadMemory(vm.GetRegister32(EDX), vm.GetRegister32(ESI), &value[1], MemAccessReason::Syscall);

    effects.returnValues[std::string(&key[0], &key[keysize])] = 
        std::string(&value[0], &value[valuesize]);

    vm.addGasUsed(100 + ((valuesize + keysize) * 1));
    //we could use status to return if a key was overwritten, but leaving that blind
    //allows us to more easily change implementation in the future
    return 0;
}

uint32_t QtumHypervisor::ReadStorage(uint32_t syscall, x86Lib::x86CPU &vm){
        //ebx = key, ecx = key size
        //edi = value, esi = max value size
        //eax = actual value size
        uint32_t status = 0;
        unsigned char *k = new unsigned char[vm.Reg32(ECX)];
        vm.ReadMemory(vm.Reg32(EBX), vm.Reg32(ECX), k);
        valtype key(k,k+vm.Reg32(ECX));
        valtype value;
        bool ret;
        ret = db.readState(output.address, key, value);
        if(ret==true){
            status = (value.size() <= vm.Reg32(ESI))? value.size() : vm.Reg32(ESI);					
            vm.WriteMemory(vm.Reg32(EDX), status, value.data());
        }
        vm.addGasUsed(500 + ((key.size() + value.size()) * 1));
        delete []k;
        return status;
}
uint32_t QtumHypervisor::WriteStorage(uint32_t syscall, x86Lib::x86CPU &vm){
    //ebx = key, ecx = key size
    //edx = value, esi = value size
    //eax = 0
    unsigned char *k = new unsigned char[vm.Reg32(ECX)];
    unsigned char *v = new unsigned char[vm.Reg32(ESI)];
    vm.ReadMemory(vm.Reg32(EBX), vm.Reg32(ECX), k);
    vm.ReadMemory(vm.Reg32(EDX), vm.Reg32(ESI), v);
    valtype key(k,k+vm.Reg32(ECX));
    valtype value(v,v+vm.Reg32(ESI));
    db.writeState(output.address, key, value);
    vm.addGasUsed(1000 + ((value.size() + key.size()) * 20));
    delete []k;
    delete []v;
    return 0;
}

uint32_t QtumHypervisor::SenderAddress(uint32_t syscall, x86Lib::x86CPU& vm){
    //ebx = address (address/33 byte buffer)
    UniversalAddressABI addr = output.sender.toAbi();
    vm.WriteMemory(vm.Reg32(EBX), sizeof(addr), &addr);
    return 0;
}

uint32_t QtumHypervisor::BlockGasLimit(uint32_t syscall, x86Lib::x86CPU& vm){
    //ebx = gas limit (64 bit integer)
    vm.WriteMemory(vm.Reg32(EBX), sizeof(uint64_t), (void*) contractVM.getEnv().gasLimit);
    return 0;
}

std::map<uint32_t, QtumSyscall> QtumHypervisor::qsc_syscalls;
#define INSTALL_QSC(func, cap) do {qsc_syscalls[QSC_##func] = QtumSyscall(&QtumHypervisor::func, cap);}while(0)
#define INSTALL_QSC_COST(func, cap, cost) do {qsc_syscalls[QSC_##func] = QtumSyscall(&QtumHypervisor::func, cap, cost);}while(0)
void QtumHypervisor::setupSyscalls(){
    INSTALL_QSC(BlockGasLimit, QSCCAP_BLOCKCHAIN);
    INSTALL_QSC(BlockCreator, QSCCAP_BLOCKCHAIN);
    INSTALL_QSC(BlockDifficulty, QSCCAP_BLOCKCHAIN);
    INSTALL_QSC_COST(AddReturnData, QSCCAP_EVENTS, 100);
    INSTALL_QSC(BlockHeight, QSCCAP_BLOCKCHAIN);
}


/*
#define ABI_TYPE_UNKNOWN 0
#define ABI_TYPE_INT 1
#define ABI_TYPE_UINT 2
#define ABI_TYPE_HEX 3
#define ABI_TYPE_STRING 4
#define ABI_TYPE_BOOL 5
#define ABI_TYPE_ADDRESS 6
*/


std::string parseABIToString(std::string abidata){
    uint8_t type = abidata[0];
    abidata = std::string(abidata.begin() + 1, abidata.end());
    switch(type){
        default:
        case ABI_TYPE_UNKNOWN:
        case ABI_TYPE_HEX:
            return HexStr(abidata);
        case ABI_TYPE_STRING:
            return abidata;
        case ABI_TYPE_BOOL:
            return abidata[0] > 0 ? "true" : "false";
        case ABI_TYPE_ADDRESS:
        {
            UniversalAddress tmp;
            if(abidata.size() != sizeof(UniversalAddressABI)){
                return "invalid address data";
            }
            UniversalAddressABI abi = *((const UniversalAddressABI*) abidata.data());
            UniversalAddress address(abi);
            return address.asBitcoinAddress().ToString();
        }
        case ABI_TYPE_INT:
        {
            if(abidata.size() == 1){
                int8_t tmp = *(int8_t*)abidata.data();
                return std::to_string((int) tmp);
            }else if(abidata.size() == 2){
                int16_t tmp = *(int16_t*)abidata.data();
                return std::to_string((int) tmp);
            }else if(abidata.size() == 4){
                int32_t tmp = *(int32_t*)abidata.data();
                return std::to_string((int) tmp);
            }else if(abidata.size() == 8){
                int64_t tmp = *(int64_t*)abidata.data();
                return std::to_string((int64_t) tmp);
            }
            return "invalid integer size";
        }
        case ABI_TYPE_UINT:
        {
            if(abidata.size() == 1){
                uint8_t tmp = *(uint8_t*)abidata.data();
                return std::to_string((unsigned int) tmp);
            }else if(abidata.size() == 2){
                uint16_t tmp = *(uint16_t*)abidata.data();
                return std::to_string((unsigned int) tmp);
            }else if(abidata.size() == 4){
                uint32_t tmp = *(uint32_t*)abidata.data();
                return std::to_string((unsigned int) tmp);
            }else if(abidata.size() == 8){
                uint64_t tmp = *(uint64_t*)abidata.data();
                return std::to_string((uint64_t) tmp);
            }
            return "invalid integer size";
        }
    }
}