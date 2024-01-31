#ifndef QTUM_WALLET_RPC_MINING_H
#define QTUM_WALLET_RPC_MINING_H

#include <span.h>

class CRPCCommand;

namespace wallet {
Span<const CRPCCommand> GetMiningRPCCommands();
} // namespace wallet

#endif // QTUM_WALLET_RPC_MINING_H
