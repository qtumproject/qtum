#ifndef QTUMTRANSACTION_H
#define QTUMTRANSACTION_H

#include <unordered_map>

#include <univalue.h>
#include <libethcore/Transaction.h>
#include <libethereum/Transaction.h>
#include <coins.h>
#include <script/interpreter.h>
#include <libevm/ExtVMFace.h>
#include <dbwrapper.h>
#include <util.h>
#include <uint256.h>
#include <base58.h>
#include "qtumstate.h"


std::string parseABIToString(std::string abidata);

struct VersionVM{
    //this should be portable, see https://stackoverflow.com/questions/31726191/is-there-a-portable-alternative-to-c-bitfields
# if __BYTE_ORDER == __LITTLE_ENDIAN
    uint8_t format : 2;
    uint8_t rootVM : 6;
#elif __BYTE_ORDER == __BIG_ENDIAN
    uint8_t rootVM : 6;
    uint8_t format : 2;
#endif
    uint8_t vmVersion;
    uint16_t flagOptions;
    // CONSENSUS CRITICAL!
    // Do not add any other fields to this struct

    uint32_t toRaw(){
        return *(uint32_t*)this;
    }
    static VersionVM fromRaw(uint32_t val){
        VersionVM x = *(VersionVM*)&val;
        return x;
    }
    static VersionVM GetNoExec(){
        VersionVM x;
        x.flagOptions=0;
        x.rootVM=0;
        x.format=0;
        x.vmVersion=0;
        return x;
    }
    static VersionVM GetNoExecVersion2(){
        //for new AAL
        VersionVM x;
        x.flagOptions = 0;
        x.rootVM = 0;
        x.format = 0;
        x.vmVersion = 2;
        return x;
    }
    static VersionVM GetEVMDefault(){
        VersionVM x;
        x.flagOptions=0;
        x.rootVM=1;
        x.format=0;
        x.vmVersion=0;
        return x;
    }
    static VersionVM Getx86Default(){
        VersionVM x;
        x.flagOptions=0;
        x.rootVM=2;
        x.format=0;
        x.vmVersion=0;
        return x;
    }
}__attribute__((__packed__));

static const uint8_t ROOT_VM_NULL = 0;
static const uint8_t ROOT_VM_EVM = 1;
static const uint8_t ROOT_VM_X86 = 2;

//Note: we don't use Params().Base58Prefixes for these version numbers because otherwise
//contracts would need to use two different SDKs since the the base58 version for pubkeyhash etc changes
//between regtest, testnet, and mainnet
enum AddressVersion{
    UNKNOWN = 0,
    //legacy is either pubkeyhash or EVM, depending on if the address already exists
    LEGACYEVM = 1,
    PUBKEYHASH = 2,
    EVM = 3,
    X86 = 4,
    SCRIPTHASH = 5,
};

static const size_t ADDRESS_DATA_SIZE = 32;

struct UniversalAddressABI{
    //Do not modify this struct's fields
    //This is consensus critical!
    uint32_t version;
    uint8_t data[ADDRESS_DATA_SIZE];
}__attribute__((__packed__));

struct UniversalAddress{
    UniversalAddress(){
        version = AddressVersion::UNKNOWN;
        convertData();
    }
    UniversalAddress(AddressVersion v, const std::vector<uint8_t> &d)
    : version(v), data(d.begin(), d.end()) {
        convertData();
    }
    UniversalAddress(AddressVersion v, const unsigned char* begin, const unsigned char* end)
    : version(v), data(begin, end) {
        convertData();
    }
    UniversalAddress(CBitcoinAddress &address){
        fromBitcoinAddress(address);
    }
    UniversalAddress(UniversalAddressABI &abi)
    : version((AddressVersion)abi.version), data(&abi.data[0], &abi.data[sizeof(abi.data)])
    {
        convertData();
    }

    bool operator<(const UniversalAddress& a) const{
        return version < a.version || data < a.data;
    }
    bool operator==(const UniversalAddress& a) const{
        return version == a.version && data == a.data;
    }
    bool operator!=(const UniversalAddress& a) const{
        return !(a == *this);
    }
    UniversalAddressABI toAbi() const{
        UniversalAddressABI abi;
        toAbi(abi);
        return abi;
    }
    void toAbi(UniversalAddressABI &abi) const{
        abi.version = (uint32_t) version;
        memset(&abi.data[0], 0, ADDRESS_DATA_SIZE);
        memcpy(&abi.data[0], data.data(), data.size());
    }
    //converts to flat data suitable for passing to contract code
    std::vector<uint8_t> toFlatData() const{
        std::vector<uint8_t> tmp;
        tmp.resize(sizeof(UniversalAddressABI));
        UniversalAddressABI abi = toAbi();
        memcpy(&tmp.front(), &abi, tmp.size());
        return tmp;
    }

    //converts to flat data suitable for placing in blockchain
    std::vector<uint8_t> toChainData() const{
        std::vector<uint8_t> tmp;
        size_t sz = getRealAddressSize(version);
        tmp.resize(sz);
        memcpy(&tmp.front(), data.data(), sz);
        return tmp;
    }
    //hasAAL means this type of address should have an AAL record in DeltaDB
    bool hasAAL() const{
        return version == AddressVersion::EVM ||
               version == AddressVersion::X86;
    }
    bool isContract() const{
        return hasAAL();
    }

    static UniversalAddress FromScript(const CScript& script);
    static UniversalAddress FromOutput(AddressVersion v, uint256 txid, uint32_t vout);

    CBitcoinAddress asBitcoinAddress() const{
        CBitcoinAddress a(convertUniversalVersion(version), data.data(), getRealAddressSize(version));
        return a;
    }
    void fromBitcoinAddress(CBitcoinAddress &address) {
        version = convertBitcoinVersion(address.getVersion());
        data = address.getData();
        convertData();
    }
    void convertData(){
        data.resize(ADDRESS_DATA_SIZE);
    }

    static AddressVersion convertBitcoinVersion(std::vector<unsigned char> version){
        unsigned char v = version[0];
        if(Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS)[0] == v){
            return AddressVersion::PUBKEYHASH;
        }else if(Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS)[0] == v){
            return AddressVersion::SCRIPTHASH;
        }else if(Params().Base58Prefix(CChainParams::EVM_ADDRESS)[0] == v) {
            return AddressVersion::EVM;
        }else if(Params().Base58Prefix(CChainParams::X86VM_ADDRESS)[0] == v) {
            return AddressVersion::X86;
        }else {
            return AddressVersion::UNKNOWN;
        }
    }
    static std::vector<unsigned char> convertUniversalVersion(AddressVersion v){
        if(v == AddressVersion::EVM){
            return Params().Base58Prefix(CChainParams::EVM_ADDRESS);
        }else if(v == AddressVersion::X86){
            return Params().Base58Prefix(CChainParams::X86VM_ADDRESS);
        }else if(v == AddressVersion::PUBKEYHASH){
            return Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS);
        }else if(v == AddressVersion::SCRIPTHASH){
            return Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS);
        }else{
            return std::vector<unsigned char>();
        }
    }
    static size_t getRealAddressSize(AddressVersion v){
        switch(v){
            case AddressVersion::EVM:
            case AddressVersion::X86:
            case AddressVersion::PUBKEYHASH:
            case AddressVersion::LEGACYEVM:
            case AddressVersion::SCRIPTHASH:
            return 20;
            default:
            return 32;
        }
    }


    AddressVersion version;
    std::vector<uint8_t> data;
};


struct ContractOutput{
    VersionVM version;
    uint64_t value, gasPrice, gasLimit;
    UniversalAddress address;
    std::vector<uint8_t> data;
    UniversalAddress sender;
    COutPoint vout;
    bool OpCreate;
};

class ContractOutputParser{
public:

    ContractOutputParser(const CTransaction &tx, uint32_t vout, const CCoinsViewCache* v = NULL, const std::vector<CTransactionRef>* blockTxs = NULL)
            : tx(tx), nvout(vout), view(v), blockTransactions(blockTxs) {}
    bool parseOutput(ContractOutput& output);
    UniversalAddress getSenderAddress();

private:
    bool receiveStack(const CScript& scriptPubKey);
    const CTransaction &tx;
    const uint32_t nvout;
    const CCoinsViewCache* view;
    const std::vector<CTransactionRef> *blockTransactions;
    std::vector<valtype> stack;
    opcodetype opcode;

};

struct ContractEnvironment{
    uint32_t blockNumber;
    uint64_t blockTime;
    uint64_t difficulty;
    uint64_t gasLimit;
    UniversalAddress blockCreator;
    std::vector<uint256> blockHashes;

    //todo for x86: tx info
};


class DeltaDB : public CDBWrapper
{

public:
	DeltaDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "deltaDB", nCacheSize, fMemory, fWipe) { }	
	DeltaDB() : CDBWrapper(GetDataDir() / "deltaDB", 4, false, false) { }
	~DeltaDB() {    }
};

struct DeltaCheckpoint{
    //all state changes in current checkpoint
    std::unordered_map<std::string, std::vector<uint8_t>> deltas;
    //all vins spent in transfers within the current checkpoint
    std::set<COutPoint> spentVins;
    //all addresses with modified balances in the current checkpoint
    //note: do not use this as a cache. It should only be used to track modified balances
    std::map<UniversalAddress, uint64_t> balances;

    UniValue toJSON();
};

class DeltaDBWrapper{
    DeltaDB* db;
    //0 is 0th checkpoint, 1 is 1st checkpoint etc
    std::vector<DeltaCheckpoint> checkpoints;
    DeltaCheckpoint *current;

    std::set<UniversalAddress> hasNoAAL; //a cache to keep track of which addresses have no AAL data in the disk-database
    COutPoint initialCoins; //initial coins sent by origin tx
    UniversalAddress initialCoinsReceiver;
public:
    DeltaDBWrapper(DeltaDB* db_) : db(db_){
        checkpoint(); //this will add the initial "0" checkpoint and set all pointers
    }

    void commit(); //commits everything to disk
    int checkpoint(); //advanced to next checkpoint; returns new checkpoint number
    int revertCheckpoint(); //Discard latest checkpoint and revert to previous checkpoint; returns new checkpoint number
    void condenseAllCheckpoints(); //condences all outstanding checkpoints to 0th
    void condenseSingleCheckpoint(); //condenses only the latest checkpoint into the previous
    void setInitialCoins(UniversalAddress a, COutPoint vout, uint64_t value); //initial coins sent with origin tx
    //AAL access
    uint64_t getBalance(UniversalAddress a);
    bool transfer(UniversalAddress from, UniversalAddress to, uint64_t value);

    DeltaCheckpoint getLatestModifiedState(){
        return *current;
    }

    CTransaction createCondensingTx();

    /*************** Live data *****************/
    /* newest data associated with the contract. */
    bool writeState(UniversalAddress address, valtype key, valtype value);
    bool readState(UniversalAddress address, valtype key, valtype& value);

    /* bytecode of the contract. */
    bool writeByteCode(UniversalAddress address,valtype byteCode);
    bool readByteCode(UniversalAddress address,valtype& byteCode);


    /* data updated point of the keys in a contract. */
    bool writeUpdatedKey(UniversalAddress address, valtype key, unsigned int blk_num, uint256 blk_hash);
    bool readUpdatedKey(UniversalAddress address, valtype key, unsigned int &blk_num, uint256 &blk_hash);


    /*************** Change log data *****************/
    /* raw name of the keys in a contract. */
    bool writeRawKey(UniversalAddress address,      valtype key, valtype rawkey);
    bool readRawKey(UniversalAddress address,     valtype key, valtype &rawkey);

    /* current iterator of the keys in a contract. */
    bool writeCurrentIterator(UniversalAddress address,      valtype key, uint64_t iterator);
    bool readCurrentIterator(UniversalAddress address,      valtype key, uint64_t &iterator);

    /* data of the keys in a contract, indexed by iterator. */
    bool writeStateWithIterator(UniversalAddress address,        valtype key, uint64_t iterator, valtype value);
    bool readStateWithIterator(UniversalAddress address,        valtype key, uint64_t iterator, valtype &value);

    /* info of the keys in a contract, indexed by iterator. */
    bool writeInfoWithIterator(UniversalAddress address,        valtype key, uint64_t iterator, unsigned int blk_num, uint256 blk_hash, uint256 txid, unsigned int vout);
    bool readInfoWithIterator(UniversalAddress address,        valtype key, uint64_t iterator, unsigned int &blk_num, uint256 &blk_hash, uint256 &txid, unsigned int &vout);

    /* Oldest iterator and the respect block info that exists in the changelog database. */
    bool writeOldestIterator(UniversalAddress address,        valtype key, uint64_t iterator, unsigned int blk_num, uint256 blk_hash);
    bool readOldestIterator(UniversalAddress address,        valtype key, uint64_t &iterator, unsigned int &blk_num, uint256 &blk_hash);

private:
    //AAL is more complicated, so don't allow direct access
    bool writeAalData(UniversalAddress address, uint256 txid, unsigned int vout, uint64_t balance);
    bool readAalData(UniversalAddress address, uint256 &txid, unsigned int &vout, uint64_t &balance);
    bool Write(valtype K, valtype V);
    bool Read(valtype K, valtype& V);
    bool Write(valtype K, uint64_t V);
    bool Read(valtype K, uint64_t& V);
};

class ContractStatus{
    int status;
    std::string statusString;
    std::string extraString;
    ContractStatus(){}
    ContractStatus(int code, std::string str, std::string extra) : status(code), statusString(str), extraString(extra) {}

    public:

    int getCode(){
        return status;
    }
    bool isError(){
        return status != 0;
    }
    std::string toString(){
        if(extraString == ""){
            return statusString;
        }else{
            return statusString + "; Extra info: " + extraString;
        }
    }

    static ContractStatus Success(std::string extra=""){
        return ContractStatus(0, "Success", extra);
    }
    static ContractStatus OutOfGas(std::string extra=""){
        return ContractStatus(1, "Out of gas", extra);
    }
    static ContractStatus CodeError(std::string extra=""){
        return ContractStatus(2, "Unhandled exception triggered in execution ", extra);
    }
    static ContractStatus DoesntExist(std::string extra=""){
        return ContractStatus(3, "Contract does not exist", extra);
    }
    static ContractStatus ReturnedError(std::string extra=""){
        return ContractStatus(4, "Contract executed successfully but returned an error code", extra);
    }
    static ContractStatus ErrorWithCommit(std::string extra=""){
        return ContractStatus(5, "Contract chose to commit state, but returned an error code", extra);
    }
};

struct ContractExecutionResult{
    uint64_t usedGas;
    CAmount refundSender = 0;
    ContractStatus status = ContractStatus::CodeError();
    CMutableTransaction transferTx;
    bool commitState;
    DeltaCheckpoint modifiedData;
    std::map<std::string, std::string> events;
    std::vector<ContractExecutionResult> callResults;
    UniversalAddress address;

    UniValue toJSON(){
        UniValue result(UniValue::VOBJ);
        result.push_back(Pair("address", address.asBitcoinAddress().ToString()));
        result.push_back(Pair("used-gas", usedGas));
        result.push_back(Pair("sender-refund", refundSender));
        result.push_back(Pair("status", status.toString()));
        result.push_back(Pair("status-code", status.getCode()));
        result.push_back(Pair("transfer-txid", transferTx.GetHash().ToString()));
        result.push_back(Pair("commit-state", commitState));
        result.push_back(Pair("modified-state", modifiedData.toJSON()));
        UniValue returnjson(UniValue::VOBJ);
        for(auto& kvp : events){
            returnjson.push_back(Pair(parseABIToString(kvp.first), parseABIToString(kvp.second)));
        }
        result.push_back(Pair("events", returnjson));
        UniValue calls(UniValue::VARR);
        for(auto& res : callResults){
            calls.push_back(res.toJSON());
        }
        result.push_back(Pair("calls", calls));
        return result;
    }
};

class QtumTransaction;
class x86ContractVM;
//the abstract class for the VM interface
//in the future, enterprise/private VMs will use this interface
class ContractVM{
    //todo database
protected:
    ContractVM(DeltaDBWrapper &_db, const ContractEnvironment &_env, uint64_t _remainingGasLimit)
    : db(_db), env(_env), remainingGasLimit(_remainingGasLimit) {}
public:
    virtual bool execute(ContractOutput &output, ContractExecutionResult &result, bool commit)=0;
protected:
    DeltaDBWrapper &db;
    const ContractEnvironment &env;
    const uint64_t remainingGasLimit;
};



class EVMContractVM : public ContractVM {
public:
    EVMContractVM(DeltaDBWrapper &db, const ContractEnvironment &env, uint64_t remainingGasLimit)
            : ContractVM(db, env, remainingGasLimit)
    {}
    virtual bool execute(ContractOutput &output, ContractExecutionResult &result, bool commit);
private:
    dev::eth::EnvInfo buildEthEnv();
    QtumTransaction buildQtumTx(const ContractOutput &output);
};

class ContractExecutor{
public:
    ContractExecutor(const CBlock& _block, ContractOutput _output, uint64_t _blockGasLimit);
    bool execute(ContractExecutionResult &result, bool commit);
private:
    bool buildTransferTx(ContractExecutionResult& res);
    ContractEnvironment buildEnv();
    const CBlock& block;
    ContractOutput output;
    const uint64_t blockGasLimit;
};

class QtumTransaction : public dev::eth::Transaction{

public:

    QtumTransaction() : nVout(0) {}

    QtumTransaction(dev::u256 const& _value, dev::u256 const& _gasPrice, dev::u256 const& _gas, dev::bytes const& _data, dev::u256 const& _nonce = dev::Invalid256):
		dev::eth::Transaction(_value, _gasPrice, _gas, _data, _nonce) {}

    QtumTransaction(dev::u256 const& _value, dev::u256 const& _gasPrice, dev::u256 const& _gas, dev::Address const& _dest, dev::bytes const& _data, dev::u256 const& _nonce = dev::Invalid256):
		dev::eth::Transaction(_value, _gasPrice, _gas, _dest, _data, _nonce) {}

    void setHashWith(const dev::h256 hash) { m_hashWith = hash; }

    dev::h256 getHashWith() const { return m_hashWith; }

    void setNVout(uint32_t vout) { nVout = vout; }

    uint32_t getNVout() const { return nVout; }

    void setVersion(VersionVM v){
        version=v;
    }
    VersionVM getVersion() const{
        return version;
    }
private:

    uint32_t nVout;
    VersionVM version;

};


#endif
