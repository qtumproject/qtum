#ifndef QTUMX86_H
#define QTUMX86_H

#include "qtumstate.h"
#include "qtumtransaction.h"
#include "uint256.h"
#include <map>
#include <utility>
#include <x86lib.h>
class ContractVM;

class QtumHypervisor;

class x86ContractVM : public ContractVM{
public:
    x86ContractVM(DeltaDBWrapper &db, const ContractEnvironment &env, uint64_t remainingGasLimit)
            : ContractVM(db, env, remainingGasLimit)
    {}
    virtual bool execute(ContractOutput &output, ContractExecutionResult &result, bool commit);
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
};

class QtumHypervisor : public x86Lib::InterruptHypervisor{
    QtumHypervisor(x86ContractVM &vm, const ContractOutput& out, DeltaDBWrapper& db_) : contractVM(vm), output(out), db(db_){
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
        effects.exitCode = 0;
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
    
private:
    x86ContractVM &contractVM;
    ContractOutput output;
    DeltaDBWrapper &db;
    HypervisorEffect effects;
    std::stack<std::vector<uint8_t>> sccs; //smart contract communication stack
    size_t sccsSize;

    friend x86ContractVM;

    //syscalls map. Key is the syscall number
    //qsc is interrupt 0x40
    static std::map<uint32_t, QtumSyscall> qsc_syscalls;

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
};


//constants below this line should exactly match libqtum's qtum.h! 

static const int QTUM_SYSTEM_ERROR_INT = 0xFF;

//interrupt 0x40
//QSC = Qtum System Call
//use defines here so that we can use it in C easily

//basic execution calls, 0x0000
#define QSC_BlockGasLimit           1
#define QSC_BlockCreator            2
#define QSC_BlockDifficulty         3
#define QSC_BlockHeight             4
#define QSC_GetBlockHash            5
#define QSC_IsCreate                6
#define QSC_SelfAddress             7
#define QSC_PreviousBlockTime       8
#define QSC_UsedGas                 9
#define QSC_SetFaultHandler         10
#define QSC_SetDoubleFaultHandler   11
#define QSC_CodeSize                12
#define QSC_DataSize                13
#define QSC_ScratchSize             14
#define QSC_SelfDestruct            15

#define QSC_AddEvent                16
/* -- this quickly gets very complicated. Defer/cancel implementation
#define QSC_GetEvent                17
#define QSC_GetEventSize            18
#define QSC_ExecutingCallID         19
#define QSC_NextCallID              20
*/


    //storage commands, 0x1000
#define QSC_ReadStorage             0x1000
#define QSC_WriteStorage            0x1001

    //value commands, 0x2000
#define QSC_SendValue               0x2000 //send coins somewhere
#define QSC_GetBalance              0x2001
#define QSC_SendValueAndCall        0x2002

    //callee commands, 0x3000
#define QSC_GasProvided             0x3000
#define QSC_CallerTransaction       0x3001 //provides output scripts, etc
#define QSC_ValueProvided           0x3002
#define QSC_OriginAddress           0x3003
#define QSC_SenderAddress           0x3004
#define QSC_CallStackSize           0x3005
//SCCS = Smart Contract Communication Stack
//note: Upon contract error, this stack is not cleared. Thus an error contract can have side effects
#define QSC_SCCSItemCount               0x3006
//#define QSC_SCCSMaxItems            0x3007
//#define QSC_SCCSMaxSize             0x3008
#define QSC_SCCSSize                0x3009
#define QSC_SCCSItemSize            0x300A
#define QSC_SCCSPop                 0x300B
#define QSC_SCCSPeek                0x300C
#define QSC_SCCSPush                0x300D
#define QSC_SCCSDiscard             0x300E //pops off the stack without any data transfer possible (for cheaper gas cost)
#define QSC_SCCSClear               0x300F

    //caller commands, 0x4000
#define QSC_CallContract            0x4000
#define QSC_CallLibrary             0x4001

//error code types
//These will cause appropriate revert of state etc
//note, this is the last value pushed onto SCCS upon contract termination
#define QTUM_EXIT_SUCCESS 0 //successful execution
#define QTUM_EXIT_HAS_DATA 1 //there is user defined data pushed onto the stack (optional, no consensus function)
#define QTUM_EXIT_REVERT 2 //execution that reverted state
#define QTUM_EXIT_ERROR 4 //error execution (which may or may not revert state)
#define QTUM_EXIT_OUT_OF_GAS 8 //execution which ended in out of gas exception
#define QTUM_EXIT_CRASH 16 //execution which ended due to CPU or memory errors
#define QTUM_EXIT_SYSCALL_EXCEPTION 32 //execution which ended due to an exception by a syscall, such as transfering more money than the contract owns

//NOTE: only QTUM_EXIT_SUCCESS, QTUM_EXIT_ERROR, QTUM_EXIT_REVERT, and QTUM_HAS_DATA may be specified by __qtum_terminate


//ABI type prefixes
//note the limit for this is 15, since it should fit into a nibble
#define ABI_TYPE_UNKNOWN 0
#define ABI_TYPE_INT 1
#define ABI_TYPE_UINT 2
#define ABI_TYPE_HEX 3
#define ABI_TYPE_STRING 4
#define ABI_TYPE_BOOL 5
#define ABI_TYPE_ADDRESS 6

enum QtumEndpoint{
    QtumExit = 0xF0,
    QtumSystem = 0x40,
    QtumTrustedLibrary = 0x41,
    InteralUI = 0x50
};

//Read only fixed-size per-execution data
struct ExecDataABI{
    //total size of txdata area
    uint32_t size;

    uint32_t isCreate;
    UniversalAddressABI sender;
    uint64_t gasLimit;
    uint64_t valueSent;
    UniversalAddressABI origin;
    UniversalAddressABI self;
     
}  __attribute__((__packed__));



#endif
