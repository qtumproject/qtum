#ifndef QTUMX86_H
#define QTUMX86_H

#include "qtumtransaction.h"
#include "qtumstate.h"

#include <x86lib.h>

class ContractVM;

class x86ContractVM : public ContractVM{
public:
    x86ContractVM(DeltaDB &db, const ContractEnvironment &env, uint64_t remainingGasLimit)
            : ContractVM(db, env, remainingGasLimit)
    {}
    virtual bool execute(ContractOutput &output, ContractExecutionResult &result, bool commit);
};

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