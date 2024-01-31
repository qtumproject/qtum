#ifndef QTUM_WALLET_STAKE_H
#define QTUM_WALLET_STAKE_H

#include <consensus/amount.h>
#include <wallet/transaction.h>
#include <wallet/wallet.h>

namespace wallet {
/* Start staking qtums */
void StartStake(CWallet& wallet);

/* Stop staking qtums */
void StopStake(CWallet& wallet);

/* Create coin stake */
bool CreateCoinStake(CWallet& wallet, unsigned int nBits, const CAmount& nTotalFees, uint32_t nTimeBlock, CMutableTransaction& tx, PKHash& pkhash, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoins, std::vector<COutPoint>& setSelectedCoins, std::vector<COutPoint>& setDelegateCoins, bool selectedOnly, bool sign, std::vector<unsigned char>& vchPoD, COutPoint& headerPrevout);

//! select coins for staking from the available coins for staking.
bool SelectCoinsForStaking(const CWallet& wallet, CAmount& nTargetValue, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet);

//! select delegated coins for staking from other users.
bool SelectDelegateCoinsForStaking(const CWallet& wallet, std::vector<COutPoint>& setDelegateCoinsRet, std::map<uint160, CAmount>& mDelegateWeight);

//! select list of address with coins.
void SelectAddress(const CWallet& wallet, std::map<uint160, bool>& mapAddress);

//! update miner stake cache.
void UpdateMinerStakeCache(CWallet& wallet, bool fStakeCache, const std::vector<COutPoint>& prevouts, CBlockIndex* pindexPrev);

//! get stake weight.
uint64_t GetStakeWeight(const CWallet& wallet, uint64_t* pStakerWeight = nullptr, uint64_t* pDelegateWeight = nullptr);

} // namespace wallet

#endif // QTUM_WALLET_STAKE_H
