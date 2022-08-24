#ifndef QTUMNFT_H
#define QTUMNFT_H
#include <string>
#include <vector>
#include <map>
#include <uint256.h>

struct QtumNftData;
class FunctionABI;

struct WalletNFTInfo {
    uint256 NFTId;
    std::string name;
    std::string url;
    std::string desc;
    int64_t createAt;
    int32_t count;

    WalletNFTInfo()
    {
        SetNull();
    }

    void SetNull()
    {
        NFTId.SetNull();
        createAt = 0;
        count = 0;
    }
};

struct NftEvent{
    std::string address;
    std::string sender;
    std::string receiver;
    uint256 blockHash;
    uint64_t blockNumber;
    uint256 transactionHash;
    uint256 id;
    int32_t value;

    NftEvent()
    {
        SetNull();
    }

    void SetNull()
    {
        blockHash.SetNull();
        blockNumber = 0;
        transactionHash.SetNull();
        id.SetNull();
        value = 0;
    }
};

class QtumNftExec
{
public:
    virtual bool execValid(const int& func, const bool& sendTo);
    virtual bool execEventsValid(const int& func, const int64_t& fromBlock);
    virtual bool exec(const bool& sendTo, const std::map<std::string, std::string>& lstParams, std::string& result, std::string& message);
    virtual bool execEvents(const int64_t& fromBlock, const int64_t& toBlock, const int64_t& minconf, const std::string& eventName, const std::string& contractAddress, const int& numTopics, const FunctionABI& func, std::vector<NftEvent>& result);
    virtual bool privateKeysDisabled();
    virtual bool isEventMine(const std::string& sender, const std::string& receiver);
    virtual bool addEvent(const FunctionABI &func, const std::vector<std::string>& topics, const std::string& data, NftEvent nftEvent, std::vector<NftEvent> &result);
    virtual bool parseEvent(const FunctionABI &func, const std::vector<std::string>& topics, const std::string& data, NftEvent& nftEvent, std::vector<uint256>& evtIds, std::vector<int32_t>& evtValues);
    virtual bool filterMatch(const NftEvent& nftEvent);
    virtual ~QtumNftExec();
};

class QtumNft
{
public:
    QtumNft();
    virtual ~QtumNft();

    void setQtumNftExec(QtumNftExec* nftExec);

    // Set command data
    void setAddress(const std::string &address);
    void setDataHex(const std::string &datahex);
    void setAmount(const std::string &amount);
    void setGasLimit(const std::string &gaslimit);
    void setGasPrice(const std::string &gasPrice);
    void setSender(const std::string &sender);
    void clear();

    // Get transaction data
    std::string getTxId();
    std::string getPsbt();
    std::string getErrorMessage();
    std::string getAddress();

    // Set transaction data
    void setTxId(const std::string& txid);

    // ABI Functions
    bool balanceOf(const std::string& account, const uint256& id, int32_t& result, bool sendTo = false);
    bool balanceOf(const uint256& id, int32_t& result, bool sendTo = false);
    bool createNFT(const std::string& owner, const std::string& name, const std::string& url, const std::string& desc, const int32_t& count, bool sendTo = false);
    bool createNFT(const std::string& name, const std::string& url, const std::string& desc, const int32_t& count, bool sendTo = false);
    bool isApprovedForAll(const std::string& account, const std::string& operant, bool& success, bool sendTo = false);
    bool safeTransferFrom(const std::string& from, const std::string& to, const uint256& id, const int32_t& amount, bool sendTo = false);
    bool safeTransfer(const std::string& to, const uint256& id, const int32_t& amount, bool sendTo = false);
    bool safeBatchTransferFrom(const std::string& from, const std::string& to, const std::vector<uint256>& ids, const std::vector<int32_t>& amounts, bool sendTo = false);
    bool safeBatchTransfer(const std::string& to, const std::vector<uint256>& ids, const std::vector<int32_t>& amounts, bool sendTo = false);
    bool walletNFTList(WalletNFTInfo& result, const uint256& id, bool sendTo = false);

    // ABI Events
    bool transferEvents(std::vector<NftEvent>& nftEvents, int64_t fromBlock = 0, int64_t toBlock = -1, int64_t minconf = 0);

    // Static functions
    static bool ToHash160(const std::string& strQtumAddress, std::string& strHash160);
    static bool ToQtumAddress(const std::string& strHash160, std::string& strQtumAddress, bool isNullValid = true);
    static uint256 ToUint256(const std::string& data);
    static int32_t ToInt32(const std::string& data);
    static void addNftEvent(std::vector<NftEvent> &nftEvents, NftEvent nftEvent);

    // Get param functions
    static const char* paramAddress();
    static const char* paramDatahex();
    static const char* paramAmount();
    static const char* paramGasLimit();
    static const char* paramGasPrice();
    static const char* paramSender();
    static const char* paramBroadcast();
    static const char* paramChangeToSender();
    static const char* paramPsbt();

private:
    bool exec(const std::vector<std::string>& input, int func, std::vector<std::string>& output, bool sendTo);
    bool exec(const std::vector<std::vector<std::string>> &values, int func, std::vector<std::string> &output, bool sendTo);
    bool execEvents(int64_t fromBlock, int64_t toBlock, int64_t minconf, int func, std::vector<NftEvent> &nftEvents);

    QtumNft(QtumNft const&);
    QtumNft& operator=(QtumNft const&);

private:
    QtumNftData* d;
};

#endif // QTUMNFT_H
