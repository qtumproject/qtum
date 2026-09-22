#ifndef QTUM_WALLET_RPC_MINING_H
#define QTUM_WALLET_RPC_MINING_H

#include <span.h>

class CRPCCommand;

namespace wallet {
std::span<const CRPCCommand> GetMiningRPCCommands();
} // namespace wallet

#endif // QTUM_WALLET_RPC_MINING_H
