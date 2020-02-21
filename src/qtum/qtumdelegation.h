#ifndef QTUMDELEGATION_H
#define QTUMDELEGATION_H
#include <string>
#include <vector>
#include <stdint.h>
#include <uint256.h>

class QtumDelegationPriv;
class ContractABI;

extern const std::string strDelegationsABI;
const ContractABI &DelegationABI();

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

struct DelegationItem : public Delegation
{
    DelegationItem()
    {}

    bool IsNull() const
    {
        return Delegation::IsNull() &&
                delegate == uint160();
    }

    uint160 delegate;
};

enum DelegationType
{
    DELEGATION_NONE = 0,
    DELEGATION_ADD = 1,
    DELEGATION_REMOVE = 2,
};

struct DelegationEvent
{
    DelegationItem item;
    DelegationType type;

    DelegationEvent():
        type(DelegationType::DELEGATION_NONE)
    {}
};

/**
 * @brief The IQtumStaker class Delegation filter
 */
class IDelegationFilter
{
public:
    virtual bool Match(const DelegationEvent& event) const = 0;
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

    /**
     * @brief FilterDelegationEvents Filter delegation events
     * @param events Output list of delegation events for the filter
     * @param filter Delegation filter
     * @param fromBlock Start with block
     * @param toBlock End with block, -1 mean all available
     * @param minconf Minimum confirmations
     * @return true/false
     */
    bool FilterDelegationEvents(std::vector<DelegationEvent>& events, const IDelegationFilter& filter, int fromBlock = 0, int toBlock = -1, int minconf = 0) const;

private:
    QtumDelegation(const QtumDelegation&);
    QtumDelegation& operator=(const QtumDelegation&);
    QtumDelegationPriv* priv;
};
#endif
