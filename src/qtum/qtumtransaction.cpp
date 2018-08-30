
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <util.h>
#include <tinyformat.h>
#include <script/standard.h>
#include <chainparams.h>
#include <validation.h>
#include <pubkey.h>
#include "qtumtransaction.h"
#include <qtum/qtumx86.h>
#include <vector>
#include <streams.h>
#include <serialize.h>
#include <uint256.h>
#include <string>


bool ContractOutputParser::parseOutput(ContractOutput& output){
    output.sender = getSenderAddress();
    try{
        if(!receiveStack(tx.vout[nvout].scriptPubKey)){
            return false;
        }
        std::vector<uint8_t> receiveAddress;
        valtype vecAddr;
        if (opcode == OP_CALL)
        {
            vecAddr = stack.back();
            stack.pop_back();
            receiveAddress = vecAddr;
            output.OpCreate = false;
        }else{
            output.OpCreate = true;
            //must compute address
            std::vector<unsigned char> SHA256TxVout(32);
            std::vector<unsigned char> contractAddress(20);
            auto tmp = tx.GetHash();
            std::vector<unsigned char> txIdAndVout(tmp.begin(), tmp.end());
            uint32_t voutNumber=nvout;
            std::vector<unsigned char> voutNumberChrs;
            if (voutNumberChrs.size() < sizeof(voutNumber))voutNumberChrs.resize(sizeof(voutNumber));
            std::memcpy(voutNumberChrs.data(), &voutNumber, sizeof(voutNumber));
            txIdAndVout.insert(txIdAndVout.end(),voutNumberChrs.begin(),voutNumberChrs.end());
            //address is ripemd160(sha256(txid + vout))
            CSHA256().Write(txIdAndVout.data(), txIdAndVout.size()).Finalize(SHA256TxVout.data());
            CRIPEMD160().Write(SHA256TxVout.data(), SHA256TxVout.size()).Finalize(contractAddress.data());

            receiveAddress = contractAddress;
        }

        if(stack.size() < 4)
            return false;

        if(stack.back().size() < 1){
            return false;
        }
        valtype code(stack.back());
        stack.pop_back();
        uint64_t gasPrice = CScriptNum::vch_to_uint64(stack.back());
        stack.pop_back();
        uint64_t gasLimit = CScriptNum::vch_to_uint64(stack.back());
        stack.pop_back();
        if(gasPrice > INT64_MAX || gasLimit > INT64_MAX){
            return false;
        }
        //we track this as CAmount in some places, which is an int64_t, so constrain to INT64_MAX
        if(gasPrice !=0 && gasLimit > INT64_MAX / gasPrice){
            //overflows past 64bits, reject this tx
            return false;
        }
        if(stack.back().size() > 4){
            return false;
        }
        VersionVM version = VersionVM::fromRaw((uint32_t)CScriptNum::vch_to_uint64(stack.back()));
        stack.pop_back();
        output.version = version;
        output.gasPrice = gasPrice;
        if(version.rootVM == ROOT_VM_EVM) {
            output.address = UniversalAddress(AddressVersion ::EVM, receiveAddress);
        }else if(version.rootVM == ROOT_VM_X86){
            output.address = UniversalAddress(AddressVersion::X86, receiveAddress);
        }else{
            LogPrintf("Invalid contract address!");
            return false;
        }
        output.data = code;
        output.gasLimit = gasLimit;
        output.vout.n = nvout;
        output.vout.hash = tx.GetHash();
        return true;
    }
    catch(const scriptnum_error& err){
        LogPrintf("Incorrect parameters to VM.");
        return false;
    }
}

bool ContractOutputParser::receiveStack(const CScript& scriptPubKey){
    EvalScript(stack, scriptPubKey, SCRIPT_EXEC_BYTE_CODE, BaseSignatureChecker(), SIGVERSION_BASE, nullptr);
    if (stack.empty())
        return false;

    CScript scriptRest(stack.back().begin(), stack.back().end());
    stack.pop_back();

    opcode = (opcodetype)(*scriptRest.begin());
    if((opcode == OP_CREATE && stack.size() < 4) || (opcode == OP_CALL && stack.size() < 5)){
        stack.clear();
        return false;
    }

    return true;
}

UniversalAddress ContractOutputParser::getSenderAddress(){
    if(view == NULL || blockTransactions == NULL){
        return UniversalAddress();
    }
    CScript script;
    bool scriptFilled=false; //can't use script.empty() because an empty script is technically valid

    // First check the current (or in-progress) block for zero-confirmation change spending that won't yet be in txindex
    if(blockTransactions){
        for(auto btx : *blockTransactions){
            if(btx->GetHash() == tx.vin[0].prevout.hash){
                script = btx->vout[tx.vin[0].prevout.n].scriptPubKey;
                scriptFilled=true;
                break;
            }
        }
    }
    if(!scriptFilled && view){
        script = view->AccessCoin(tx.vin[0].prevout).out.scriptPubKey;
        scriptFilled = true;
    }
    if(!scriptFilled)
    {
        CTransactionRef txPrevout;
        uint256 hashBlock;
        if(GetTransaction(tx.vin[0].prevout.hash, txPrevout, Params().GetConsensus(), hashBlock, true)){
            script = txPrevout->vout[tx.vin[0].prevout.n].scriptPubKey;
        } else {
            LogPrintf("Error fetching transaction details of tx %s. This will probably cause more errors", tx.vin[0].prevout.hash.ToString());
            return UniversalAddress();
        }
    }

    CTxDestination addressBit;
    txnouttype txType=TX_NONSTANDARD;
    if(ExtractDestination(script, addressBit, &txType)){
        if ((txType == TX_PUBKEY || txType == TX_PUBKEYHASH) &&
            addressBit.type() == typeid(CKeyID)){
            CKeyID senderAddress(boost::get<CKeyID>(addressBit));
            return UniversalAddress(AddressVersion::PUBKEYHASH, senderAddress.begin(), senderAddress.end());
        }
    }
    //prevout is not a standard transaction format, so just return 0
    return UniversalAddress();
}

ContractEnvironment ContractExecutor::buildEnv() {
    ContractEnvironment env;
    CBlockIndex* tip = chainActive.Tip();
    assert(*tip->phashBlock == block.hashPrevBlock);
    env.blockNumber = tip->nHeight + 1;
    env.blockTime = block.nTime;
    env.difficulty = block.nBits;
    env.gasLimit = blockGasLimit;
    env.blockHashes.resize(256);
    for(int i = 0 ; i < 256; i++){
        if(!tip)
            break;
        env.blockHashes[i] = *tip->phashBlock;
        tip = tip->pprev;
    }

    if(block.IsProofOfStake()){
        env.blockCreator = UniversalAddress::FromScript(block.vtx[1]->vout[1].scriptPubKey);
    }else {
        env.blockCreator = UniversalAddress::FromScript(block.vtx[0]->vout[0].scriptPubKey);
    }
    return env;
}

UniversalAddress UniversalAddress::FromScript(const CScript& script){
    CTxDestination addressBit;
    txnouttype txType=TX_NONSTANDARD;
    if(ExtractDestination(script, addressBit, &txType)){
        if ((txType == TX_PUBKEY || txType == TX_PUBKEYHASH) &&
            addressBit.type() == typeid(CKeyID)){
            CKeyID addressKey(boost::get<CKeyID>(addressBit));
            return UniversalAddress(AddressVersion::PUBKEYHASH, addressKey.begin(), addressKey.end());
        }
    }
    //if not standard or not a pubkey or pubkeyhash output, then return 0
    return UniversalAddress();
}

ContractExecutor::ContractExecutor(const CBlock &_block, ContractOutput _output, uint64_t _blockGasLimit)
: block(_block), output(_output), blockGasLimit(_blockGasLimit)
{

}

bool ContractExecutor::execute(ContractExecutionResult &result, bool commit)
{
    DeltaDBWrapper wrapper(pdeltaDB);
    ContractEnvironment env=buildEnv();
    if(output.version.rootVM == ROOT_VM_EVM){
        EVMContractVM evm(wrapper, env, blockGasLimit);
        evm.execute(output, result, commit);
    }else if(output.version.rootVM == ROOT_VM_X86){
        x86ContractVM x86(wrapper, env, blockGasLimit);
        x86.execute(output, result, commit);
    }else{
        return false;
    }
    if(commit && result.commitState){
        wrapper.commit();
    }
    //no need to revert if not committing
    return true;
}

bool ContractExecutor::buildTransferTx(ContractExecutionResult &res)
{
    return false;
}



bool EVMContractVM::execute(ContractOutput &output, ContractExecutionResult &result, bool commit) {
    dev::eth::EnvInfo envInfo(buildEthEnv());
    if (output.address.version != AddressVersion::UNKNOWN &&
        !globalState->addressInUse(dev::Address(output.address.data))) {
        //contract is not in database
        result.usedGas = output.gasLimit;
        result.refundSender = 0;
        result.status = ContractStatus::DOESNT_EXIST;
        return false;
    }
    dev::eth::Permanence p = commit ? dev::eth::Permanence::Committed : dev::eth::Permanence::Reverted;
    ResultExecute ethres = globalState->execute(envInfo, *globalSealEngine.get(), buildQtumTx(output), p, OnOpFunc());
    //TODO, make proper status
    switch(ethres.execRes.excepted){
        case dev::eth::TransactionException::None:
            result.status = ContractStatus::SUCCESS;
            break;
        default:
            result.status = ContractStatus ::CODE_ERROR;
            break;
    }
    result.refundSender = (uint64_t) ethres.execRes.gasRefunded * output.gasPrice;
    if(result.refundSender > output.gasLimit * output.gasPrice){
        result.refundSender = output.gasLimit * output.gasPrice; //don't ever return more than was sent
    }
    result.usedGas = (uint64_t) ethres.execRes.gasUsed;

    result.transferTx = ethres.tx;
    /*
    for(auto& vout : ethres.tx.vout){
        tx.vout.push_back(vout);
    }
    for(auto& vin : ethres.tx.vin){
        tx.vin.push_back(vin);
    }
    result.transferTx =  tx;
     */
    //result.push_back(globalState->execute(envInfo, *globalSealEngine.get(), tx, type, OnOpFunc()));
    globalState->db().commit();
    globalState->dbUtxo().commit();
    globalSealEngine.get()->deleteAddresses.clear();
    return true;
}

dev::eth::EnvInfo EVMContractVM::buildEthEnv(){
    dev::eth::EnvInfo eth;
    eth.setAuthor(dev::Address(env.blockCreator.data));
    eth.setDifficulty(dev::u256(env.difficulty));
    eth.setGasLimit(env.gasLimit);
    eth.setNumber(dev::u256(env.blockNumber));
    eth.setTimestamp(dev::u256(env.blockTime));
    dev::eth::LastHashes lh;
    lh.resize(256);
    for(int i=0;i<256;i++){
        lh[i]= uintToh256(env.blockHashes[i]);
    }
    eth.setLastHashes(std::move(lh));
    return eth;
}

UniversalAddress UniversalAddress::FromOutput(AddressVersion v, uint256 txid, uint32_t vout){
    std::vector<unsigned char> txIdAndVout(txid.begin(), txid.end());
    std::vector<unsigned char> voutNumberChrs;
    if (voutNumberChrs.size() < sizeof(vout))voutNumberChrs.resize(sizeof(vout));
    std::memcpy(voutNumberChrs.data(), &vout, sizeof(vout));
    txIdAndVout.insert(txIdAndVout.end(),voutNumberChrs.begin(),voutNumberChrs.end());

    std::vector<unsigned char> SHA256TxVout(32);
    CSHA256().Write(txIdAndVout.data(), txIdAndVout.size()).Finalize(SHA256TxVout.data());

    std::vector<unsigned char> hashTxIdAndVout(20);
    CRIPEMD160().Write(SHA256TxVout.data(), SHA256TxVout.size()).Finalize(hashTxIdAndVout.data());

    return UniversalAddress(v, hashTxIdAndVout);
}

QtumTransaction EVMContractVM::buildQtumTx(const ContractOutput &output)
{
    QtumTransaction txEth;
    if (output.address.version == AddressVersion::UNKNOWN && output.OpCreate) {
        txEth = QtumTransaction(output.value, output.gasPrice, output.gasLimit, output.data, dev::u256(0));
        //txEth = QtumTransaction(txBit.vout[nOut].nValue, etp.gasPrice, etp.gasLimit, etp.code, dev::u256(0));
    } else {
        txEth = QtumTransaction(output.value, output.gasPrice, output.gasLimit, dev::Address(output.address.data),
                                std::vector<uint8_t>(output.data.rbegin(), output.data.rend()), dev::u256(0));
        //txEth = QtumTransaction(txBit.vout[nOut].nValue, etp.gasPrice, etp.gasLimit, etp.receiveAddress, etp.code,
        //                        dev::u256(0));
    }
    //todo cross-contract communication?
    dev::Address sender(output.sender.data);
    txEth.forceSender(sender);
    txEth.setHashWith(uintToh256(output.vout.hash));
    txEth.setNVout(output.vout.n);
    txEth.setVersion(output.version);

    return txEth;
}

static const uint8_t byteCodePre[]={'b','y','t','e','c','o','d','e','_'};
static const uint8_t aalPre[]={'a', 'a', 'l', '_'};
static const uint8_t dataPre[]={'_','d','a','t','a','_'};
static const uint8_t updatePre[]={'u','p','d','a','t','e','d','_'};
static const uint8_t keysPre[]={'k','e','y','s','_'};
static const uint8_t iteratorPre[]={'i','t','e','r','t', 'o', 'r','_'};
static const uint8_t infoPre[]={'i','n','f','o','_'};
static const uint8_t oldPre[]={'o','l','d','_'};

bool DeltaDBWrapper::Write(valtype K, valtype V){
    std::string k(K.begin(), K.end());
    (*deltas)[k] = V;
    return true;
    //return db->Write(K, V);
}
bool DeltaDBWrapper::Read(valtype K, valtype& V){
    std::string k(K.begin(), K.end());
    //check from the latest checkpoint to the oldest before giving up and going to database
    for(int i = checkpoints.size() - 1; i >= 0; i++){
        auto *check = &checkpoints[i];
        if(check->find(k) != check->end()){
            V = (*check)[k];
            return true;
        }
    }
    return db->Read(K, V);
}
bool DeltaDBWrapper::Write(valtype K, uint64_t V){
    std::vector<uint8_t> v(sizeof(uint64_t));
    memcpy(v.data(), (void*) &V, sizeof(uint8_t));
    return Write(K, v);
}
bool DeltaDBWrapper::Read(valtype K, uint64_t& V){
    std::vector<uint8_t> v(sizeof(uint64_t));
    int status = db->Read(K, v);
    if(status){
        memcpy((void*)&V, v.data(), sizeof(uint64_t));
    }
    return status;
}

void DeltaDBWrapper::commit() {
    CDBBatch b(*db);
    condenseAllCheckpoints(); //make sure we only have one checkpoint to deal with
    for(auto kv : *deltas){
        if(kv.second.size() == 0){
            b.Erase(kv.first);
        }else{
            b.Write(kv.first, kv.second);
        }
    }
    db->WriteBatch(b, true); //need fSync?

    //clear data stored and reinit
    checkpoints.clear();
    checkpoints.push_back(std::unordered_map<std::string, std::vector<uint8_t>>());
    deltas = &checkpoints[0];
    hasNoAAL.clear();
}
int DeltaDBWrapper::checkpoint() {
    checkpoints.push_back(std::unordered_map<std::string, std::vector<uint8_t>>());
    deltas = &checkpoints[checkpoints.size() - 1];
    balanceCheckpoints.push_back(std::map<UniversalAddress, uint64_t>());
    currentBalances = &balanceCheckpoints[balanceCheckpoints.size() - 1];
    vinCheckpoints.push_back(std::set<COutPoint>());
    currentVins = &vinCheckpoints[vinCheckpoints.size() - 1];
    return checkpoints.size() - 1;
}
int DeltaDBWrapper::revertCheckpoint() {
    if(checkpoints.size() == 1){
        return 0;
    }
    checkpoints.pop_back();
    deltas = &checkpoints[checkpoints.size() - 1];
    balanceCheckpoints.pop_back();
    currentBalances = &balanceCheckpoints[balanceCheckpoints.size() - 1];
    vinCheckpoints.pop_back();
    currentVins = &vinCheckpoints[vinCheckpoints.size() - 1];
    return checkpoints.size() - 1;
}

uint64_t DeltaDBWrapper::getBalance(UniversalAddress a) {
    for(int i=balanceCheckpoints.size() - 1; i >= 0; i--){
        if(balanceCheckpoints[i].find(a) != balanceCheckpoints[i].end()){
            return balanceCheckpoints[i][a];
        }
    }
    uint256 txid;
    unsigned int vout;
    uint64_t balance = 0;
    if(readAalData(a, txid, vout, balance)){
        //(*currentBalances)[a] = balance; //cache it
        return balance;
    }
    return 0;
}

bool DeltaDBWrapper::transfer(UniversalAddress from, UniversalAddress to, uint64_t value) {
    if(value == 0) { return true; }
    uint64_t fromOldBalance = 0;
    bool foundOldBalance = false;
    for (int i = balanceCheckpoints.size() - 1; i >= 0; i--) {
        if (balanceCheckpoints[i].find(from) != balanceCheckpoints[i].end()) {
            fromOldBalance = balanceCheckpoints[i][from];
            foundOldBalance = true;
            break;
        }
    }
    uint256 txid;
    unsigned int vout;
    uint64_t balance;
    if(!foundOldBalance){
        //hasn't been touched in this execution, so lookup from database
        if(readAalData(from, txid, vout, balance)){
            fromOldBalance = balance;
            //since there is a vout being used, we should spend it
            currentVins->insert(COutPoint(txid, vout));
        }
    }
    if (value > fromOldBalance) {
        //not enough balance to cover transfer
        return false;
    }
    (*currentBalances)[from] = fromOldBalance - value;

    if(initialCoinsReceiver == from){
        //if initial coins receiver, then just spend that vin
        //result is either initialCoins is already in currentVins and there is no oldvout
        //OR that both initialCoins and oldvout is already in currentVins
        //OR that initialCoins is not in currentvins and there is no oldvout
        //either way, we don't need to go to database and we must spend the initialCoins vout
        currentVins->insert(initialCoins);
    }else{
        //coins are normal, not from initial coins receiver
        if(readAalData(from, txid, vout, balance)){
            currentVins->insert(COutPoint(txid, vout));
        }
        //if readAalData is false, then no previous vout to spend
        //So it must be "virtual" transfers without an associated UTXO
        //This can happen when transfering coins from A -> B -> C where B had no UTXO before A's execution
    }
    uint64_t toOldBalance = 0;
    //now spend the 'to' utxo if it has one so that both from and to UTXOs are spent for condensing
    for (int i = balanceCheckpoints.size() - 1; i >= 0; i--) {
        if (balanceCheckpoints[i].find(to) != balanceCheckpoints[i].end()) {
            toOldBalance = balanceCheckpoints[i][to];
            break;
        }
    }


}

void DeltaDBWrapper::setInitialCoins(UniversalAddress a, COutPoint vout, uint64_t value) {
    if(balanceCheckpoints.size() != 1){
        return; //this shouldn't be called other than at the very beginning
    }
    uint256 oldtxid;
    unsigned int oldvout;
    uint64_t oldbalance;
    if(readAalData(a, oldtxid, oldvout, oldbalance)){
        //need to spend old vout and sum balance+value
        (*currentBalances)[a] = oldbalance + value;
        //need to spend both old vout and new vout to condense into a single vout
        currentVins->insert(COutPoint(oldtxid, oldvout));
        currentVins->insert(vout);
    }else{
        //no previous record, so just set balance, no need to spend vin
        (*currentBalances)[a] = value;
    }
    initialCoins = vout;
    initialCoinsReceiver = a;
}

void DeltaDBWrapper::condenseAllCheckpoints() {
    if(checkpoints.size() == 1){
        return;
    }
    //can't refactor this to just do multiple condenseSingle
    //without data being touched several times unnecessarily
    deltas = &checkpoints[0];
    //apply from 1 to latest
    for(int i=1;i<checkpoints.size();i++){
        auto* check = &checkpoints[i];
        for(auto &kv : *check){
            (*deltas)[kv.first] = kv.second;
        }
    }
    for(int i=1;i<checkpoints.size();i++){
        checkpoints.pop_back();
    }
    currentBalances = &balanceCheckpoints[0];
    for(int i=1; i < balanceCheckpoints.size(); i++){
        auto *b = &balanceCheckpoints[i];
        for(auto &kv : *b){
            (*currentBalances)[kv.first] = (*b)[kv.first];
        }
    }
    for(int i=1;i<balanceCheckpoints.size();i++){
        balanceCheckpoints.pop_back();
    }
    currentVins = &vinCheckpoints[0];
    for(int i = 1;i < vinCheckpoints.size(); i++){
        for(auto &v : vinCheckpoints[i]){
            currentVins->insert(v);
        }
    }
    for(int i=1;i<vinCheckpoints.size();i++){
        vinCheckpoints.pop_back();
    }
}

void DeltaDBWrapper::condenseSingleCheckpoint() {
    if(checkpoints.size() == 1){
        return;
    }
    deltas = &checkpoints[checkpoints.size() - 2]; //set to previous checkpoint
    auto* check = &checkpoints[checkpoints.size() - 1]; // set to latest checkpoiint
    for(auto &kv : *check){
        (*deltas)[kv.first] = kv.second;
    }
    checkpoints.pop_back(); //remove latest

    auto *b = currentBalances;
    currentBalances = &balanceCheckpoints[balanceCheckpoints.size() - 2];
    for(auto &kv : *b){
        (*currentBalances)[kv.first] = (*b)[kv.first];
    }
    balanceCheckpoints.pop_back();

    auto* vins = currentVins;
    currentVins = &vinCheckpoints[vinCheckpoints.size() - 2];
    for(auto &v : *vins){
        currentVins->insert(v);
    }
    vinCheckpoints.pop_back();
}




/* Bytecode of contract: KEY is derived from %bytecode%_%address% */
bool DeltaDBWrapper:: writeByteCode(UniversalAddress address,valtype byteCode){
	std::vector<uint8_t> K;
	std::vector<uint8_t> V;
	K.insert(K.end(), address.version);
	K.insert(K.end(), address.data.begin(), address.data.end());	
	K.insert(K.end(), byteCodePre, byteCodePre + sizeof(byteCodePre)/sizeof(uint8_t));
	K.insert(K.end(), 'c');	
	return Write(K, byteCode);
}

bool DeltaDBWrapper:: readByteCode(UniversalAddress address,valtype& byteCode){
	std::vector<uint8_t> K;
	std::vector<uint8_t> V;
	K.insert(K.end(), address.version);
	K.insert(K.end(), address.data.begin(), address.data.end());	
	K.insert(K.end(), byteCodePre, byteCodePre + sizeof(byteCodePre)/sizeof(uint8_t));
	K.insert(K.end(), 'c');	
    return Read(K, byteCode);   
}

bool DeltaDBWrapper:: writeAalData(UniversalAddress address, uint256 txid, unsigned int vout, uint64_t balance){
	std::vector<uint8_t> K;
	std::vector<uint8_t> V;
	K.insert(K.end(), address.version);
	K.insert(K.end(), address.data.begin(), address.data.end());	
	K.insert(K.end(), aalPre, aalPre + sizeof(aalPre)/sizeof(uint8_t));
	K.insert(K.end(), 'u');	

	CDataStream dsValue(SER_DISK,0);
	dsValue<<txid;	
	dsValue<<vout;
	dsValue<<balance;	
	V.insert(V.end(),dsValue.begin(),dsValue.end());
	return Write(K, V);
}

bool DeltaDBWrapper:: readAalData(UniversalAddress address, uint256 &txid, unsigned int &vout, uint64_t &balance){
    if(hasNoAAL.find(address) == hasNoAAL.end()){
        return false;
    }
	std::vector<uint8_t> K;
	std::vector<uint8_t> V;
	K.insert(K.end(), address.version);
	K.insert(K.end(), address.data.begin(), address.data.end());
	K.insert(K.end(), aalPre , aalPre + sizeof(aalPre)/sizeof(uint8_t));
	K.insert(K.end(), 'u');	
	if(Read(K, V)){
		CDataStream dsValue(V,SER_DISK,0);
		dsValue>>txid;	
		dsValue>>vout;
		dsValue>>balance;
		return true;
	}else{
        hasNoAAL.insert(address);
        return false;
	}
}

bool DeltaDBWrapper:: writeState(UniversalAddress address, valtype key, valtype value){
	std::vector<uint8_t> K;
    std::vector<unsigned char> keyHash(32);

	K.insert(K.end(), address.version);
	K.insert(K.end(), address.data.begin(), address.data.end());	
	K.insert(K.end(), dataPre, dataPre + sizeof(dataPre)/sizeof(uint8_t));
	if(key.size() > 31){
		CSHA256().Write(key.data(), key.size()).Finalize(keyHash.data());
		K.insert(K.end(), keyHash.begin(),keyHash.end());	
	}else{
		K.insert(K.end(), '_'); 
		K.insert(K.end(), key.begin(),key.end());	
	}
	return Write(K, value);
}

bool DeltaDBWrapper:: readState(UniversalAddress address, valtype key, valtype& value){
	std::vector<uint8_t> K;
    std::vector<unsigned char> keyHash(32);

	K.insert(K.end(), address.version);
	K.insert(K.end(), address.data.begin(), address.data.end());	
	K.insert(K.end(), dataPre, dataPre + sizeof(dataPre)/sizeof(uint8_t));
	if(key.size() > 31){
		CSHA256().Write(key.data(), key.size()).Finalize(keyHash.data());
		K.insert(K.end(), keyHash.begin(),keyHash.end());	
	}else{
		K.insert(K.end(), '_'); 
		K.insert(K.end(), key.begin(),key.end());	
	}
    return Read(K, value);
}



/* Live data of contract updated:  %address%_updated_%key%_ */
bool DeltaDBWrapper:: writeUpdatedKey(UniversalAddress address, valtype key, unsigned int blk_num, uint256 blk_hash){
	std::vector<uint8_t> K;
	std::vector<uint8_t> V;
	K.insert(K.end(), address.version);
	K.insert(K.end(), address.data.begin(), address.data.end());	
	K.insert(K.end(), '_');
	K.insert(K.end(), updatePre, updatePre + sizeof(updatePre)/sizeof(uint8_t));
	K.insert(K.end(), key.begin(),key.end());	
	
	CDataStream dsValue(SER_DISK,0);
	dsValue<<blk_num;
	dsValue<<blk_hash;
	V.insert(V.end(),dsValue.begin(),dsValue.end());
	return Write(K, V);

}

bool DeltaDBWrapper:: readUpdatedKey(UniversalAddress address, valtype key, unsigned int &blk_num, uint256 &blk_hash){
	std::vector<uint8_t> K;
	std::vector<uint8_t> V;
	K.insert(K.end(), address.version);
	K.insert(K.end(), address.data.begin(), address.data.end());	
	K.insert(K.end(), '_');
	K.insert(K.end(), updatePre, updatePre + sizeof(updatePre)/sizeof(uint8_t));
	K.insert(K.end(), key.begin(),key.end());	
	if(Read(K, V)){
		CDataStream dsValue(V,SER_DISK,0);
		dsValue>>blk_num;
		dsValue>>blk_hash;
		return true;
	}else{
        return false;
	}
}

/* the raw unhashed key to be looked up by hash: %address%_keys_%key% */
bool DeltaDBWrapper:: writeRawKey(UniversalAddress address,	valtype key, valtype rawkey){
	std::vector<uint8_t> K;
	K.insert(K.end(), address.version);
	K.insert(K.end(), address.data.begin(), address.data.end());	
	K.insert(K.end(), '_');
	K.insert(K.end(), keysPre, keysPre + sizeof(keysPre)/sizeof(uint8_t));
	K.insert(K.end(), key.begin(),key.end());	
	return Write(K, rawkey);
}

bool DeltaDBWrapper:: readRawKey(UniversalAddress address, valtype key, valtype &rawkey){
	std::vector<uint8_t> K;
	K.insert(K.end(), address.version);
	K.insert(K.end(), address.data.begin(), address.data.end());	
	K.insert(K.end(), '_');
	K.insert(K.end(), keysPre, keysPre + sizeof(keysPre)/sizeof(uint8_t));
	K.insert(K.end(), key.begin(),key.end());	
	return Read(K, rawkey);
}

/* current iterator of a key: %address%_iterator_%key% */
bool DeltaDBWrapper:: writeCurrentIterator(UniversalAddress address, valtype key, uint64_t iterator){
	std::vector<uint8_t> K;
	K.insert(K.end(), address.version);
	K.insert(K.end(), address.data.begin(), address.data.end());	
	K.insert(K.end(), '_');
	K.insert(K.end(), iteratorPre, iteratorPre + sizeof(iteratorPre)/sizeof(uint8_t));
	K.insert(K.end(), key.begin(),key.end());	
	return Write(K, iterator);
}

bool DeltaDBWrapper:: readCurrentIterator(UniversalAddress address,		valtype key, uint64_t &iterator){
	std::vector<uint8_t> K;
	K.insert(K.end(), address.version);
	K.insert(K.end(), address.data.begin(), address.data.end());	
	K.insert(K.end(), '_');
	K.insert(K.end(), iteratorPre, iteratorPre + sizeof(iteratorPre)/sizeof(uint8_t));
	K.insert(K.end(), key.begin(),key.end());	
	return Read(K, iterator);
}

/*  key's data at point of iterator: %address%_data_%key%_%iterator% */
bool DeltaDBWrapper:: writeStateWithIterator(UniversalAddress address,		 valtype key, uint64_t iterator, valtype value){
	std::vector<uint8_t> K;
	CDataStream dsKey(SER_DISK,0);
	dsKey<<iterator;
	K.insert(K.end(), address.version);
	K.insert(K.end(), address.data.begin(), address.data.end());	
	K.insert(K.end(), dataPre, dataPre + sizeof(dataPre)/sizeof(uint8_t));
	K.insert(K.end(), key.begin(),key.end());
	K.insert(K.end(), dsKey.begin(),dsKey.end());	
	return Write(K, value);
}


bool DeltaDBWrapper:: readStateWithIterator(UniversalAddress address,		valtype key, uint64_t iterator, valtype &value){
	std::vector<uint8_t> K;
	CDataStream dsKey(SER_DISK,0);
	dsKey<<iterator;
	K.insert(K.end(), address.version);
	K.insert(K.end(), address.data.begin(), address.data.end());	
	K.insert(K.end(), dataPre, dataPre + sizeof(dataPre)/sizeof(uint8_t));
	K.insert(K.end(), key.begin(),key.end());	
	K.insert(K.end(), dsKey.begin(),dsKey.end());	
	return Read(K, value);
}

/* block number + block hash + txid/vout at point of iterator: %address%_info_%key%_%iterator%  */
bool DeltaDBWrapper:: writeInfoWithIterator(UniversalAddress address,		valtype key, uint64_t iterator, unsigned int blk_num, uint256 blk_hash, uint256 txid, unsigned int vout){
	std::vector<uint8_t> K;
	std::vector<uint8_t> V;
	CDataStream dsKey(SER_DISK,0);
	dsKey<<iterator;
	K.insert(K.end(), address.version);
	K.insert(K.end(), address.data.begin(), address.data.end());	
	K.insert(K.end(), '_');
	K.insert(K.end(), infoPre, infoPre + sizeof(infoPre)/sizeof(uint8_t));
	K.insert(K.end(), key.begin(),key.end());	
	K.insert(K.end(), dsKey.begin(),dsKey.end());

	CDataStream dsValue(SER_DISK,0);
	dsValue<<blk_num;	
	dsValue<<blk_hash;	
	dsValue<<txid;	
	dsValue<<vout;	
	V.insert(V.end(),dsValue.begin(),dsValue.end());
	return Write(K, V);
}

bool DeltaDBWrapper:: readInfoWithIterator(UniversalAddress address, 	   valtype key, uint64_t iterator, unsigned int &blk_num, uint256 &blk_hash, uint256 &txid, unsigned int &vout){
	std::vector<uint8_t> K;
	std::vector<uint8_t> V;
	CDataStream dsKey(SER_DISK,0);
	dsKey<<iterator;
	K.insert(K.end(), address.version);
	K.insert(K.end(), address.data.begin(), address.data.end());	
	K.insert(K.end(), '_');
	K.insert(K.end(), infoPre, infoPre + sizeof(infoPre)/sizeof(uint8_t));
	K.insert(K.end(), key.begin(),key.end());
	K.insert(K.end(), dsKey.begin(),dsKey.end());
	if(Read(K, V)){
		CDataStream dsValue(V,SER_DISK,0);
		dsValue>>blk_num;
		dsValue>>blk_hash;
		dsValue>>txid;	
		dsValue>>vout;	
		return true;
	}else{
        return false;
	}
}

/* Oldest iterator that exists in the changelog database: %address%_old_%key% */
bool DeltaDBWrapper:: writeOldestIterator(UniversalAddress address,		  valtype key, uint64_t iterator, unsigned int blk_num, uint256 blk_hash){
	std::vector<uint8_t> K;
	std::vector<uint8_t> V;
	K.insert(K.end(), address.version);
	K.insert(K.end(), address.data.begin(), address.data.end());	
	K.insert(K.end(), '_');
	K.insert(K.end(), oldPre, oldPre + sizeof(oldPre)/sizeof(uint8_t));
	K.insert(K.end(), key.begin(),key.end());	

	CDataStream dsValue(SER_DISK,0);
	dsValue<<iterator;	
	dsValue<<blk_num;	
	dsValue<<blk_hash;	
	V.insert(V.end(),dsValue.begin(),dsValue.end());
	return Write(K, V);
}

bool DeltaDBWrapper:: readOldestIterator(UniversalAddress address,		 valtype key, uint64_t &iterator, unsigned int &blk_num, uint256 &blk_hash){
	std::vector<uint8_t> K;
	std::vector<uint8_t> V;
	K.insert(K.end(), address.version);
	K.insert(K.end(), address.data.begin(), address.data.end());	
	K.insert(K.end(), '_');
	K.insert(K.end(), oldPre, oldPre + sizeof(oldPre)/sizeof(uint8_t));
	K.insert(K.end(), key.begin(),key.end());
	if(Read(K, V)){
		CDataStream dsValue(V,SER_DISK,0);
		dsValue>>iterator;	
		dsValue>>blk_num;
		dsValue>>blk_hash;
		return true;
	}else{
        return false;
	}
}


