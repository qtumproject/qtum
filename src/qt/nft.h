#ifndef NFT_H
#define NFT_H
#include <string>
#include <vector>
#include <map>
#include <uint256.h>
#include <qtum/qtumnft.h>

struct NftData;
class WalletModel;

class Nft : public QtumNft
{
public:
    void setModel(WalletModel *model){};
    bool transfer(const std::string& _to, const std::string& _value, bool& success, bool sendTo = false){return false;};
    bool name(std::string& result, bool sendTo = false){return false;};
};

#endif // NFT_H
