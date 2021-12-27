#ifndef CONTRACT_UTIL_H
#define CONTRACT_UTIL_H

#include <univalue.h>
#include <validation.h>
#include <qtum/qtumtoken.h>

class ChainstateManager;

UniValue CallToContract(const UniValue& params, ChainstateManager &chainman);

UniValue SearchLogs(const UniValue& params, ChainstateManager &chainman);

void assignJSON(UniValue& entry, const TransactionReceiptInfo& resExec);

void assignJSON(UniValue& logEntry, const dev::eth::LogEntry& log,
        bool includeAddress);

void transactionReceiptInfoToJSON(const TransactionReceiptInfo& resExec, UniValue& entry);

size_t parseUInt(const UniValue& val, size_t defaultVal);

int parseBlockHeight(const UniValue& val, int defaultVal);

void parseParam(const UniValue& val, std::set<dev::h160> &h160s);

void parseParam(const UniValue& val, std::vector<boost::optional<dev::h256>> &h256s);

/**
 * @brief The CallToken class Read available token data
 */
class CallToken : public QtumTokenExec, public QtumToken
{
public:
    CallToken(ChainstateManager &_chainman);

    bool execValid(const int& func, const bool& sendTo) override;

    bool execEventsValid(const int &func, const int64_t &fromBlock) override;

    bool exec(const bool& sendTo, const std::map<std::string, std::string>& lstParams, std::string& result, std::string&) override;

    bool execEvents(const int64_t &fromBlock, const int64_t &toBlock, const int64_t &minconf, const std::string &eventName, const std::string &contractAddress, const std::string &senderAddress, const int &numTopics, std::vector<TokenEvent> &result) override;

    bool searchTokenTx(const int64_t &fromBlock, const int64_t &toBlock, const int64_t &minconf, const std::string &eventName, const std::string &contractAddress, const std::string &senderAddress, const int &numTopics, UniValue& resultVar);

    void setCheckGasForCall(bool value);

protected:
    ChainstateManager &chainman;

private:
    bool checkGasForCall = false;
};

#endif // CONTRACT_UTIL_H
