#include "qtumstate.h"

std::pair<dev::eth::ExecutionResult, dev::eth::TransactionReceipt> QtumState::execute(dev::eth::EnvInfo const& _envInfo, dev::eth::SealEngineFace const& _sealEngine, dev::eth::Transaction const& _t, dev::eth::Permanence _p, OnOpFunc const& _onOp){
    std::pair<dev::eth::ExecutionResult, dev::eth::TransactionReceipt> res = State::execute(_envInfo, _sealEngine, _t, _p, _onOp);
    
    dev::eth::Account* acSender = const_cast<dev::eth::Account*>(account(_t.sender()));
    dev::eth::Account* acAuthor = const_cast<dev::eth::Account*>(account(_envInfo.author()));
    acSender->kill();
    acAuthor->kill();
    
    commit(CommitBehaviour::RemoveEmptyAccounts);
    db().commit();
    return res;
}
