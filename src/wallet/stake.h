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

} // namespace wallet

#endif // QTUM_WALLET_STAKE_H
