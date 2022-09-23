#ifndef QTUM_WALLET_RPC_ADDRESSES_H
#define QTUM_WALLET_RPC_ADDRESSES_H

#include <script/standard.h>
#include <univalue.h>

namespace wallet {
class CWallet;

UniValue DescribeWalletAddress(const CWallet& wallet, const CTxDestination& dest);
} //  namespace wallet

#endif // QTUM_WALLET_RPC_ADDRESSES_H
