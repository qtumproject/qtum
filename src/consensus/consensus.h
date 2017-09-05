// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_CONSENSUS_H
#define BITCOIN_CONSENSUS_CONSENSUS_H

#include <stdint.h>

/** The maximum allowed size for a serialized block, in bytes (only for buffer size limits) */
extern unsigned int dgpMaxBlockSerSize;
/** The maximum allowed weight for a block, see BIP 141 (network rule) */
extern unsigned int dgpMaxBlockWeight;
/** The maximum allowed size for a block excluding witness data, in bytes (network rule) */
extern unsigned int dgpMaxBlockBaseSize;

extern unsigned int dgpMaxBlockSize; // qtum

/** The maximum allowed number of signature check operations in a block (network rule) */
extern int64_t dgpMaxBlockSigOps;

extern unsigned int dgpMaxProtoMsgLength;

extern unsigned int dgpMaxTxSigOps;

/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 500; //qtum: change to 500 for prod

static const int MAX_TRANSACTION_BASE_SIZE = 1000000;

/** Flags for nSequence and nLockTime locks */
enum {
    /* Interpret sequence numbers as relative lock-time constraints. */
    LOCKTIME_VERIFY_SEQUENCE = (1 << 0),

    /* Use GetMedianTimePast() instead of nTime for end point timestamp. */
    LOCKTIME_MEDIAN_TIME_PAST = (1 << 1),
};

void updateBlockSizeParams(unsigned int newBlockSize);

#endif // BITCOIN_CONSENSUS_CONSENSUS_H
