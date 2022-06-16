#ifndef NFTCONFIG_H
#define NFTCONFIG_H
#include <uint256.h>

#include <string>
#include <vector>

class NftConfig
{
public:
    /**
     * @brief instance Get the nft config instance
     * @return
     */
    static const NftConfig &Instance();

    /**
     * @brief GetNftAddress Get nft contract address
     * @return Nft contract address
     */
    uint160 GetNftAddress() const;

    /**
     * @brief SetNftAddress Set nft contract address
     * @param value Nft contract address
     */
    void SetNftAddress(const uint160 &value);

    /**
     * @brief IsUrlValid Check if URL is valid
     * @param sUrl String URL
     * @return is valid
     */
    bool IsUrlValid(const std::string& sUrl) const;

    /**
     * @brief GetUriRegex Get uri regex
     * @return regex
     */
    std::string GetUriRegex() const;

protected:
    /**
     * @brief NftConfig Constructor
     */
    NftConfig();

    uint160 nftAddress;
    std::string urlRegex;
    unsigned int nUrlMaxLength;
};

/**
 * Allows modifying the nft address regtest parameter.
 */
void UpdateNftAddress(const uint160& address);

#endif // NFTCONFIG_H
