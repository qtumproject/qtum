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

struct HypervisorEffect{
    int exitCode = 0;
    uint64_t gasUsed = 0;
};

class QtumHypervisor : public x86Lib::InterruptHypervisor{
    QtumHypervisor(x86ContractVM &vm, const ContractOutput& out, DeltaDBWrapper& db_) : contractVM(vm), output(out), db(db_){
        clearEffects();
    }
    virtual void HandleInt(int number, x86Lib::x86CPU &vm);
    HypervisorEffect getEffects(){
        return effects;
    }
    void clearEffects(){
        effects.exitCode = 0;
    }
private:
    x86ContractVM &contractVM;
    ContractOutput output;
    DeltaDBWrapper &db;
    HypervisorEffect effects;

    friend x86ContractVM;
};


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

    //call commands, 0x4000
#define QSC_CallContract            0x4000
#define QSC_CallLibrary             0x4001


enum QtumEndpoint{
    QtumSystem = 0x40,
    QtumTrustedLibrary = 0x41,
    InteralUI = 0x50
};


#endif
