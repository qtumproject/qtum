#ifndef QTUMNFT_H
#define QTUMNFT_H
#include <string>
#include <vector>
#include <map>
#include <uint256.h>

struct QtumNftData;

struct NftEvent{
    std::string address;
    std::string sender;
    std::string receiver;
    uint256 blockHash;
    uint64_t blockNumber;
    uint256 transactionHash;
    uint256 value;

    NftEvent()
    {
        SetNull();
    }

    void SetNull()
    {
        blockHash.SetNull();
        blockNumber = 0;
        transactionHash.SetNull();
        value.SetNull();
    }
};

class QtumNftExec
{
public:
    virtual bool execValid(const int& func, const bool& sendTo);
    virtual bool execEventsValid(const int& func, const int64_t& fromBlock);
    virtual bool exec(const bool& sendTo, const std::map<std::string, std::string>& lstParams, std::string& result, std::string& message);
    virtual bool execEvents(const int64_t& fromBlock, const int64_t& toBlock, const int64_t& minconf, const std::string& eventName, const std::string& contractAddress, const std::string& senderAddress, const int& numTopics, std::vector<NftEvent>& result);
    virtual bool privateKeysDisabled();
    virtual ~QtumNftExec();
};

class QtumNft
{
public:
    QtumNft();
    virtual ~QtumNft();

    void setQtumNftExec(QtumNftExec* nftExec);

    // Set command data
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

    // Set transaction data
    void setTxId(const std::string& txid);

    // ABI Functions
    bool balanceOf(const std::string& account, const std::string& id, std::string& result, bool sendTo = false);
    bool balanceOf(std::string& result, const std::string& id, bool sendTo = false);
    bool createNFT(const std::string& owner, const std::string& name, const std::string& url, const std::string& desc, const std::string& count, bool sendTo = false);
    bool createNFT(const std::string& name, const std::string& url, const std::string& desc, const std::string& count, bool sendTo = false);
    bool isApprovedForAll(const std::string& account, const std::string& operant, bool& success, bool sendTo = false);
    bool safeTransferFrom(const std::string& from, const std::string& to, const std::string& id, const std::string& amount, bool sendTo = false);

    // ABI Events
    bool transferEvents(std::vector<NftEvent>& nftEvents, int64_t fromBlock = 0, int64_t toBlock = -1, int64_t minconf = 0);
    bool burnEvents(std::vector<NftEvent>& nftEvents, int64_t fromBlock = 0, int64_t toBlock = -1, int64_t minconf = 0);

    // Static functions
    static bool ToHash160(const std::string& strQtumAddress, std::string& strHash160);
    static bool ToQtumAddress(const std::string& strHash160, std::string& strQtumAddress);
    static uint256 ToUint256(const std::string& data);
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

protected:
    void setAddress(const std::string &address);

private:
    bool exec(const std::vector<std::string>& input, int func, std::vector<std::string>& output, bool sendTo);
    bool execEvents(int64_t fromBlock, int64_t toBlock, int64_t minconf, int func, std::vector<NftEvent> &nftEvents);

    QtumNft(QtumNft const&);
    QtumNft& operator=(QtumNft const&);

private:
    QtumNftData* d;
};

#endif // QTUMNFT_H
