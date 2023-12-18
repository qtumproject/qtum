// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAINPARAMS_H
#define BITCOIN_CHAINPARAMS_H

#include <kernel/chainparams.h>

#include <chainparamsbase.h>
#include <consensus/params.h>
#include <netaddress.h>
#include <primitives/block.h>
#include <protocol.h>
#include <util/hash_type.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * Creates and returns a std::unique_ptr<CChainParams> of the chosen chain.
 * @returns a CChainParams* of the chosen chain.
 * @throws a std::runtime_error if the chain is not supported.
 */
std::unique_ptr<const CChainParams> CreateChainParams(const ArgsManager& args, const std::string& chain);

/**
 * Return the currently selected parameters. This won't change after app
 * startup, except for unit tests.
 */
const CChainParams &Params();

/**
 * Sets the params returned by Params() to those for the given chain name.
 * @throws std::runtime_error when the chain is not supported.
 */
void SelectParams(const std::string& chain);

/**
 * Allows modifying the Op Sender block height regtest parameter.
 */
void UpdateOpSenderBlockHeight(int nHeight);

/**
 * Allows modifying the btc_ecrecover block height regtest parameter.
 */
void UpdateBtcEcrecoverBlockHeight(int nHeight);

/**
 * Allows modifying the constantinople block height regtest parameter.
 */
void UpdateConstantinopleBlockHeight(int nHeight);

/**
 * Allows modifying the difficulty change block height regtest parameter.
 */
void UpdateDifficultyChangeBlockHeight(int nHeight);

/**
 * Allows modifying the offline staking block height regtest parameter.
 */
void UpdateOfflineStakingBlockHeight(int nHeight);

/**
 * Allows modifying the delegations address regtest parameter.
 */
void UpdateDelegationsAddress(const uint160& address);

/**
 * @brief UpdateLastMPoSBlockHeight Last mpos block height
 * @param nHeight Block height
 */
void UpdateLastMPoSBlockHeight(int nHeight);

/**
 * Allows modifying the reduce block time height regtest parameter.
 */
void UpdateReduceBlocktimeHeight(int nHeight);

/**
 * Allows modifying the pow allow for min difficulty blocks regtest parameter.
 */
void UpdatePowAllowMinDifficultyBlocks(bool fValue);

/**
 * Allows modifying the pow no retargeting regtest parameter.
 */
void UpdatePowNoRetargeting(bool fValue);

/**
 * Allows modifying the pos no retargeting regtest parameter.
 */
void UpdatePoSNoRetargeting(bool fValue);

/**
 * Allows modifying the muir glacier block height regtest parameter.
 */
void UpdateMuirGlacierHeight(int nHeight);

/**
 * Allows modifying the london block height regtest parameter.
 */
void UpdateLondonHeight(int nHeight);

/**
 * Allows modifying the taproot block height regtest parameter.
 */
void UpdateTaprootHeight(int nHeight);

/**
 * Allows modifying the shanghai block height regtest parameter.
 */
void UpdateShanghaiHeight(int nHeight);

#endif // BITCOIN_CHAINPARAMS_H
