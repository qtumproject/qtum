#ifndef QTUMX86_H
#define QTUMX86_H

#include "qtumstate.h"
#include "qtumtransaction.h"

#include <x86lib.h>

class ContractVM;

class x86ContractVM : public ContractVM{
public:
    x86ContractVM(DeltaDB &db, const ContractEnvironment &env, uint64_t remainingGasLimit)
            : ContractVM(db, env, remainingGasLimit)
    {}
    virtual bool execute(ContractOutput &output, ContractExecutionResult &result, bool commit);
private:
    const ContractEnvironment &getEnv();

    friend class QtumHypervisor;
};

class QtumHypervisor : public x86Lib::InterruptHypervisor{
    QtumHypervisor(x86ContractVM &vm) : contractVM(vm){
    }
    virtual void HandleInt(int number, x86Lib::x86CPU &vm);
private:
    x86ContractVM &contractVM;


    friend x86ContractVM;
};

static const int QTUM_SYSTEM_ERROR_INT = 0xFF;

//interrupt 0x40
enum QtumSystemCall{
    PreviousBlockTime = 0,
    BlockGasLimit = 1,
    BlockCreator = 2,
    BlockDifficulty = 3,
    BlockHeight = 4,
    GetBlockHash = 5
};

enum QtumEndpoint{
    QtumSystem = 0x40,
    QtumTrustedLibrary = 0x41,
    InteralUI = 0x50
};


#endif