// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_MINING_H
#define BITCOIN_RPC_MINING_H

class UniValue;
class JSONRPCRequest;
class ChainstateManager;

/** Default max iterations to try in RPC generatetodescriptor, generatetoaddress, and generateblock. */
static const uint64_t DEFAULT_MAX_TRIES{1000000};

UniValue GetReqNetworkHashPS(const JSONRPCRequest& request, ChainstateManager& chainman);

#endif // BITCOIN_RPC_MINING_H
