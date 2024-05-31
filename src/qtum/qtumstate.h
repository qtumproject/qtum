#pragma once

#include <libdevcore/UndefMacros.h>
#include <libethereum/State.h>
#include <libevm/ExtVMFace.h>
#include <crypto/sha256.h>
#include <crypto/ripemd160.h>
#include <uint256.h>
#include <util/convert.h>
#include <primitives/transaction.h>
#include <qtum/qtumtransaction.h>

#include <libethereum/Executive.h>
#include <libethcore/SealEngine.h>

class CChain;

using OnOpFunc = std::function<void(uint64_t, uint64_t, dev::eth::Instruction, dev::bigint, dev::bigint, 
    dev::bigint, dev::eth::VMFace const*, dev::eth::ExtVMFace const*)>;
using plusAndMinus = std::pair<dev::u256, dev::u256>;
using valtype = std::vector<unsigned char>;

struct TransferInfo{
    dev::Address from;
    dev::Address to;
    dev::u256 value;
};

struct Vin{
    dev::h256 hash;
    uint32_t nVout;
    dev::u256 value;
    uint8_t alive;
};

class QtumTransactionReceipt: public dev::eth::TransactionReceipt {
public:
    QtumTransactionReceipt(dev::h256 const& state_root, dev::h256 const& utxo_root, dev::u256 const& gas_used, dev::eth::LogEntries const& log) : dev::eth::TransactionReceipt(state_root, gas_used, log), m_utxoRoot(utxo_root) {}

    dev::h256 const& utxoRoot() const {
        return m_utxoRoot;
    }
private:
    dev::h256 m_utxoRoot;
};

struct ResultExecute{
    dev::eth::ExecutionResult execRes;
    QtumTransactionReceipt txRec;
    CTransaction tx;
};

namespace qtum{
    template <class DB>
    dev::AddressHash commit(std::unordered_map<dev::Address, Vin> const& _cache, dev::eth::SecureTrieDB<dev::Address, DB>& _state, std::unordered_map<dev::Address, dev::eth::Account> const& _cacheAcc)
    {
        dev::AddressHash ret;
        for (auto const& i: _cache){
            if(i.second.alive == 0){
                 _state.remove(i.first);
            } else {
                dev::RLPStream s(4);
                s << i.second.hash << i.second.nVout << i.second.value << i.second.alive;
                _state.insert(i.first, &s.out());
            }
            ret.insert(i.first);
        }
        return ret;
    }
}

class CondensingTX;

class QtumState : public dev::eth::State {
    
public:

    QtumState();

    QtumState(dev::u256 const& _accountStartNonce, dev::OverlayDB const& _db, const std::string& _path, dev::eth::BaseState _bs = dev::eth::BaseState::PreExisting);

    ResultExecute execute(dev::eth::EnvInfo const& _envInfo, dev::eth::SealEngineFace const& _sealEngine, QtumTransaction const& _t, CChain& _chain, dev::eth::Permanence _p = dev::eth::Permanence::Committed, dev::eth::OnOpFunc const& _onOp = OnOpFunc());

    void setRootUTXO(dev::h256 const& _r) { cacheUTXO.clear(); stateUTXO.setRoot(_r); }

    void setCacheUTXO(dev::Address const& address, Vin const& vin) { cacheUTXO.insert(std::make_pair(address, vin)); }

    dev::h256 rootHashUTXO() const { return stateUTXO.root(); }

    std::unordered_map<dev::Address, Vin> vins() const; // temp

    dev::OverlayDB const& dbUtxo() const { return dbUTXO; }

    dev::OverlayDB& dbUtxo() { return dbUTXO; }

    static const dev::Address createQtumAddress(dev::h256 hashTx, uint32_t voutNumber){
        uint256 hashTXid(h256Touint(hashTx));
        std::vector<unsigned char> txIdAndVout(hashTXid.begin(), hashTXid.end());
        std::vector<unsigned char> voutNumberChrs;
        if (voutNumberChrs.size() < sizeof(voutNumber))voutNumberChrs.resize(sizeof(voutNumber));
        std::memcpy(voutNumberChrs.data(), &voutNumber, sizeof(voutNumber));
        txIdAndVout.insert(txIdAndVout.end(),voutNumberChrs.begin(),voutNumberChrs.end());

        std::vector<unsigned char> SHA256TxVout(32);
        CSHA256().Write(txIdAndVout.data(), txIdAndVout.size()).Finalize(SHA256TxVout.data());

        std::vector<unsigned char> hashTxIdAndVout(20);
        CRIPEMD160().Write(SHA256TxVout.data(), SHA256TxVout.size()).Finalize(hashTxIdAndVout.data());

        return dev::Address(hashTxIdAndVout);
    }

    void deployDelegationsContract();

    virtual ~QtumState(){}

    friend CondensingTX;

private:

    void transferBalance(dev::Address const& _from, dev::Address const& _to, dev::u256 const& _value) override;

    Vin const* vin(dev::Address const& _a) const;

    Vin* vin(dev::Address const& _addr);

    // void commit(CommitBehaviour _commitBehaviour);

    void kill(dev::Address _addr) override;

    void addBalance(dev::Address const& _id, dev::u256 const& _amount) override;

    void deleteAccounts(std::set<dev::Address>& addrs);

    void updateUTXO(const std::unordered_map<dev::Address, Vin>& vins);

    void printfErrorLog(const dev::eth::TransactionException er);

    dev::Address newAddress;

    std::vector<TransferInfo> transfers;

    dev::OverlayDB dbUTXO;

	dev::eth::SecureTrieDB<dev::Address, dev::OverlayDB> stateUTXO;

	std::unordered_map<dev::Address, Vin> cacheUTXO;

	void validateTransfersWithChangeLog();
};


struct TemporaryState{
    std::unique_ptr<QtumState>& globalStateRef;
    dev::h256 oldHashStateRoot;
    dev::h256 oldHashUTXORoot;

    TemporaryState(std::unique_ptr<QtumState>& _globalStateRef) : 
        globalStateRef(_globalStateRef),
        oldHashStateRoot(globalStateRef->rootHash()), 
        oldHashUTXORoot(globalStateRef->rootHashUTXO()) {}
                
    void SetRoot(dev::h256 newHashStateRoot, dev::h256 newHashUTXORoot)
    {
        globalStateRef->setRoot(newHashStateRoot);
        globalStateRef->setRootUTXO(newHashUTXORoot);
    }

    ~TemporaryState(){
        globalStateRef->setRoot(oldHashStateRoot);
        globalStateRef->setRootUTXO(oldHashUTXORoot);
    }
    TemporaryState() = delete;
    TemporaryState(const TemporaryState&) = delete;
    TemporaryState& operator=(const TemporaryState&) = delete;
    TemporaryState(TemporaryState&&) = delete;
    TemporaryState& operator=(TemporaryState&&) = delete;
};


///////////////////////////////////////////////////////////////////////////////////////////
class CondensingTX{

public:

    CondensingTX(QtumState* _state, const std::vector<TransferInfo>& _transfers, const QtumTransaction& _transaction, std::set<dev::Address> _deleteAddresses = std::set<dev::Address>()) : transfers(_transfers), deleteAddresses(_deleteAddresses), transaction(_transaction), state(_state){}

    CTransaction createCondensingTX();

    std::unordered_map<dev::Address, Vin> createVin(const CTransaction& tx);

    bool reachedVoutLimit(){ return voutOverflow; }

private:

    void selectionVin();

    void calculatePlusAndMinus();

    bool createNewBalances();

    std::vector<CTxIn> createVins();

    std::vector<CTxOut> createVout();

    bool checkDeleteAddress(dev::Address addr);

    std::map<dev::Address, plusAndMinus> plusMinusInfo;

    std::map<dev::Address, dev::u256> balances;

    std::map<dev::Address, uint32_t> nVouts;

    std::map<dev::Address, Vin> vins;

    const std::vector<TransferInfo>& transfers;

    //We don't need the ordered nature of "set" here, but unordered_set's theoretical worst complexity is O(n), whereas set is O(log n)
    //So, making this unordered_set could be an attack vector
    const std::set<dev::Address> deleteAddresses;

    const QtumTransaction& transaction;

    QtumState* state;

    bool voutOverflow = false;

};
///////////////////////////////////////////////////////////////////////////////////////////
