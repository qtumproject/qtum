#ifndef QTUMDELEGATION_H
#define QTUMDELEGATION_H
#include <string>
#include <vector>
#include <stdint.h>
#include <uint256.h>

extern const std::string strDelegationsABI;
class QtumDelegationPriv;

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
    uint blockHeight;
    std::vector<unsigned char> PoD; //Proof Of Delegation
};

/**
 * @brief The QtumDelegation class Communicate with the qtum delegation contract
 */
class QtumDelegation {
    
public:
    /**
     * @brief QtumDelegation Constructor
     */
    QtumDelegation();

    /**
     * @brief ~QtumDelegation Destructor
     */
    virtual ~QtumDelegation();

    /**
     * @brief GetDelegation Get delegation for an address
     * @param address Public key hash address
     * @param delegation Delegation information for an address
     * @return true/false
     */
    bool GetDelegation(const uint160& address, Delegation& delegation) const;

    /**
     * @brief VerifyDelegation Verify delegation for an address
     * @param address Public key hash address
     * @param delegation Delegation information for an address
     * @return true/false
     */
    static bool VerifyDelegation(const uint160& address, const Delegation& delegation);

private:
    QtumDelegation(const QtumDelegation&);
    QtumDelegation& operator=(const QtumDelegation&);
    QtumDelegationPriv* priv;
};
#endif
