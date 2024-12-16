// Copyright (c) 2012-2013 The PPCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef QUANTUM_POS_H
#define QUANTUM_POS_H

#include <chain.h>
#include <primitives/transaction.h>
#include <consensus/validation.h>
#include <txdb.h>
#include <validation.h>
#include <arith_uint256.h>
#include <hash.h>
#include <chainparams.h>
#include <script/sign.h>
#include <consensus/consensus.h>
#include <qtum/posutils.h>

void CacheKernel(std::map<COutPoint, CStakeCache>& cache, const COutPoint& prevout, CBlockIndex* pindexPrev, CCoinsViewCache& view);

// Compute the hash modifier for proof-of-stake
uint256 ComputeStakeModifier(const CBlockIndex* pindexPrev, const uint256& kernel);

// Check whether stake kernel meets hash target
// Sets hashProofOfStake on success return
bool CheckStakeKernelHash(CBlockIndex* pindexPrev, unsigned int nBits, uint32_t blockFromTime, CAmount prevoutAmount, const COutPoint& prevout, unsigned int nTimeTx, uint256& hashProofOfStake, uint256& targetProofOfStake, bool fPrintProofOfStake=false);

// Check kernel hash target and coinstake signature
// Sets hashProofOfStake on success return
bool CheckProofOfStake(CBlockIndex* pindexPrev, BlockValidationState& state, const CTransaction& tx, unsigned int nBits, uint32_t nTimeBlock, const std::vector<unsigned char>& vchPoD, const COutPoint& headerPrevout, uint256& hashProofOfStake, uint256& targetProofOfStake, CCoinsViewCache& view, Chainstate& chainstate);

// Check whether the coinstake timestamp meets protocol
inline bool CheckCoinStakeTimestamp(uint32_t nTimeBlock, int nHeight, const Consensus::Params &consensusParams)
{
    return (nTimeBlock & consensusParams.StakeTimestampMask(nHeight)) == 0;
}

// Should be called in ConnectBlock to make sure that the input pubkey == output pubkey
// Since it is only used in ConnectBlock, we know that we have access to the full contextual utxo set
bool CheckBlockInputPubKeyMatchesOutputPubKey(const CBlock& block, CCoinsViewCache& view, bool delegateOutputExist);

// Recover the pubkey and check that it matches the prevoutStake's scriptPubKey.
bool CheckRecoveredPubKeyFromBlockSignature(CBlockIndex* pindexPrev, const CBlockHeader& block, CCoinsViewCache& view, Chainstate& chainstate);

// Wrapper around CheckStakeKernelHash()
// Also checks existence of kernel input and min age
// Convenient for searching a kernel
bool CheckKernel(CBlockIndex* pindexPrev, unsigned int nBits, uint32_t nTimeBlock, const COutPoint& prevout, CCoinsViewCache& view, Chainstate& chainstate);
bool CheckKernel(CBlockIndex* pindexPrev, unsigned int nBits, uint32_t nTimeBlock, const COutPoint& prevout, CCoinsViewCache& view, const std::map<COutPoint, CStakeCache>& cache, Chainstate& chainstate);
bool CheckKernelCache(CBlockIndex* pindexPrev, unsigned int nBits, uint32_t nTimeBlock, const COutPoint& prevout, const std::map<COutPoint, CStakeCache>& cache, uint256& hashProofOfStake);

unsigned int GetStakeMaxCombineInputs();

int64_t GetStakeCombineThreshold();

bool SplitOfflineStakeReward(const int64_t& nReward, const uint8_t& fee, int64_t& nRewardOffline, int64_t& nRewardStaker);

bool IsDelegateOutputExist(int inFee);

int GetDelegationFeeTx(const CTransaction& tx, const Coin& coin, bool delegateOutputExist);

bool GetDelegationFeeFromContract(const uint160& address, uint8_t& fee, Chainstate& chainstate);

unsigned int GetStakeSplitOutputs();

int64_t GetStakeSplitThreshold();

bool GetMPoSOutputs(std::vector<CTxOut>& mposOutputList, int64_t nRewardPiece, int nHeight, const Consensus::Params& consensusParams, CChain& chain, node::BlockManager& blockman);

bool CreateMPoSOutputs(CMutableTransaction& txNew, int64_t nRewardPiece, int nHeight, const Consensus::Params& consensusParams, CChain& chain, node::BlockManager& blockman);

#endif // QUANTUM_POS_H
