#ifndef QTUMTOKEN_H
#define QTUMTOKEN_H
#include <string>
#include <vector>
#include <map>
#include <uint256.h>

struct QtumTokenData;

struct TokenEvent{
    std::string address;
    std::string sender;
    std::string receiver;
    uint256 blockHash;
    uint64_t blockNumber;
    uint256 transactionHash;
    uint256 value;

    TokenEvent()
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

class QtumTokenExec
{
public:
    virtual bool execValid(const int& func, const bool& sendTo);
    virtual bool execEventsValid(const int& func, const int64_t& fromBlock);
    virtual bool exec(const bool& sendTo, const std::map<std::string, std::string>& lstParams, std::string& result, std::string& message);
    virtual bool execEvents(const int64_t& fromBlock, const int64_t& toBlock, const int64_t& minconf, const std::string& eventName, const std::string& contractAddress, const std::string& senderAddress, const int& numTopics, std::vector<TokenEvent>& result);
    virtual bool privateKeysDisabled();
    virtual ~QtumTokenExec();
};

class QtumToken
{
public:
    QtumToken();
    virtual ~QtumToken();

    void setQtumTokenExec(QtumTokenExec* tokenExec);

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

    // Set transaction data
    void setTxId(const std::string& txid);

    // ABI Functions
    bool name(std::string& result, bool sendTo = false);
    bool approve(const std::string& _spender, const std::string& _value, bool& success, bool sendTo = false);
    bool totalSupply(std::string& result, bool sendTo = false);
    bool transferFrom(const std::string& _from, const std::string& _to, const std::string& _value, bool& success, bool sendTo = false);
    bool decimals(std::string& result, bool sendTo = false);
    bool decimals(uint32_t& result);
    bool burn(const std::string& _value, bool& success, bool sendTo = false);
    bool balanceOf(std::string& result, bool sendTo = false);
    bool balanceOf(const std::string& spender, std::string& result, bool sendTo = false);
    bool burnFrom(const std::string& _from, const std::string& _value, bool& success, bool sendTo = false);
    bool symbol(std::string& result, bool sendTo = false);
    bool transfer(const std::string& _to, const std::string& _value, bool& success, bool sendTo = false);
    bool approveAndCall(const std::string& _spender, const std::string& _value, const std::string& _extraData, bool& success, bool sendTo = false);
    bool allowance(const std::string& _from, const std::string& _to, std::string& result, bool sendTo = false);

    // ABI Events
    bool transferEvents(std::vector<TokenEvent>& tokenEvents, int64_t fromBlock = 0, int64_t toBlock = -1, int64_t minconf = 0);
    bool burnEvents(std::vector<TokenEvent>& tokenEvents, int64_t fromBlock = 0, int64_t toBlock = -1, int64_t minconf = 0);

    // Static functions
    static bool ToHash160(const std::string& strQtumAddress, std::string& strHash160);
    static bool ToQtumAddress(const std::string& strHash160, std::string& strQtumAddress);
    static uint256 ToUint256(const std::string& data);
    static void addTokenEvent(std::vector<TokenEvent> &tokenEvents, TokenEvent tokenEvent);

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
    bool execEvents(int64_t fromBlock, int64_t toBlock, int64_t minconf, int func, std::vector<TokenEvent> &tokenEvents);

    QtumToken(QtumToken const&);
    QtumToken& operator=(QtumToken const&);

private:
    QtumTokenData* d;
};

#endif // QTUMTOKEN_H
