#ifndef TOKEN_H
#define TOKEN_H
#include <string>
#include <vector>
#include <map>
#include <uint256.h>
#include <qtum/qtumtoken.h>

struct TokenData;
class WalletModel;

class Token : public QtumTokenExec, public QtumToken
{
public:
    Token();
    ~Token();

    void setModel(WalletModel *model);

    bool execValid(const int& func, const bool& sendTo);
    bool execEventsValid(const int& func, const int64_t& fromBlock);
    bool exec(const bool& sendTo, const std::map<std::string, std::string>& lstParams, std::string& result, std::string& message);
    bool execEvents(const int64_t& fromBlock, const int64_t& toBlock, const int64_t& minconf, const std::string& eventName, const std::string& contractAddress, const std::string& senderAddress, const int& numTopics, std::vector<TokenEvent>& result);

private:
    TokenData* d;
};

#endif // TOKEN_H
