#pragma once

#include <libethereum/State.h>
#include <libevm/ExtVMFace.h>
#include <crypto/sha256.h>
#include <crypto/ripemd160.h>
#include <uint256.h>
#include <qtum/qtumtransaction.h>

using OnOpFunc = std::function<void(uint64_t, uint64_t, dev::eth::Instruction, dev::bigint, dev::bigint, 
    dev::bigint, dev::eth::VM*, dev::eth::ExtVMFace const*)>;
using execResult = std::pair<dev::eth::ExecutionResult, dev::eth::TransactionReceipt>;


class QtumState : public dev::eth::State {
    
public:

    QtumState() : dev::eth::State(dev::Invalid256, dev::OverlayDB(), dev::eth::BaseState::PreExisting) {}

    QtumState(dev::u256 const& _accountStartNonce, dev::OverlayDB const& _db, dev::eth::BaseState _bs = dev::eth::BaseState::PreExisting) :
        dev::eth::State(_accountStartNonce, _db, _bs) {}

    execResult execute(dev::eth::EnvInfo const& _envInfo, 
        dev::eth::SealEngineFace const& _sealEngine, QtumTransaction const& _t, dev::eth::Permanence _p, OnOpFunc const& _onOp);

private:

    void addBalance(dev::Address const& _id, dev::u256 const& _amount);

    dev::Address createQtumAddress(dev::h256 hashTx, uint32_t voutNumber);

    dev::Address newAddress;
};
