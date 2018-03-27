
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <util.h>
#include <tinyformat.h>
#include <script/standard.h>
#include <chainparams.h>
#include <validation.h>
#include <pubkey.h>
#include "qtumtransaction.h"


bool ContractOutputParser::parseOutput(ContractOutput& output){
    try{
        std::vector<uint8_t> receiveAddress;
        valtype vecAddr;
        if (opcode == OP_CALL)
        {
            vecAddr = stack.back();
            stack.pop_back();
            receiveAddress = vecAddr;
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
    assert(*tip->phashBlock == block.GetHash());
    env.blockNumber = tip->nHeight;
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
    ContractEnvironment env=buildEnv();
    DeltaDB db; //todo
    if(output.version.rootVM == ROOT_VM_EVM){
        EVMContractVM evm(db, env, blockGasLimit);
        evm.execute(commit);
    }
    return true;
}

bool EVMContractVM::execute(bool commit)
{
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