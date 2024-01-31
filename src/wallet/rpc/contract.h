#ifndef QTUM_WALLET_RPC_CONTRACT_H
#define QTUM_WALLET_RPC_CONTRACT_H

#include <span.h>

class CRPCCommand;

namespace wallet {
Span<const CRPCCommand> GetContractRPCCommands();
} // namespace wallet

#endif // QTUM_WALLET_RPC_CONTRACT_H
