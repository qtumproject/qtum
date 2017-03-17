#pragma once

#include <libethereum/State.h>
#include <libevm/ExtVMFace.h>

using OnOpFunc = std::function<void(uint64_t, uint64_t, dev::eth::Instruction, dev::bigint, dev::bigint, dev::bigint, dev::eth::VM*, dev::eth::ExtVMFace const*)>;

class QtumState : public dev::eth::State {
    
public:

    QtumState() : State(dev::Invalid256, dev::OverlayDB(), dev::eth::BaseState::PreExisting) {}

    QtumState(dev::u256 const& _accountStartNonce, dev::OverlayDB const& _db, dev::eth::BaseState _bs = dev::eth::BaseState::PreExisting) :
        dev::eth::State(_accountStartNonce, _db, _bs) {}

    std::pair<dev::eth::ExecutionResult, dev::eth::TransactionReceipt> execute(dev::eth::EnvInfo const& _envInfo, dev::eth::SealEngineFace const& _sealEngine, dev::eth::Transaction const& _t, dev::eth::Permanence _p, OnOpFunc const& _onOp){
        std::pair<dev::eth::ExecutionResult, dev::eth::TransactionReceipt> res = State::execute(_envInfo, _sealEngine, _t, _p, _onOp);
        dev::eth::Account* acSender = const_cast<dev::eth::Account*>(account(_t.sender()));
        dev::eth::Account* acAuthor = const_cast<dev::eth::Account*>(account(_envInfo.author()));
        acSender->kill();
        acAuthor->kill();
        commit(CommitBehaviour::RemoveEmptyAccounts);
        db().commit();
        return res;
    }

private:

};
