#include <sstream>
#include <util.h>
#include <validation.h>
#include <tinyformat.h>
#include "chainparams.h"
#include "qtumstate.h"

using namespace std;
using namespace dev;
using namespace dev::eth;

QtumState::QtumState(u256 const& _accountStartNonce, OverlayDB const& _db, const string& _path, BaseState _bs) :
        State(_accountStartNonce, _db, _bs) {
            dbUTXO = QtumState::openDB(_path + "/qtumDB", sha3(rlp("")), WithExisting::Trust);
	        stateUTXO = SecureTrieDB<Address, OverlayDB>(&dbUTXO);
}

QtumState::QtumState() : dev::eth::State(dev::Invalid256, dev::OverlayDB(), dev::eth::BaseState::PreExisting) {
    dbUTXO = OverlayDB();
    stateUTXO = SecureTrieDB<Address, OverlayDB>(&dbUTXO);
}

ResultExecute QtumState::execute(EnvInfo const& _envInfo, SealEngineFace const& _sealEngine, QtumTransaction const& _t, Permanence _p, OnOpFunc const& _onOp){

    assert(_t.getVersion().toRaw() == VersionVM::GetEVMDefault().toRaw());

    addBalance(_t.sender(), _t.value() + (_t.gas() * _t.gasPrice()));
    newAddress = _t.isCreation() ? createQtumAddress(_t.getHashWith(), _t.getNVout()) : dev::Address();

    _sealEngine.deleteAddresses.insert({_t.sender(), _envInfo.author()});

    h256 oldStateRoot = rootHash();
    bool voutLimit = false;

	auto onOp = _onOp;
#if ETH_VMTRACE
	if (isChannelVisible<VMTraceChannel>())
		onOp = Executive::simpleTrace(); // override tracer
#endif
	// Create and initialize the executive. This will throw fairly cheaply and quickly if the
	// transaction is bad in any way.
	Executive e(*this, _envInfo, _sealEngine);
	ExecutionResult res;
	e.setResultRecipient(res);

    CTransactionRef tx;
    u256 startGasUsed;
    try{
        if (_t.isCreation() && _t.value())
            BOOST_THROW_EXCEPTION(CreateWithValue());

        e.initialize(_t);
        // OK - transaction looks valid - execute.
        startGasUsed = _envInfo.gasUsed();
        if (!e.execute()){
            e.go(onOp);
        } else {

            e.revert();
            throw Exception();
        }
        e.finalize();
        if (_p == Permanence::Reverted){
            m_cache.clear();
            cacheUTXO.clear();
        } else {
            deleteAccounts(_sealEngine.deleteAddresses);
            if(res.excepted == TransactionException::None){
                //CondensingTX ctx(this, transfers, _t, _sealEngine.deleteAddresses);
                AccountTransfer senderTransfer;
                senderTransfer.value = (uint64_t) _t.value();
                senderTransfer.from.version = AddressVersion::PUBKEYHASH;
                senderTransfer.from.data = _t.sender().asBytes();
                senderTransfer.fromVin.value = (uint64_t)_t.value();
                senderTransfer.fromVin.alive = true;
                senderTransfer.fromVin.nVout = _t.getNVout();
                senderTransfer.fromVin.txid = h256Touint(_t.getHashWith());
                senderTransfer.to.version = AddressVersion::EVM;
                senderTransfer.to.data = _t.receiveAddress().asBytes();
                if(_t.value() != 0) {

                    Vin const *v = vin(_t.receiveAddress());
                    if (v != NULL) {
                        senderTransfer.toVin = AccountVin::fromVin(*v);
                    }
                    //transfers.insert(transfers.begin(), senderTransfer);
                    for(auto& t : transfers){
                        if(t.from == senderTransfer.from){
                            t.fromVin = senderTransfer.fromVin;
                        }
                    }
                }
                AccountAbstractionLayer aal(transfers, senderTransfer);
                bool reachedVoutLimit = false;
                tx = MakeTransactionRef(aal.createCondensingTx(reachedVoutLimit));
                if(reachedVoutLimit){
                    voutLimit = true;
                    e.revert();
                    throw Exception();
                }
                if(!tx.get()->IsNull()) {
                    //no condensing transaction to update
                    std::unordered_map<dev::Address, Vin> vins;
                    for (auto &kv : aal.spentVins()) {
                        if(kv.first == senderTransfer.from){
                            continue; //don't add the pubkey owned vin to this
                        }
                        //first, update all of the spent vins
                        vins[dev::Address(kv.first.data)] = kv.second.toVin();
                        vins[dev::Address(kv.first.data)].value = 0; //spent
                        vins[dev::Address(kv.first.data)].alive = false; //spent
                    }
                    //now update all of the accounts that have new vins
                    for (auto &kv : aal.getNewVoutNumbers()) {
                        if(kv.first == senderTransfer.from){
                            continue; //don't add the pubkey owned vin to this
                        }
                        Vin v;
                        v.alive = true;
                        v.hash = uintToh256(tx.get()->GetHash());
                        v.nVout = kv.second;
                        v.value = tx.get()->vout[v.nVout].nValue;
                        vins[dev::Address(kv.first.data)] = v;
                    }

                    updateUTXO(vins); //write vins to database
                }else if(senderTransfer.value > 0){
                    //still need to update that the executed contract has a new vin
                    std::unordered_map<dev::Address, Vin> vins;
                    Vin v;
                    v.alive = true;
                    v.hash = uintToh256(senderTransfer.fromVin.txid);
                    v.nVout = senderTransfer.fromVin.nVout;
                    v.value = senderTransfer.value;
                    vins[dev::Address(senderTransfer.to.data)] = v;
                    updateUTXO(vins);
                }
            } else {
                printfErrorLog(res.excepted);
            }
            
            qtum::commit(cacheUTXO, stateUTXO, m_cache);
            cacheUTXO.clear();
            bool removeEmptyAccounts = _envInfo.number() >= _sealEngine.chainParams().u256Param("EIP158ForkBlock");
            commit(removeEmptyAccounts ? State::CommitBehaviour::RemoveEmptyAccounts : State::CommitBehaviour::KeepEmptyAccounts);
        }
    }
    catch(Exception const& _e){

        printfErrorLog(dev::eth::toTransactionException(_e));
        res.excepted = dev::eth::toTransactionException(_e);
        res.gasUsed = _t.gas();
        const Consensus::Params& consensusParams = Params().GetConsensus();
        if(chainActive.Height() < consensusParams.nFixUTXOCacheHFHeight  && _p != Permanence::Reverted){
            deleteAccounts(_sealEngine.deleteAddresses);
            commit(CommitBehaviour::RemoveEmptyAccounts);
        } else {
            m_cache.clear();
            cacheUTXO.clear();
        }
    }

    if(!_t.isCreation())
        res.newAddress = _t.receiveAddress();
    newAddress = dev::Address();
    transfers.clear();
    if(voutLimit){
        //use old and empty states to create virtual Out Of Gas exception
        LogEntries logs;
        u256 gas = _t.gas();
        ExecutionResult ex;
        ex.gasRefunded=0;
        ex.gasUsed=gas;
        ex.excepted=TransactionException();
        //create a refund tx to send back any coins that were suppose to be sent to the contract
        CMutableTransaction refund;
        if(_t.value() > 0) {
            refund.vin.push_back(CTxIn(h256Touint(_t.getHashWith()), _t.getNVout(), CScript() << OP_SPEND));
            //note, if sender was a non-standard tx, this will send the coins to pubkeyhash 0x00, effectively destroying the coins
            CScript script(CScript() << OP_DUP << OP_HASH160 << _t.sender().asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
            refund.vout.push_back(CTxOut(CAmount(_t.value().convert_to<uint64_t>()), script));
        }
        //make sure to use empty transaction if no vouts made
        return ResultExecute{ex, dev::eth::TransactionReceipt(oldStateRoot, gas, e.logs()), refund.vout.empty() ? CTransaction() : CTransaction(refund)};
    }else{
        return ResultExecute{res, dev::eth::TransactionReceipt(rootHash(), startGasUsed + e.gasUsed(), e.logs()), tx.get() ? *tx.get() : CTransaction()};
    }
}

std::unordered_map<dev::Address, Vin> QtumState::vins() const // temp
{
    std::unordered_map<dev::Address, Vin> ret;
    for (auto& i: cacheUTXO)
        if (i.second.alive)
            ret[i.first] = i.second;
    auto addrs = addresses();
    for (auto& i : addrs){
        if (cacheUTXO.find(i.first) == cacheUTXO.end() && vin(i.first))
            ret[i.first] = *vin(i.first);
    }
    return ret;
}

bool QtumState::addressIsPubKeyHash(dev::Address const &a) {
    //pubkeyhash inserted addresses will not have any code
    return globalSealEngine.get()->deleteAddresses.count(a); // && !addressInUse(a);
}

void QtumState::transferBalance(dev::Address const& _from, dev::Address const& _to, dev::u256 const& _value) {
    subBalance(_from, _value);
    addBalance(_to, _value);
    if (_value > 0) {
        AccountTransfer t;
        if(addressIsPubKeyHash(_from)) {
            //don't add a transfer for transfers from pubkeyhash
            //this is only possible in the initial sender transaction, and we handle that elsewhere
            t.from.version = AddressVersion::PUBKEYHASH;
            //return; //don't add a transfer for this.
        }else{
            t.from.version = AddressVersion::EVM;
        }
        t.from.data = _from.asBytes();
        if(addressIsPubKeyHash(_to)) {
            t.to.version = AddressVersion::PUBKEYHASH;
        }else{
            t.to.version = AddressVersion::EVM;
        }
        t.to.data = _to.asBytes();
        Vin* toVin = vin(_to);
        if(toVin != NULL){
            t.toVin = AccountVin::fromVin(*toVin);
        }
        Vin* fromVin = vin(_from);
        if(fromVin != NULL){
            t.fromVin = AccountVin::fromVin(*fromVin);
        }else{
            LogPrintf("Transfer from account with no vin!");
        }
        //overflow check?
        t.value = (uint64_t) _value;
        transfers.push_back(t);
    }
}

Vin const* QtumState::vin(dev::Address const& _a) const
{
    return const_cast<QtumState*>(this)->vin(_a);
}

Vin* QtumState::vin(dev::Address const& _addr)
{
    auto it = cacheUTXO.find(_addr);
    if (it == cacheUTXO.end()){
        std::string stateBack = stateUTXO.at(_addr);
        if (stateBack.empty())
            return nullptr;
            
        dev::RLP state(stateBack);
        auto i = cacheUTXO.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(_addr),
            std::forward_as_tuple(Vin{state[0].toHash<dev::h256>(), state[1].toInt<uint32_t>(), state[2].toInt<dev::u256>(), state[3].toInt<uint8_t>()})
        );
        return &i.first->second;
    }
    return &it->second;
}

// void QtumState::commit(CommitBehaviour _commitBehaviour)
// {
//     if (_commitBehaviour == CommitBehaviour::RemoveEmptyAccounts)
//         removeEmptyAccounts();

//     qtum::commit(cacheUTXO, stateUTXO, m_cache);
//     cacheUTXO.clear();
        
//     m_touched += dev::eth::commit(m_cache, m_state);
//     m_changeLog.clear();
//     m_cache.clear();
//     m_unchangedCacheEntries.clear();
// }

void QtumState::kill(dev::Address _addr)
{
    // If the account is not in the db, nothing to kill.
    if (auto a = account(_addr))
        a->kill();
    if (auto v = vin(_addr))
        v->alive = 0;
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
            newAddress = dev::Address();
        }
        createAccount(_id, {requireAccountStartNonce(), _amount});
    }

    if (_amount)
        m_changeLog.emplace_back(dev::eth::detail::Change::Balance, _id, _amount);
}

dev::Address QtumState::createQtumAddress(dev::h256 hashTx, uint32_t voutNumber){
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

void QtumState::deleteAccounts(std::set<dev::Address>& addrs){
    for(dev::Address addr : addrs){
        dev::eth::Account* acc = const_cast<dev::eth::Account*>(account(addr));
        if(acc)
            acc->kill();
        Vin* in = const_cast<Vin*>(vin(addr));
        if(in)
            in->alive = 0;
    }
}

void QtumState::updateUTXO(const std::unordered_map<dev::Address, Vin>& vins){
    for(auto& v : vins){
        Vin* vi = const_cast<Vin*>(vin(v.first));

        if(vi){
            vi->hash = v.second.hash;
            vi->nVout = v.second.nVout;
            vi->value = v.second.value;
            vi->alive = v.second.alive;
        } else if(v.second.alive > 0) {
            cacheUTXO[v.first] = v.second;
        }
    }
}

void QtumState::printfErrorLog(const dev::eth::TransactionException er){
    std::stringstream ss;
    ss << er;
    clog(ExecutiveWarnChannel) << "VM exception:" << ss.str();
}

///////////////////////////////////////////////////////////////////////////////////////////
CTransaction CondensingTX::createCondensingTX(){
    selectionVin();
    calculatePlusAndMinus();
    if(!createNewBalances())
        return CTransaction();
    CMutableTransaction tx;
    tx.vin = createVins();;
    tx.vout = createVout();
    return !tx.vin.size() || !tx.vout.size() ? CTransaction() : CTransaction(tx);
}

std::unordered_map<dev::Address, Vin> CondensingTX::createVin(const CTransaction& tx){
    std::unordered_map<dev::Address, Vin> vins;
    for(auto& b : balances){
        if(b.first == transaction.sender())
            continue;

        if(b.second > 0){
            vins[b.first] = Vin{uintToh256(tx.GetHash()), nVouts[b.first], b.second, 1};
        } else {
            vins[b.first] = Vin{uintToh256(tx.GetHash()), 0, 0, 0};
        }
    }
    return vins;
}

void CondensingTX::selectionVin(){
    for(const TransferInfo& ti : transfers){
        if(!vins.count(ti.from)){
            if(auto a = state->vin(ti.from))
                vins[ti.from] = *a;
            if(ti.from == transaction.sender() && transaction.value() > 0){
                vins[ti.from] = Vin{transaction.getHashWith(), transaction.getNVout(), transaction.value(), 1};
            }
        }

        if(!vins.count(ti.to)){
            if(auto a = state->vin(ti.to))
                vins[ti.to] = *a;
        }
    }
}

void CondensingTX::calculatePlusAndMinus(){
    for(const TransferInfo& ti : transfers){
        if(!plusMinusInfo.count(ti.from)){
            plusMinusInfo[ti.from] = std::make_pair(0, ti.value);
        } else {
            plusMinusInfo[ti.from] = std::make_pair(plusMinusInfo[ti.from].first, plusMinusInfo[ti.from].second + ti.value);
        }

        if(!plusMinusInfo.count(ti.to)){
            plusMinusInfo[ti.to] = std::make_pair(ti.value, 0);
        } else {
            plusMinusInfo[ti.to] = std::make_pair(plusMinusInfo[ti.to].first + ti.value, plusMinusInfo[ti.to].second);
        }
    }
}

bool CondensingTX::createNewBalances(){
    for(auto& p : plusMinusInfo){
        dev::u256 balance = 0;
        //!alive & checkDelete means that this address is for a pubkeyhash, and so don't track it's balance
        if((vins.count(p.first) && vins[p.first].alive) || (!vins[p.first].alive && !checkDeleteAddress(p.first))){
            balance = vins[p.first].value;
        }
        balance += p.second.first;
        if(balance < p.second.second)
            return false;
        balance -= p.second.second;
        balances[p.first] = balance;
    }
    return true;
}

std::vector<CTxIn> CondensingTX::createVins(){
    std::vector<CTxIn> ins;
    for(auto& v : vins){
        //!alive & checkDelete means that this address is for a pubkeyhash, and so don't track it's balance
        if((v.second.value > 0 && v.second.alive) || (v.second.value > 0 && !vins[v.first].alive && !checkDeleteAddress(v.first)))
            ins.push_back(CTxIn(h256Touint(v.second.hash), v.second.nVout, CScript() << OP_SPEND));
    }
    return ins;
}

std::vector<CTxOut> CondensingTX::createVout(){
    size_t count = 0;
    std::vector<CTxOut> outs;
    for(auto& b : balances){
        if(b.second > 0){
            CScript script;
            auto* a = state->account(b.first);
            if(a && a->isAlive()){
                //create a no-exec contract output
                script = CScript() << valtype{0} << valtype{0} << valtype{0} << valtype{0} << b.first.asBytes() << OP_CALL;
            } else {
                script = CScript() << OP_DUP << OP_HASH160 << b.first.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG;
            }
            outs.push_back(CTxOut(CAmount(b.second), script));
            nVouts[b.first] = count;
            count++;
        }
        if(count > MAX_CONTRACT_VOUTS){
            voutOverflow=true;
            return outs;
        }
    }
    return outs;
}

bool CondensingTX::checkDeleteAddress(dev::Address addr){
    return deleteAddresses.count(addr) != 0;
}
///////////////////////////////////////////////////////////////////////////////////////////
static bool int64AddWillOverflow(int64_t a, int64_t b){
    return a > 0 && (b > INT64_MAX - a);
}

bool AccountAbstractionLayer::calculateBalances() {
    //requiredVins contains only 1 record per address. note: sort order is consensus-critical
    for(auto &t : transfers){
        //populate initial balances to greatly simplify this algorithm
        if(!balances.count(t.from)) {
            balances[t.from] = t.fromVin.value;
        }
        if(!balances.count(t.to)) {
            balances[t.to] = t.toVin.value;
        }
    }
    for(auto &t : transfers){
        if(int64AddWillOverflow(t.value, balances[t.to])){
            LogPrintf("Account balance change would cause overflow!");
            return false;
        }
        if(balances[t.from] < t.value){
            LogPrintf("Account balance change would cause underflow!");
            //return false;
        }
        balances[t.to] += t.value;
        balances[t.from] -= t.value;
    }
    for(auto b : balances){
        if(b.second < 0){
            LogPrintf("underflow in final balance of AAL");
            return false;
        }
    }
    return true;
}

void AccountAbstractionLayer::selectVins() {
    for(auto &t : transfers){
        if(!selectedVins.count(t.from)){
            if(t.from == senderTransfer.from && t.value > 0){
                selectedVins[t.from] = senderTransfer.fromVin;
            }else {
                if(t.fromVin.txid.IsNull()){
                    LogPrintf("no txid to spend from!");
                }
                selectedVins[t.from] = t.fromVin;
            }
        }
        //if an account has a vin, then spend it to condense it into 1 output
        if(!t.toVin.txid.IsNull() && !selectedVins.count(t.to)){
            selectedVins[t.to] = t.toVin;
        }
    }
}

CTransaction AccountAbstractionLayer::createCondensingTx(bool &voutsBeyondMax) {

    if(!calculateBalances()){
        return CTransaction();
    }

    //note: vout and vin order are consensus-critical!
    //map isn't particularly necessary, but the sort order is
    selectVins();

    CMutableTransaction tx;
    //generate vins
    //note: the executing output and the previously owned output must be condensed.
    //In order to do this, they are both placed in the vin list.
    //the confusing part is that the sender vin is inserted as "fromAddress" rather than "toAddress"
    //So, it looks like you are spending a pubkeyhash output, but actually it's an OP_CALL
    //this can't be changed or fixed because sort order is consensus-critical
    for(auto &v : selectedVins){
        if(!v.second.alive){
            continue;
        }
       // if(v.first.version != AddressVersion::PUBKEYHASH) { // && v.first != senderTransfer.from){
            tx.vin.push_back(CTxIn(v.second.txid, v.second.nVout, CScript() << OP_SPEND));
       // }
    }
    //generate vouts
    int n=0;
    for(auto &balance : balances){
        if(balance.second == 0){
            continue;
        }
        CScript script;
        if(balance.first.version == AddressVersion::PUBKEYHASH){
            script = CScript() << OP_DUP << OP_HASH160 << balance.first.data << OP_EQUALVERIFY << OP_CHECKSIG;
        } else {
            //create a no-exec contract output
            script = CScript() << valtype{0} << valtype{0} << valtype{0} << valtype{0} << balance.first.data << OP_CALL;

            voutNumbers[balance.first] = n;
        }
        tx.vout.push_back(CTxOut(balance.second, script));
        if(tx.vout.size() > MAX_CONTRACT_VOUTS){
            voutsBeyondMax=true;
            if(!tx.vin.size() && tx.vout.size()>1){
                LogPrintf("AAL Transaction has a vout, but no vins");
                return CTransaction();
            }
            return !tx.vin.size() || !tx.vout.size() ? CTransaction() : CTransaction(tx);
        }
        n++;
    }
    //safety check
    CAmount totalIn=0, totalOut=0;

    for(auto &v : tx.vout){
        totalOut += v.nValue;
    }
    for(auto &v : selectedVins){
        totalIn += v.second.value;
    }
    if(totalOut != totalIn){
        LogPrintf("Mismatch total input and output for contract created transaction!");
    }
    if(!tx.vin.size() && tx.vout.size()>0){
        LogPrintf("AAL Transaction has a vout, but no vins");
        return CTransaction();
    }
    if(!tx.vout.size() && tx.vin.size()>0){
        LogPrintf("AAL Transaction has a vin, but no vouts");
        return CTransaction();
    }
    return !tx.vin.size() || !tx.vout.size() ? CTransaction() : CTransaction(tx);
}










