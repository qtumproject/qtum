#ifndef QTUMX86_H
#define QTUMX86_H

#include "qtumstate.h"
#include "qtumtransaction.h"
#include "uint256.h"
#include <map>
#include <utility>
#include <x86lib.h>
class ContractVM;



class x86ContractVM : public ContractVM{
public:
    x86ContractVM(DeltaDBWrapper &db, const ContractEnvironment &env, uint64_t remainingGasLimit)
            : ContractVM(db, env, remainingGasLimit)
    {}
    virtual bool execute(ContractOutput &output, ContractExecutionResult &result, bool commit);
private:
    const ContractEnvironment &getEnv();
    const std::vector<uint8_t> buildAdditionalData(ContractOutput &output);

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
    }
    static void setupSyscalls();
private:
    x86ContractVM &contractVM;
    ContractOutput output;
    DeltaDBWrapper &db;
    HypervisorEffect effects;

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
#define QSC_GetEvent                17
#define QSC_GetEventSize            18
#define QSC_ExecutingCallID         19
#define QSC_NextCallID              20


    //storage commands, 0x1000
#define QSC_ReadStorage             0x1000
#define QSC_WriteStorage            0x1001

    //value commands, 0x2000
#define QSC_SendValue               0x2000 //send coins somewhere
#define QSC_GetBalance              0x2001
#define QSC_SendValueAndCall        0x2002

    //caller commands, 0x3000
#define QSC_GasProvided             0x3000
#define QSC_CallerTransaction       0x3001 //provides output scripts, etc
#define QSC_ValueProvided           0x3002
#define QSC_OriginAddress           0x3003
#define QSC_SenderAddress           0x3004
#define QSC_CallStackSize           0x3005

#define QSC_SCCSCount               0x3006
#define QSC_SCCSMaxItems            0x3007
#define QSC_SCCSMaxSize             0x3008
#define QSC_SCCSSize                0x3009
#define QSC_SCCSItemSize            0x300A
#define QSC_SCCSPop                 0x300B
#define QSC_SCCSPeek                0x300C
#define QSC_SCCSPush                0x300D
#define QSC_SCCSClear               0x300E

    //call commands, 0x4000
#define QSC_CallContract            0x4000
#define QSC_CallLibrary             0x4001
//pushes data onto invisible arg stack
#define QSC_PushCallArg             0x4002
//pops data off of the invisible arg stack
#define QSC_PopCallArg              0x4003
//clears all data from the arg stack, potentially issuing a gas refund
#define QSC_ClearCallArgs           0x4004


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
    QtumSystem = 0x40,
    QtumTrustedLibrary = 0x41,
    InteralUI = 0x50
};


#endif
