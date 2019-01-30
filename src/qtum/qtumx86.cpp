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

BlockDataABI x86ContractVM::getBlockData(){
    BlockDataABI b;
    b.blockCreator = env.blockCreator.toAbi();
    b.blockDifficulty = env.difficulty;
    b.blockGasLimit = env.gasLimit;
    b.blockHeight = env.blockNumber;
    b.previousTime = env.blockTime;
    for(int i = 0; i < 256 ; i++){
        memcpy(&b.blockHashes[i].data, env.blockHashes[i].begin(), 32);
    }
    b.size = sizeof(b);
    return b;
}

TxDataABI x86ContractVM::getTxData(){
    return TxDataABI();
}


bool x86ContractVM::execute(ContractOutput &output, ContractExecutionResult &result, bool commit)
{
    //default results
    result.usedGas = output.gasLimit;
    result.refundSender = 0;
    result.commitState = false;
    result.status = ContractStatus::CodeError();
    //const uint8_t *options; //todo: eventually need to get this from initVM
    std::vector<uint8_t> bytecode;

    if(output.OpCreate) {
        bytecode = output.data;
    }else {
        db.readByteCode(output.address, bytecode);
    }
    if(bytecode.size() <= sizeof(ContractMapInfo)){
        result.status = ContractStatus::CodeError("Contract bytecode is not big enough to be valid");
        return false;
    }


    BlockDataABI blockdata = getBlockData();
    TxDataABI txdata = getTxData();

    ExecDataABI execdata;
    execdata.gasLimit = output.gasLimit;
    execdata.isCreate = output.OpCreate;
    execdata.nestLevel = 0;
    execdata.origin = output.sender.toAbi();
    execdata.sender = execdata.origin;
    execdata.size = sizeof(execdata);
    execdata.valueSent = output.value;
    execdata.self = output.address.toAbi();


    QtumHypervisor qtumhv(*this, db, execdata);
    if(!output.OpCreate){
        //load call data into memory space if not create
        pushArguments(qtumhv, output.data);
    }
    qtumhv.initVM(bytecode, blockdata, txdata);
    qtumhv.cpu.addGasUsed(50000); //base execution cost
    try{
        qtumhv.cpu.Exec(INT32_MAX);
    }
    catch(CPUFaultException err){
        std::string msg;
        msg = tfm::format("CPU Panic! Message: %s, code: %x, opcode: %s, hex: %x, location: %x\n", err.desc, err.code, qtumhv.cpu.GetLastOpcodeName(), qtumhv.cpu.GetLastOpcode(), qtumhv.cpu.GetLocation());
        result.modifiedData = db.getLatestModifiedState();
        result.status = ContractStatus::CodeError(msg);
        result.usedGas = output.gasLimit;
        result.refundSender = output.value;
        result.events = qtumhv.getEffects().events;
        result.callResults = qtumhv.getEffects().callResults;
        return false;
    }
    catch(MemoryException *err){
        std::string msg;
        msg = tfm::format("Memory error! address: %x, opcode: %s, hex: %x, location: %x\n", err->address, qtumhv.cpu.GetLastOpcodeName(), qtumhv.cpu.GetLastOpcode(), qtumhv.cpu.GetLocation());
        result.modifiedData = db.getLatestModifiedState();
        result.status = ContractStatus::CodeError(msg);
        result.usedGas = output.gasLimit;
        result.refundSender = output.value;
        result.events = qtumhv.getEffects().events;
        result.callResults = qtumhv.getEffects().callResults;
        return false;
    }
    HypervisorEffect effects = qtumhv.getEffects();
    result.address = output.address;
    result.modifiedData = db.getLatestModifiedState();
    result.usedGas = (uint64_t)qtumhv.cpu.getGasUsed();
    result.refundSender = 0;
    result.events = qtumhv.getEffects().events;
    result.callResults = qtumhv.getEffects().callResults;

    if(qtumhv.cpu.gasExceeded()){
        LogPrintf("Execution ended due to OutOfGas. Gas used: %i\n", qtumhv.cpu.getGasUsed());
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
        result.modifiedData = db.getLatestModifiedState();
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

void x86ContractVM::pushArguments(QtumHypervisor& hv, std::vector<uint8_t> args){
    for(size_t i=0;i<args.size();){
        uint32_t size=0;
        std::vector<uint8_t> buffer;
        if(args.size() - i < sizeof(uint32_t)){
            return;
        }
        memcpy(&size, &args.data()[i], sizeof(uint32_t));
        i += sizeof(uint32_t);
        if(args.size() - i < size){
            return;
        }
        buffer.resize(size);
        memcpy(buffer.data(), &args.data()[i], size);
    }
}


bool QtumHypervisor::initVM(const std::vector<uint8_t> bytecode, const BlockDataABI &block, const TxDataABI &tx){
    const uint8_t *code;
    const uint8_t *data;
    const uint8_t *options;
    ContractMapInfo *map;
    if(bytecode.size() <= sizeof(ContractMapInfo)){
        //result.status = ContractStatus::CodeError("Contract bytecode is not big enough to be valid");
        return false;
    }
    map = parseContractData(bytecode.data(), &code, &data, &options);

    MemorySystem memory;
    //note, Init will zero memory allocated
    vmdata.code.Init(MAX_CODE_SIZE, "code");
    vmdata.data.Init(MAX_DATA_SIZE, "data");
    vmdata.stack.Init(MAX_STACK_SIZE, "stack");

    vmdata.block.Init(sizeof(BlockDataABI), "block");
    vmdata.tx.Init(1, "tx"); //TODO, this is dynamic size
    vmdata.exec.Init(sizeof(ExecDataABI), "exec");

    //init memory
    vmdata.code.BypassWrite(0, map->codeSize, code);
    vmdata.data.Write(0, map->dataSize, data);
    //stack is not written to
    vmdata.block.BypassWrite(0, sizeof(BlockDataABI), &block);
    //todo tx
    vmdata.exec.BypassWrite(0, sizeof(ExecDataABI), &execData);

    MemorySystem memsys;
    vmdata.memory.Add(CODE_ADDRESS, CODE_ADDRESS + MAX_CODE_SIZE, &vmdata.code);
    vmdata.memory.Add(DATA_ADDRESS, DATA_ADDRESS + MAX_DATA_SIZE, &vmdata.data);
    vmdata.memory.Add(STACK_ADDRESS, STACK_ADDRESS + MAX_STACK_SIZE, &vmdata.stack);

    vmdata.memory.Add(BLOCK_DATA_ADDRESS, BLOCK_DATA_ADDRESS + sizeof(BlockDataABI), &vmdata.block);
    //todo tx
    vmdata.memory.Add(EXEC_DATA_ADDRESS, EXEC_DATA_ADDRESS + sizeof(ExecDataABI), &vmdata.exec);

    cpu.Memory = &vmdata.memory;
    cpu.Hypervisor = this;
    cpu.setGasLimit(execData.gasLimit);
    return true;
}

bool QtumHypervisor::initSubVM(const std::vector<uint8_t> bytecode, x86VMData& parentvmdata){
    const uint8_t *code;
    const uint8_t *data;
    const uint8_t *options;
    ContractMapInfo *map;
    if(bytecode.size() <= sizeof(ContractMapInfo)){
        //result.status = ContractStatus::CodeError("Contract bytecode is not big enough to be valid");
        return false;
    }
    map = parseContractData(bytecode.data(), &code, &data, &options);

    MemorySystem memory;
    //note, Init will zero memory allocated
    vmdata.code.Init(MAX_CODE_SIZE, "code");
    vmdata.data.Init(MAX_DATA_SIZE, "data");
    vmdata.stack.Init(MAX_STACK_SIZE, "stack");

    vmdata.block.Init(sizeof(BlockDataABI), "block");
    vmdata.tx.Init(1, "tx"); //TODO, this is dynamic size
    vmdata.exec.Init(sizeof(ExecDataABI), "exec");

    //init memory
    vmdata.code.BypassWrite(0, map->codeSize, code);
    vmdata.data.Write(0, map->dataSize, data);
    //stack is not written to
    //todo tx
    vmdata.exec.BypassWrite(0, sizeof(ExecDataABI), &execData);

    MemorySystem memsys;
    vmdata.memory.Add(CODE_ADDRESS, CODE_ADDRESS + MAX_CODE_SIZE, &vmdata.code);
    vmdata.memory.Add(DATA_ADDRESS, DATA_ADDRESS + MAX_DATA_SIZE, &vmdata.data);
    vmdata.memory.Add(STACK_ADDRESS, STACK_ADDRESS + MAX_STACK_SIZE, &vmdata.stack);

    vmdata.memory.Add(BLOCK_DATA_ADDRESS, BLOCK_DATA_ADDRESS + sizeof(BlockDataABI), &parentvmdata.block);
    //todo tx
    vmdata.memory.Add(EXEC_DATA_ADDRESS, EXEC_DATA_ADDRESS + sizeof(ExecDataABI), &vmdata.exec);

    cpu.Memory = &vmdata.memory;
    cpu.Hypervisor = this;
    cpu.setGasLimit(execData.gasLimit);
    return true;
}

void QtumHypervisor::HandleInt(int number, x86Lib::x86CPU &vm)
{
    //available registers:
    //status: EAX
    //arguments (in order) : EBX, ECX, EDX, ESI, EDI, EBP
    if(number == 0xF0){
        //exit code
        
        effects.exitCode = vm.Reg32(EAX) & 0x07; //only allow success, has data, error, or revert here
        vm.Stop();
        return;
    }
    if(number != QtumEndpoint::QtumSystem){
        LogPrintf("Invalid interrupt endpoint received");
        vm.Int(QTUM_SYSTEM_ERROR_INT);
        return;
    }
    uint32_t syscall = vm.Reg32(EAX);
    if(qsc_syscalls.find(syscall) == qsc_syscalls.end()){
        LogPrintf("Invalid Qtum syscall received");
        vm.Int(QTUM_SYSTEM_ERROR_INT);
        return;
    }
    QtumSyscall s = qsc_syscalls[syscall];
    vm.addGasUsed(s.gasCost);
    vm.SetReg32(EAX, (this->*s.function)(syscall, vm));
    return;
}

uint32_t QtumHypervisor::AddEvent(uint32_t syscall, x86Lib::x86CPU &vm){
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

    effects.events[std::string(&key[0], &key[keysize])] = 
        std::string(&value[0], &value[valuesize]);

    vm.addGasUsed(100 + ((valuesize + keysize) * 1));
    //we could use status to return if a key was overwritten, but leaving that blind
    //allows us to more easily change implementation in the future
    return 0;
}

uint32_t QtumHypervisor::ReadStorage(uint32_t syscall, x86Lib::x86CPU &vm){
        //ebx = key, ecx = key size
        //edx = value, esi = max value size
        //eax = actual value size
        uint32_t status = 0;
        unsigned char *k = new unsigned char[vm.Reg32(ECX)];
        vm.ReadMemory(vm.Reg32(EBX), vm.Reg32(ECX), k);
        valtype key(k,k+vm.Reg32(ECX));
        valtype value;
        bool ret;
        ret = db.readState(UniversalAddress(execData.self), key, value);
        if(ret==true){
            status = (value.size() <= vm.Reg32(ESI))? value.size() : vm.Reg32(ESI);					
            vm.WriteMemory(vm.Reg32(EDX), status, value.data());
        }
        vm.addGasUsed(500 + ((key.size() + value.size()) * 1));
        delete []k;
        return status;
}

uint32_t QtumHypervisor::ReadExternalStorage(uint32_t syscall, x86Lib::x86CPU& vm){
    //eax = actual size
    //ebx = key, ecx = key size
    //edx = value, esi = max value size
    //edi = universal address
    uint32_t status = 0;
    unsigned char *k = new unsigned char[vm.Reg32(ECX)];
    vm.ReadMemory(vm.Reg32(EBX), vm.Reg32(ECX), k);
    valtype key(k,k+vm.Reg32(ECX));
    valtype value;
    bool ret;
    UniversalAddressABI a;
    vm.ReadMemory(vm.Reg32(EDI), sizeof(UniversalAddressABI), &a, Syscall);
    ret = db.readState(UniversalAddress(a), key, value);
    if(ret==true){
        status = (value.size() <= vm.Reg32(ESI))? value.size() : vm.Reg32(ESI);					
        vm.WriteMemory(vm.Reg32(EDX), status, value.data());
    }
    vm.addGasUsed(500 + ((key.size() + value.size()) * 1));
    delete []k;
    return status;
}

uint32_t QtumHypervisor::UpdateBytecode(uint32_t syscall, x86Lib::x86CPU& vm){
    //eax = success
    //ebx = bytecode, ecx = bytecode size
    //edx = flag options, esi = flag options size
    unsigned char *v = new unsigned char[vm.Reg32(ESI)];
    vm.ReadMemory(vm.Reg32(EBX), vm.Reg32(ESI), v);
    valtype value(v,v+vm.Reg32(ESI));
    db.writeByteCode(UniversalAddress(execData.self), value);
    vm.addGasUsed(10000 + ((value.size()) * 30));
    delete []v;
    return 0;
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
    db.writeState(UniversalAddress(execData.self), key, value);
    vm.addGasUsed(1000 + ((value.size() + key.size()) * 20));
    delete []k;
    delete []v;
    return 0;
}

uint32_t QtumHypervisor::UsedGas(uint32_t syscall, x86Lib::x86CPU &vm){
    return 0;
}

uint32_t QtumHypervisor::SCCSItemCount(uint32_t syscall, x86Lib::x86CPU& vm){
    return sccs.size();
}
uint32_t QtumHypervisor::SCCSSize(uint32_t syscall, x86Lib::x86CPU& vm){
    return sccsSize;
}
uint32_t QtumHypervisor::SCCSItemSize(uint32_t syscall, x86Lib::x86CPU& vm){
    return sccs.top().size();
}
uint32_t QtumHypervisor::SCCSPop(uint32_t syscall, x86Lib::x86CPU& vm){
    //EBX = output buffer
    //ECX = buffer size
    //returns actual size
    if(sccs.size() == 0){
        return 0;
    }
    uint32_t actual = sccs.top().size();
    uint32_t size = std::min(actual, (uint32_t)vm.Reg32(ECX));
    vm.WriteMemory(vm.Reg32(EBX), size, sccs.top().data(), Syscall);
    sccs.pop();
    return actual;
}
uint32_t QtumHypervisor::SCCSPeek(uint32_t syscall, x86Lib::x86CPU& vm){
    //EBX = output buffer
    //ECX = buffer size
    //returns actual size
    if(sccs.size() == 0){
        return 0;
    }
    uint32_t actual = sccs.top().size();
    uint32_t size = std::min(actual, (uint32_t)vm.Reg32(ECX));
    vm.WriteMemory(vm.Reg32(EBX), size, sccs.top().data(), Syscall);
    return actual;
}
uint32_t QtumHypervisor::SCCSPush(uint32_t syscall, x86Lib::x86CPU& vm){
    //EBX = output buffer
    //ECX = buffer size
    //EAX = success
    std::vector<uint8_t> tmp;
    tmp.resize(vm.Reg32(ECX));
    vm.ReadMemory(vm.Reg32(EBX), vm.Reg32(ECX), tmp.data(), Syscall);
    sccs.push(tmp);
    //todo SCCS item and memory limits
    return 0;
}
uint32_t QtumHypervisor::SCCSDiscard(uint32_t syscall, x86Lib::x86CPU& vm){
    //EAX = 0
    sccs.pop();
    return 0;
}
uint32_t QtumHypervisor::SCCSClear(uint32_t syscall, x86Lib::x86CPU& vm){
    //EAX = 0
    sccs = std::stack<std::vector<uint8_t>>();
    return 0;
}

uint32_t QtumHypervisor::ParseAddress(uint32_t syscall, x86Lib::x86CPU& vm){
    //EAX = 0 if success, otherwise 1
    //EBX = address string ptr
    //ECX = string size
    //EDX = UniversalAdressABI buffer output
    std::vector<char> tmp;
    static const uint32_t maxsize = 40;
    tmp.resize(std::min(vm.Reg32(ECX), maxsize));
    vm.ReadMemory(vm.Reg32(EBX), tmp.size(), tmp.data(), Syscall);
    UniversalAddress a;
    CBitcoinAddress btc(std::string(tmp.begin(), tmp.end()));
    a.fromBitcoinAddress(btc);
    if(a.version == AddressVersion::UNKNOWN){
        return 1;
    }
    UniversalAddressABI abi = a.toAbi();
    vm.WriteMemory(vm.Reg32(EDX), sizeof(abi), &abi, Syscall);
    return 0;
}

uint32_t QtumHypervisor::GetBalance(uint32_t syscall, x86Lib::x86CPU& vm){
    //EAX = 0 if contract address, otherwise 1 (error)
    //EBX = address
    //ECX = uint64 output memory buffer
    UniversalAddressABI abi;
    vm.ReadMemory(vm.Reg32(EBX), sizeof(UniversalAddressABI), &abi, Syscall);
    UniversalAddress a(abi);
    if(!a.isContract()){
        return 1;
    }
    uint64_t b = db.getBalance(a);
    vm.WriteMemory(vm.Reg32(ECX), sizeof(uint64_t), &b, Syscall);
    return 0;
}

uint32_t QtumHypervisor::CallContract(uint32_t syscall, x86Lib::x86CPU& vm){
    //EAX = error code (0 for success)
    //EBX = address
    //ECX = gas provided
    //EDX = buffer for CallResultABI
    //ESI = size of buffer
    //EBP:EDI = value -- value = (EBP << 32) | EDI

    //rationale: CallResultABI could later be extended
    ExecDataABI exec;
    UniversalAddressABI addressabi;
    vm.ReadMemory(vm.Reg32(EBX), sizeof(UniversalAddressABI), &addressabi, Syscall);
    exec.gasLimit = std::min((uint64_t) vm.Reg32(ECX), (uint64_t) (execData.gasLimit - cpu.getGasUsed()));
    exec.nestLevel = execData.nestLevel + 1;
    exec.origin = execData.origin;
    exec.isCreate = false;
    exec.self = addressabi;
    exec.sender = execData.self;
    exec.size = sizeof(exec);
    exec.valueSent = (((uint64_t)vm.Reg32(EBP)) << 32) | (uint64_t)(vm.Reg32(EDI));
    std::vector<uint8_t> bytecode;
    if(!db.readByteCode(UniversalAddress(exec.self), bytecode)){
        return 1;
    }
    QtumHypervisor hv(contractVM, db, exec);
    hv.initSubVM(bytecode, vmdata);
    hv.sccs = this->sccs;
    db.checkpoint();
    ContractExecutionResult result = hv.execute();
    this->effects.callResults.push_back(result);

    //propogate results from sub execution into this one
    useGas(result.usedGas);
    //todo: refunds?
    if(result.commitState){
        //Go back to our own checkpoint, carrying the sub execution state with it
        db.condenseSingleCheckpoint();
    }else{
        hv.sccs = std::stack<std::vector<uint8_t>>(); //clear stack upon error
        db.revertCheckpoint(); //discard sub state
    }

    this->sccs = hv.sccs;
    std::vector<uint8_t> ret;
    QtumCallResultABI cr;
    cr.errorCode = hv.effects.exitCode;
    cr.refundedValue = result.refundSender;
    cr.usedGas = result.usedGas;

    cpu.WriteMemory(cpu.Reg32(EDX), std::min(cpu.Reg32(ESI), (uint32_t)sizeof(cr)), &cr, Syscall);
    return cr.errorCode;
}


ContractExecutionResult QtumHypervisor::execute(){
    ContractExecutionResult result;
    result.address = UniversalAddress(execData.self);
    result.commitState = false;
    try{
        cpu.Exec(INT32_MAX);
    }
    catch(CPUFaultException err){
        std::string msg;
        msg = tfm::format("CPU Panic! Message: %s, code: %x, opcode: %s, hex: %x, location: %x\n", err.desc, err.code, cpu.GetLastOpcodeName(), cpu.GetLastOpcode(), cpu.GetLocation());
        result.modifiedData = db.getLatestModifiedState();
        result.status = ContractStatus::CodeError(msg);
        result.usedGas = execData.gasLimit;
        result.refundSender = execData.valueSent;
        result.events = getEffects().events;
        effects.exitCode = QTUM_EXIT_CRASH | QTUM_EXIT_ERROR | QTUM_EXIT_REVERT;
        return result;
    }
    catch(MemoryException *err){
        std::string msg;
        msg = tfm::format("Memory error! address: %x, opcode: %s, hex: %x, location: %x\n", err->address, cpu.GetLastOpcodeName(), cpu.GetLastOpcode(), cpu.GetLocation());
        result.modifiedData = db.getLatestModifiedState();
        result.status = ContractStatus::CodeError(msg);
        result.usedGas = execData.gasLimit;
        result.refundSender = execData.valueSent;
        effects.exitCode = QTUM_EXIT_CRASH | QTUM_EXIT_ERROR | QTUM_EXIT_REVERT;
        result.events = getEffects().events;
        return result;
    }

    result.modifiedData = db.getLatestModifiedState();
    result.usedGas = (uint64_t)cpu.getGasUsed();
    result.refundSender = 0;
    result.events = effects.events;

    if(cpu.gasExceeded()){
        LogPrintf("Execution ended due to OutOfGas. Gas used: %i\n", cpu.getGasUsed());
        result.status = ContractStatus::OutOfGas();
        result.refundSender = execData.valueSent;
        result.commitState = false;
        //potentially possible for usedGas to be greater than gasLimit, so set to gasLimit to prevent insanity
        result.usedGas = execData.gasLimit;
        return result;
    }

    if(effects.exitCode & QTUM_EXIT_REVERT){
        LogPrintf("Execution Reverting!\n");
        result.commitState = false;
        result.refundSender = execData.valueSent; //refund all
        return result;
    }
    if(effects.exitCode == QTUM_EXIT_SUCCESS || effects.exitCode == QTUM_EXIT_USER){
        LogPrintf("Execution successful!\n");
        result.status = ContractStatus::Success();
        result.modifiedData = db.getLatestModifiedState();
        result.commitState = true;
        return result;
    }

    LogPrintf("Execution ended with error: %i\n", effects.exitCode);
    result.status = ContractStatus::ReturnedError(std::to_string(effects.exitCode));
    return result;
}

std::map<uint32_t, QtumSyscall> QtumHypervisor::qsc_syscalls;
#define INSTALL_QSC(func, cap) do {qsc_syscalls[QSC_##func] = QtumSyscall(&QtumHypervisor::func, cap);}while(0)
#define INSTALL_QSC_COST(func, cap, cost) do {qsc_syscalls[QSC_##func] = QtumSyscall(&QtumHypervisor::func, cap, cost);}while(0)
void QtumHypervisor::setupSyscalls(){
    INSTALL_QSC_COST(AddEvent, QSCCAP_EVENTS, 100);
    INSTALL_QSC(UsedGas, 0);
    INSTALL_QSC_COST(ReadStorage, QSCCAP_READSTATE, 1000);
    INSTALL_QSC_COST(WriteStorage, QSCCAP_WRITESTATE, 5000);
    INSTALL_QSC_COST(ReadExternalStorage, QSCCAP_READSTATE, 2000);
    INSTALL_QSC_COST(UpdateBytecode, QSCCAP_WRITESTATE, 10000);

    INSTALL_QSC(SCCSItemCount, 0);
    INSTALL_QSC(SCCSSize, 0);
    INSTALL_QSC(SCCSItemSize, 0);
    INSTALL_QSC(SCCSPeek, 0);
    INSTALL_QSC(SCCSPop, 0);
    INSTALL_QSC(SCCSPush, 0);
    INSTALL_QSC(SCCSDiscard, 0);
    INSTALL_QSC(SCCSClear, 0);

    INSTALL_QSC_COST(CallContract, QSCCAP_CALL, 10000);

    INSTALL_QSC_COST(ParseAddress, 0, 10);

    INSTALL_QSC_COST(GetBalance, QSCCAP_BLOCKCHAIN, 100);
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