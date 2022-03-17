#ifndef NFT_H
#define NFT_H
#include <string>
#include <vector>
#include <map>
#include <uint256.h>
#include <qtum/qtumnft.h>

struct NftData;
class WalletModel;

class Nft : public QtumNftExec, public QtumNft
{
public:
    Nft();
    ~Nft();

    void setModel(WalletModel *model);

    bool execValid(const int& func, const bool& sendTo) override;
    bool execEventsValid(const int& func, const int64_t& fromBlock) override;
    bool exec(const bool& sendTo, const std::map<std::string, std::string>& lstParams, std::string& result, std::string& message) override;
    bool execEvents(const int64_t& fromBlock, const int64_t& toBlock, const int64_t& minconf, const std::string& eventName, const std::string& contractAddress, const int& numTopics, std::vector<NftEvent>& result) override;
    bool privateKeysDisabled() override;

private:
    NftData* d;
};

#endif // NFT_H
