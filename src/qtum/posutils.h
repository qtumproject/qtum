#ifndef POSUTILS_H
#define POSUTILS_H

#include <uint256.h>
#include <consensus/amount.h>

struct CStakeCache{
    CStakeCache(uint32_t blockFromTime_, CAmount amount_) : blockFromTime(blockFromTime_), amount(amount_){
    }
    uint32_t blockFromTime;
    CAmount amount;
};

struct Delegation
{
    Delegation():
        fee(0),
        blockHeight(0) {}

    bool IsNull() const
    {
        return staker == uint160() &&
                fee == 0 &&
                blockHeight == 0 &&
                PoD == std::vector<unsigned char>();
    }

    uint160 staker;
    uint8_t fee;
    uint32_t blockHeight;
    std::vector<unsigned char> PoD; //Proof Of Delegation
};

inline bool operator==(const Delegation& lhs, const Delegation& rhs)
{
    return lhs.staker == rhs.staker &&
           lhs.fee == rhs.fee &&
           lhs.blockHeight == rhs.blockHeight &&
           lhs.PoD == rhs.PoD;
}

inline bool operator!=(const Delegation& lhs, const Delegation& rhs)
{
    return !(lhs == rhs);
}

#endif
