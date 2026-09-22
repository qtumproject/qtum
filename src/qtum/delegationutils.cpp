#include <qtum/delegationutils.h>
#include <util/strencodings.h>
#include <pubkey.h>

namespace delegationutils
{
const size_t nPoDStartPosition = 131;
bool IsAddBytecode(const std::vector<unsigned char> &data)
{
    // Quick check for is set delegate address
    size_t size = data.size();
    if(size < 228)
        return false;
    if(data[0] != 76 || data[1] != 14 || data[2] != 150 || data[3] != 140 || data[nPoDStartPosition] != CPubKey::COMPACT_SIGNATURE_SIZE)
        return false;
    return true;
}

bool GetUnsignedStaker(const std::vector<unsigned char> &data, std::string &hexStaker)
{
    if(!IsAddBytecode(data))
        return false;

    // Init variables
    size_t from = nPoDStartPosition + 1;
    size_t to = from + CPubKey::COMPACT_SIGNATURE_SIZE;
    size_t stakerSize = 40;
    size_t remainSize = CPubKey::COMPACT_SIGNATURE_SIZE - stakerSize;
    std::string strStaker;
    std::string strRemain;

    strStaker.reserve(stakerSize);
    strRemain.reserve(remainSize);

    // Get unsigned staker address from PoD
    for(size_t i = from; i < to; i++)
    {
        char c = (char)data[i];
        if(strStaker.size() < stakerSize)
        {
            strStaker.push_back(c);
        }
        else
        {
            strRemain.push_back(c);
        }
    }

    // Check formatting
    if(IsHex(strStaker) && !IsHex(strRemain))
    {
        hexStaker = strStaker;
        return true;
    }

    return false;
}

bool SetSignedStaker(std::vector<unsigned char> &data, const std::string &base64PoD)
{
    if(!IsAddBytecode(data))
        return false;

    std::vector<unsigned char> strPoD;
    if(auto decodePoD = DecodeBase64(base64PoD))
    {
        strPoD = *decodePoD;
    }
    if(strPoD.size() < CPubKey::COMPACT_SIGNATURE_SIZE)
        return false;

    size_t offset = nPoDStartPosition + 1;
    for(size_t i = 0; i < CPubKey::COMPACT_SIGNATURE_SIZE; i++)
    {
        data[offset + i] = strPoD[i];
    }

    return true;
}
}
