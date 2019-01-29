#ifndef QTUMX86_H
#define QTUMX86_H

#include "qtumstate.h"
#include "qtumtransaction.h"
#include "uint256.h"
#include <map>
#include <utility>
#include <x86lib.h>

#include "shared-x86.h"

class ContractVM;

class QtumHypervisor;

class x86ContractVM : public ContractVM{
public:
    x86ContractVM(DeltaDBWrapper &db, const ContractEnvironment &env, uint64_t remainingGasLimit)
            : ContractVM(db, env, remainingGasLimit)
    {}
    virtual bool execute(ContractOutput &output, ContractExecutionResult &result, bool commit);

    BlockDataABI getBlockData();
    TxDataABI getTxData();
private:
    const ContractEnvironment &getEnv();
    const std::vector<uint8_t> buildAdditionalData(ContractOutput &output);

    void pushArguments(QtumHypervisor& hv, std::vector<uint8_t> args);

    friend class QtumHypervisor;
};

class QtumHypervisor;

typedef uint32_t (QtumHypervisor::*SyscallFunction)(uint32_t, x86Lib::x86CPU&);

//ability to read state
#define QSCCAP_READSTATE 1
//ability to write state
#define QSCCAP_WRITESTATE 2
//ability to send coins
#define QSCCAP_SENDCOINS 4
//ability to call contract (and potentially change state)
#define QSCCAP_CALL 8
//ability to self destruct
#define QSCCAP_DESTRUCT 16
//ability to read and write events
#define QSCCAP_EVENTS 32
//ability to get basic blockchain info
#define QSCCAP_BLOCKCHAIN 64

struct QtumSyscall{
    SyscallFunction function;
    uint64_t gasCost;
    int caps;
    QtumSyscall(SyscallFunction f, int c=0, uint64_t cost=1){
        function = f;
        gasCost = cost;
        caps = c;
    }
    QtumSyscall(){}
};

struct HypervisorEffect{
    int exitCode = 0;
    int64_t gasUsed = 0;
    std::map<std::string, std::string> events;
    std::vector<ContractExecutionResult> callResults;
};

class x86VMData{
    //because memory management is awful
    x86Lib::MemorySystem memory;
    x86Lib::ROMemory code;
    x86Lib::RAMemory stack;
    x86Lib::RAMemory data;

    //todo, block/tx can be shared and not require more memory
    x86Lib::ROMemory block;
    x86Lib::ROMemory tx;
    x86Lib::ROMemory exec;
    public:
    x86VMData(){}
    friend QtumHypervisor;
};

class QtumHypervisor : public x86Lib::InterruptHypervisor{
    QtumHypervisor(x86ContractVM &vm, DeltaDBWrapper& db_, const ExecDataABI& execdata) : contractVM(vm), execData(execdata), db(db_){
        if(qsc_syscalls.size() == 0){
            setupSyscalls();
        }
        clearEffects();
    }
    virtual void HandleInt(int number, x86Lib::x86CPU &vm);
    HypervisorEffect getEffects(){
        return effects;
    }
    void clearEffects(){
        effects = HypervisorEffect();
        sccs = std::stack<std::vector<uint8_t>>();
        sccsSize = 0;
    }
    static void setupSyscalls();


    void pushSCCS(std::vector<uint8_t> v){
        sccs.push(v);
    }
    std::vector<uint8_t> popSCCS(){
        std::vector<uint8_t> tmp = sccs.top();
        sccs.pop();
        return tmp;
    }
    size_t sizeofSCCS(){
        return sccs.size();
    }

    bool initVM(const std::vector<uint8_t> bytecode, const BlockDataABI &block, const TxDataABI &tx);
    int64_t useGas(int64_t v){
        return cpu.addGasUsed(v);
    }
    ContractExecutionResult execute();
private:
    x86Lib::x86CPU cpu;
    bool initSubVM(const std::vector<uint8_t> bytecode, x86VMData& data);
    x86ContractVM &contractVM;
    const ExecDataABI &execData;
    DeltaDBWrapper &db;
    HypervisorEffect effects;
    std::stack<std::vector<uint8_t>> sccs; //smart contract communication stack
    size_t sccsSize;

    x86VMData vmdata;

    friend x86ContractVM;

    //syscalls map. Key is the syscall number
    //qsc is interrupt 0x40
    static std::map<uint32_t, QtumSyscall> qsc_syscalls;

    ContractExecutionResult execute(UniversalAddress address, ExecDataABI exec);


    //syscalls
    uint32_t BlockGasLimit(uint32_t,x86Lib::x86CPU&);
    uint32_t BlockCreator(uint32_t,x86Lib::x86CPU&);
    uint32_t BlockDifficulty(uint32_t,x86Lib::x86CPU&);
    uint32_t BlockHeight(uint32_t,x86Lib::x86CPU&);
    uint32_t GetBlockHash(uint32_t,x86Lib::x86CPU&);
    uint32_t IsCreate(uint32_t,x86Lib::x86CPU&);
    uint32_t SelfAddress(uint32_t,x86Lib::x86CPU&);
    uint32_t PreviousBlockTime(uint32_t,x86Lib::x86CPU&);
    uint32_t UsedGas(uint32_t,x86Lib::x86CPU&);
    uint32_t AddEvent(uint32_t,x86Lib::x86CPU&);

    uint32_t ReadStorage(uint32_t,x86Lib::x86CPU&);
    uint32_t WriteStorage(uint32_t,x86Lib::x86CPU&);
    uint32_t ReadExternalStorage(uint32_t syscall, x86Lib::x86CPU& vm);

    uint32_t SenderAddress(uint32_t syscall, x86Lib::x86CPU& vm);

    //SCCS
    uint32_t SCCSItemCount(uint32_t syscall, x86Lib::x86CPU& vm);
    uint32_t SCCSSize(uint32_t syscall, x86Lib::x86CPU& vm);
    uint32_t SCCSItemSize(uint32_t syscall, x86Lib::x86CPU& vm);
    uint32_t SCCSPop(uint32_t syscall, x86Lib::x86CPU& vm);
    uint32_t SCCSPeek(uint32_t syscall, x86Lib::x86CPU& vm);
    uint32_t SCCSPush(uint32_t syscall, x86Lib::x86CPU& vm);
    uint32_t SCCSDiscard(uint32_t syscall, x86Lib::x86CPU& vm);
    uint32_t SCCSClear(uint32_t syscall, x86Lib::x86CPU& vm);

    uint32_t CallContract(uint32_t syscall, x86Lib::x86CPU& vm);
    uint32_t ParseAddress(uint32_t syscall, x86Lib::x86CPU& vm);

    uint32_t GetBalance(uint32_t syscall, x86Lib::x86CPU& vm);

    uint32_t UpdateBytecode(uint32_t syscall, x86Lib::x86CPU& vm);
    
    uint32_t SelfCalled(uint32_t syscall, x86Lib::x86CPU& vm);

};



#endif
