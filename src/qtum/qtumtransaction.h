#ifndef QTUMTRANSACTION_H
#define QTUMTRANSACTION_H

#include <libethcore/Transaction.h>
#include <libethereum/Transaction.h>
#include <coins.h>
#include <script/interpreter.h>
#include <libevm/ExtVMFace.h>
#include <dbwrapper.h>
#include <util.h>
#include <uint256.h>

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


enum AddressVersion{
    UNKNOWN = 0,
    //legacy is either pubkeyhash or EVM, depending on if the address already exists
    LEGACYEVM = 1,
    PUBKEYHASH = 2,
    EVM = 3,
    X86 = 4,
    SCRIPTHASH = 5,
};

struct UniversalAddress{
    UniversalAddress(){
        version = AddressVersion::UNKNOWN;
    }
    UniversalAddress(AddressVersion v, const std::vector<uint8_t> &d)
    : version(v), data(d) {}
    UniversalAddress(AddressVersion v, const unsigned char* begin, const unsigned char* end)
    : version(v), data(begin, end) {}
    AddressVersion version;
    std::vector<uint8_t> data;

    bool operator<(const UniversalAddress& a) const{
        return data < a.data;
    }
    bool operator==(const UniversalAddress& a) const{
        return version == a.version && data == a.data;
    }
    bool operator!=(const UniversalAddress& a) const{
        return !(a == *this);
    }

    static UniversalAddress FromScript(const CScript& script);
    static UniversalAddress FromOutput(AddressVersion v, uint256 txid, uint32_t vout);
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
	

};

enum ContractStatus{
    SUCCESS = 0,
    OUT_OF_GAS = 1,
    CODE_ERROR = 2,
    DOESNT_EXIST = 3
};

struct ContractExecutionResult{
    uint64_t usedGas;
    CAmount refundSender = 0;
    ContractStatus status;
    CMutableTransaction transferTx;
};

class QtumTransaction;
class x86ContractVM;
//the abstract class for the VM interface
//in the future, enterprise/private VMs will use this interface
class ContractVM{
    //todo database
protected:
    ContractVM(DeltaDB &_db, const ContractEnvironment &_env, uint64_t _remainingGasLimit)
    : db(db), env(_env), remainingGasLimit(_remainingGasLimit) {}
public:
    virtual bool execute(ContractOutput &output, ContractExecutionResult &result, bool commit)=0;
protected:
    DeltaDB &db;
    const ContractEnvironment &env;
    const uint64_t remainingGasLimit;
};



class EVMContractVM : public ContractVM {
public:
    EVMContractVM(DeltaDB &db, const ContractEnvironment &env, uint64_t remainingGasLimit)
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
