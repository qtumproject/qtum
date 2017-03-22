#include "qtumstate.h"

execResult QtumState::execute(dev::eth::EnvInfo const& _envInfo, dev::eth::SealEngineFace const& _sealEngine, QtumTransaction const& _t, dev::eth::Permanence _p, OnOpFunc const& _onOp){
    addBalance(_t.sender(), _t.value() + (_t.gas() * _t.gasPrice())); // temp
    newAddress = _t.isCreation() ? createQtumAddress(_t.getHashWith(), _t.getNVout()) : dev::Address();
    execResult res = State::execute(_envInfo, _sealEngine, _t, _p, _onOp);
    
    dev::eth::Account* acSender = const_cast<dev::eth::Account*>(account(_t.sender()));
    dev::eth::Account* acAuthor = const_cast<dev::eth::Account*>(account(_envInfo.author()));
    acSender->kill();
    acAuthor->kill();
    
    commit(CommitBehaviour::RemoveEmptyAccounts);
    db().commit();

    newAddress = dev::Address();
    return res;
}

void QtumState::addBalance(dev::Address const& _id, dev::u256 const& _amount)
{
    if (dev::eth::Account* a = account(_id))
    {
            // Log empty account being touched. Empty touched accounts are cleared
            // after the transaction, so this event must be also reverted.
            // We only log the first touch (not dirty yet), and only for empty
            // accounts, as other accounts does not matter.
            // TODO: to save space we can combine this event with Balance by having
            //       Balance and Balance+Touch events.
        if (!a->isDirty() && a->isEmpty())
            m_changeLog.emplace_back(dev::eth::detail::Change::Touch, _id);

            // Increase the account balance. This also is done for value 0 to mark
            // the account as dirty. Dirty account are not removed from the cache
            // and are cleared if empty at the end of the transaction.
        a->addBalance(_amount);
    }
    else
    {
        if(!addressInUse(newAddress) && newAddress != dev::Address()){
            const_cast<dev::Address&>(_id) = newAddress;
        }
        createAccount(_id, {requireAccountStartNonce(), _amount});
    }

    if (_amount)
        m_changeLog.emplace_back(dev::eth::detail::Change::Balance, _id, _amount);
}

dev::Address QtumState::createQtumAddress(dev::h256 hashTx, uint32_t voutNumber){
    uint256 hashTXid(h256Touint(hashTx));
	std::vector<unsigned char> txIdAndVout(hashTXid.begin(), hashTXid.end());
	txIdAndVout.push_back(voutNumber);
		
	std::vector<unsigned char> SHA256TxVout(32);
    CSHA256().Write(txIdAndVout.data(), txIdAndVout.size()).Finalize(SHA256TxVout.data());

	std::vector<unsigned char> hashTxIdAndVout(20);
    CRIPEMD160().Write(SHA256TxVout.data(), SHA256TxVout.size()).Finalize(hashTxIdAndVout.data());
		
	return dev::Address(hashTxIdAndVout);
}
