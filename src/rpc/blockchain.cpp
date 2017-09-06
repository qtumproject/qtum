// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "amount.h"
#include "base58.h"
#include "chain.h"
#include "chainparams.h"
#include "checkpoints.h"
#include "coins.h"
#include "consensus/validation.h"
#include "validation.h"
#include "policy/policy.h"
#include "primitives/transaction.h"
#include "rpc/server.h"
#include "streams.h"
#include "sync.h"
#include "txmempool.h"
#include "util.h"
#include "utilstrencodings.h"
#include "hash.h"
#include "libdevcore/CommonData.h"
#include "pos.h"
#include "txdb.h"

#include <stdint.h>

#include <univalue.h>

#include <boost/thread/thread.hpp> // boost::thread::interrupt

#include <mutex>
#include <condition_variable>
using namespace std;

struct CUpdatedBlock
{
    uint256 hash;
    int height;
};

static std::mutex cs_blockchange;
static std::condition_variable cond_blockchange;
static CUpdatedBlock latestblock;

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, UniValue& entry);
void ScriptPubKeyToJSON(const CScript& scriptPubKey, UniValue& out, bool fIncludeHex);

double GetDifficulty(const CBlockIndex* blockindex)
{
    // Floating point number that is a multiple of the minimum difficulty,
    // minimum difficulty = 1.0.
    if (blockindex == NULL)
    {
        if (chainActive.Tip() == NULL)
            return 1.0;
        else
            blockindex = chainActive.Tip();
    }

    int nShift = (blockindex->nBits >> 24) & 0xff;

    double dDiff =
        (double)0x0000ffff / (double)(blockindex->nBits & 0x00ffffff);

    while (nShift < 29)
    {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29)
    {
        dDiff /= 256.0;
        nShift--;
    }

    return dDiff;
}

double GetPoWMHashPS()
{
    if (pindexBestHeader->nHeight >= Params().GetConsensus().nLastPOWBlock)//toniqtum edit
        return 0;

    int nPoWInterval = 72;
    int64_t nTargetSpacingWorkMin = 30, nTargetSpacingWork = 30;

    CBlockIndex* pindexGenesisBlock = chainActive.Genesis();
    CBlockIndex* pindex = pindexGenesisBlock;
    CBlockIndex* pindexPrevWork = pindexGenesisBlock;

    while (pindex)
    {
        if (pindex->IsProofOfWork())
        {
            int64_t nActualSpacingWork = pindex->GetBlockTime() - pindexPrevWork->GetBlockTime();
            nTargetSpacingWork = ((nPoWInterval - 1) * nTargetSpacingWork + nActualSpacingWork + nActualSpacingWork) / (nPoWInterval + 1);
            nTargetSpacingWork = max(nTargetSpacingWork, nTargetSpacingWorkMin);
            pindexPrevWork = pindex;
        }

        pindex = pindex->pnext;
    }

    return GetDifficulty() * 4294.967296 / nTargetSpacingWork;
}

double GetPoSKernelPS()
{
    int nPoSInterval = 72;
    double dStakeKernelsTriedAvg = 0;
    int nStakesHandled = 0, nStakesTime = 0;

    CBlockIndex* pindex = pindexBestHeader;
    CBlockIndex* pindexPrevStake = NULL;

    while (pindex && nStakesHandled < nPoSInterval)
    {
        if (pindex->IsProofOfStake())
        {
            if (pindexPrevStake)
            {
                dStakeKernelsTriedAvg += GetDifficulty(pindexPrevStake) * 4294967296.0;
                nStakesTime += pindexPrevStake->nTime - pindex->nTime;
                nStakesHandled++;
            }
            pindexPrevStake = pindex;
        }

        pindex = pindex->pprev;
    }

    double result = 0;

    if (nStakesTime)
        result = dStakeKernelsTriedAvg / nStakesTime;
    
    result *= STAKE_TIMESTAMP_MASK + 1;

    return result;
}

UniValue blockheaderToJSON(const CBlockIndex* blockindex)
{
    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("hash", blockindex->GetBlockHash().GetHex()));
    int confirmations = -1;
    // Only report confirmations if the block is on the main chain
    if (chainActive.Contains(blockindex))
        confirmations = chainActive.Height() - blockindex->nHeight + 1;
    result.push_back(Pair("confirmations", confirmations));
    result.push_back(Pair("height", blockindex->nHeight));
    result.push_back(Pair("version", blockindex->nVersion));
    result.push_back(Pair("versionHex", strprintf("%08x", blockindex->nVersion)));
    result.push_back(Pair("merkleroot", blockindex->hashMerkleRoot.GetHex()));
    result.push_back(Pair("time", (int64_t)blockindex->nTime));
    result.push_back(Pair("mediantime", (int64_t)blockindex->GetMedianTimePast()));
    result.push_back(Pair("nonce", (uint64_t)blockindex->nNonce));
    result.push_back(Pair("bits", strprintf("%08x", blockindex->nBits)));
    result.push_back(Pair("difficulty", GetDifficulty(blockindex)));
    result.push_back(Pair("chainwork", blockindex->nChainWork.GetHex()));
    result.push_back(Pair("hashStateRoot", blockindex->hashStateRoot.GetHex())); // qtum
    result.push_back(Pair("hashUTXORoot", blockindex->hashUTXORoot.GetHex())); // qtum

    if (blockindex->pprev)
        result.push_back(Pair("previousblockhash", blockindex->pprev->GetBlockHash().GetHex()));
    CBlockIndex *pnext = chainActive.Next(blockindex);
    if (pnext)
        result.push_back(Pair("nextblockhash", pnext->GetBlockHash().GetHex()));
		
    result.push_back(Pair("flags", strprintf("%s", blockindex->IsProofOfStake()? "proof-of-stake" : "proof-of-work")));
    result.push_back(Pair("proofhash", blockindex->hashProof.GetHex()));
    result.push_back(Pair("modifier", blockindex->nStakeModifier.GetHex()));

    return result;
}

UniValue blockToJSON(const CBlock& block, const CBlockIndex* blockindex, bool txDetails = false)
{
    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("hash", blockindex->GetBlockHash().GetHex()));
    int confirmations = -1;
    // Only report confirmations if the block is on the main chain
    if (chainActive.Contains(blockindex))
        confirmations = chainActive.Height() - blockindex->nHeight + 1;
    result.push_back(Pair("confirmations", confirmations));
    result.push_back(Pair("strippedsize", (int)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS)));
    result.push_back(Pair("size", (int)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION)));
    result.push_back(Pair("weight", (int)::GetBlockWeight(block)));
    result.push_back(Pair("height", blockindex->nHeight));
    result.push_back(Pair("version", block.nVersion));
    result.push_back(Pair("versionHex", strprintf("%08x", block.nVersion)));
    result.push_back(Pair("merkleroot", block.hashMerkleRoot.GetHex()));
    result.push_back(Pair("hashStateRoot", block.hashStateRoot.GetHex())); // qtum
    result.push_back(Pair("hashUTXORoot", block.hashUTXORoot.GetHex())); // qtum
    UniValue txs(UniValue::VARR);
    for(const auto& tx : block.vtx)
    {
        if(txDetails)
        {
            UniValue objTx(UniValue::VOBJ);
            TxToJSON(*tx, uint256(), objTx);
            txs.push_back(objTx);
        }
        else
            txs.push_back(tx->GetHash().GetHex());
    }
    result.push_back(Pair("tx", txs));
    result.push_back(Pair("time", block.GetBlockTime()));
    result.push_back(Pair("mediantime", (int64_t)blockindex->GetMedianTimePast()));
    result.push_back(Pair("nonce", (uint64_t)block.nNonce));
    result.push_back(Pair("bits", strprintf("%08x", block.nBits)));
    result.push_back(Pair("difficulty", GetDifficulty(blockindex)));
    result.push_back(Pair("chainwork", blockindex->nChainWork.GetHex()));

    if (blockindex->pprev)
        result.push_back(Pair("previousblockhash", blockindex->pprev->GetBlockHash().GetHex()));
    CBlockIndex *pnext = chainActive.Next(blockindex);
    if (pnext)
        result.push_back(Pair("nextblockhash", pnext->GetBlockHash().GetHex()));

    result.push_back(Pair("flags", strprintf("%s", blockindex->IsProofOfStake()? "proof-of-stake" : "proof-of-work")));
    result.push_back(Pair("proofhash", blockindex->hashProof.GetHex()));
    result.push_back(Pair("modifier", blockindex->nStakeModifier.GetHex()));

    if (block.IsProofOfStake())
        result.push_back(Pair("signature", HexStr(block.vchBlockSig.begin(), block.vchBlockSig.end())));	

    return result;
}

//////////////////////////////////////////////////////////////////////////// // qtum
UniValue executionResultToJSON(const dev::eth::ExecutionResult& exRes)
{
    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("gasUsed", CAmount(exRes.gasUsed)));
    std::stringstream ss;
    ss << exRes.excepted;
    result.push_back(Pair("excepted", ss.str()));
    result.push_back(Pair("newAddress", exRes.newAddress.hex()));
    result.push_back(Pair("output", HexStr(exRes.output)));
    result.push_back(Pair("codeDeposit", static_cast<int32_t>(exRes.codeDeposit)));
    result.push_back(Pair("gasRefunded", CAmount(exRes.gasRefunded)));
    result.push_back(Pair("depositSize", static_cast<int32_t>(exRes.depositSize)));
    result.push_back(Pair("gasForDeposit", CAmount(exRes.gasForDeposit)));
    return result;
}

UniValue transactionReceiptToJSON(const dev::eth::TransactionReceipt& txRec)
{
    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("stateRoot", txRec.stateRoot().hex()));
    result.push_back(Pair("gasUsed", CAmount(txRec.gasUsed())));
    result.push_back(Pair("bloom", txRec.bloom().hex()));
    UniValue logEntries(UniValue::VARR);
    dev::eth::LogEntries logs = txRec.log();
    for(dev::eth::LogEntry log : logs){
        UniValue logEntrie(UniValue::VOBJ);
        logEntrie.push_back(Pair("address", log.address.hex()));
        UniValue topics(UniValue::VARR);
        for(dev::h256 l : log.topics){
            topics.push_back(l.hex());
        }
        logEntrie.push_back(Pair("topics", topics));
        logEntrie.push_back(Pair("data", HexStr(log.data)));
        logEntries.push_back(logEntrie);
    }
    result.push_back(Pair("log", logEntries));
    return result;
}
////////////////////////////////////////////////////////////////////////////

UniValue getblockcount(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0)
        throw runtime_error(
            "getblockcount\n"
            "\nReturns the number of blocks in the longest blockchain.\n"
            "\nResult:\n"
            "n    (numeric) The current block count\n"
            "\nExamples:\n"
            + HelpExampleCli("getblockcount", "")
            + HelpExampleRpc("getblockcount", "")
        );

    LOCK(cs_main);
    return chainActive.Height();
}

UniValue getbestblockhash(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0)
        throw runtime_error(
            "getbestblockhash\n"
            "\nReturns the hash of the best (tip) block in the longest blockchain.\n"
            "\nResult:\n"
            "\"hex\"      (string) the block hash hex encoded\n"
            "\nExamples:\n"
            + HelpExampleCli("getbestblockhash", "")
            + HelpExampleRpc("getbestblockhash", "")
        );

    LOCK(cs_main);
    return chainActive.Tip()->GetBlockHash().GetHex();
}

void RPCNotifyBlockChange(bool ibd, const CBlockIndex * pindex)
{
    if(pindex) {
        std::lock_guard<std::mutex> lock(cs_blockchange);
        latestblock.hash = pindex->GetBlockHash();
        latestblock.height = pindex->nHeight;
    }
	cond_blockchange.notify_all();
}

UniValue waitfornewblock(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 1)
        throw runtime_error(
            "waitfornewblock (timeout)\n"
            "\nWaits for a specific new block and returns useful info about it.\n"
            "\nReturns the current block on timeout or exit.\n"
            "\nArguments:\n"
            "1. timeout (int, optional, default=0) Time in milliseconds to wait for a response. 0 indicates no timeout.\n"
            "\nResult:\n"
            "{                           (json object)\n"
            "  \"hash\" : {       (string) The blockhash\n"
            "  \"height\" : {     (int) Block height\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("waitfornewblock", "1000")
            + HelpExampleRpc("waitfornewblock", "1000")
        );
    int timeout = 0;
    if (request.params.size() > 0)
        timeout = request.params[0].get_int();

    CUpdatedBlock block;
    {
        std::unique_lock<std::mutex> lock(cs_blockchange);
        block = latestblock;
        if(timeout)
            cond_blockchange.wait_for(lock, std::chrono::milliseconds(timeout), [&block]{return latestblock.height != block.height || latestblock.hash != block.hash || !IsRPCRunning(); });
        else
            cond_blockchange.wait(lock, [&block]{return latestblock.height != block.height || latestblock.hash != block.hash || !IsRPCRunning(); });
        block = latestblock;
    }
    UniValue ret(UniValue::VOBJ);
    ret.push_back(Pair("hash", block.hash.GetHex()));
    ret.push_back(Pair("height", block.height));
    return ret;
}

UniValue waitforblock(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw runtime_error(
            "waitforblock <blockhash> (timeout)\n"
            "\nWaits for a specific new block and returns useful info about it.\n"
            "\nReturns the current block on timeout or exit.\n"
            "\nArguments:\n"
            "1. \"blockhash\" (required, string) Block hash to wait for.\n"
            "2. timeout       (int, optional, default=0) Time in milliseconds to wait for a response. 0 indicates no timeout.\n"
            "\nResult:\n"
            "{                           (json object)\n"
            "  \"hash\" : {       (string) The blockhash\n"
            "  \"height\" : {     (int) Block height\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("waitforblock", "\"0000000000079f8ef3d2c688c244eb7a4570b24c9ed7b4a8c619eb02596f8862\", 1000")
            + HelpExampleRpc("waitforblock", "\"0000000000079f8ef3d2c688c244eb7a4570b24c9ed7b4a8c619eb02596f8862\", 1000")
        );
    int timeout = 0;

    uint256 hash = uint256S(request.params[0].get_str());

    if (request.params.size() > 1)
        timeout = request.params[1].get_int();

    CUpdatedBlock block;
    {
        std::unique_lock<std::mutex> lock(cs_blockchange);
        if(timeout)
            cond_blockchange.wait_for(lock, std::chrono::milliseconds(timeout), [&hash]{return latestblock.hash == hash || !IsRPCRunning();});
        else
            cond_blockchange.wait(lock, [&hash]{return latestblock.hash == hash || !IsRPCRunning(); });
        block = latestblock;
    }

    UniValue ret(UniValue::VOBJ);
    ret.push_back(Pair("hash", block.hash.GetHex()));
    ret.push_back(Pair("height", block.height));
    return ret;
}

UniValue waitforblockheight(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw runtime_error(
            "waitforblockheight <height> (timeout)\n"
            "\nWaits for (at least) block height and returns the height and hash\n"
            "of the current tip.\n"
            "\nReturns the current block on timeout or exit.\n"
            "\nArguments:\n"
            "1. height  (required, int) Block height to wait for (int)\n"
            "2. timeout (int, optional, default=0) Time in milliseconds to wait for a response. 0 indicates no timeout.\n"
            "\nResult:\n"
            "{                           (json object)\n"
            "  \"hash\" : {       (string) The blockhash\n"
            "  \"height\" : {     (int) Block height\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("waitforblockheight", "\"100\", 1000")
            + HelpExampleRpc("waitforblockheight", "\"100\", 1000")
        );
    int timeout = 0;

    int height = request.params[0].get_int();

    if (request.params.size() > 1)
        timeout = request.params[1].get_int();

    CUpdatedBlock block;
    {
        std::unique_lock<std::mutex> lock(cs_blockchange);
        if(timeout)
            cond_blockchange.wait_for(lock, std::chrono::milliseconds(timeout), [&height]{return latestblock.height >= height || !IsRPCRunning();});
        else
            cond_blockchange.wait(lock, [&height]{return latestblock.height >= height || !IsRPCRunning(); });
        block = latestblock;
    }
    UniValue ret(UniValue::VOBJ);
    ret.push_back(Pair("hash", block.hash.GetHex()));
    ret.push_back(Pair("height", block.height));
    return ret;
}

UniValue getdifficulty(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0)
        throw runtime_error(
            "getdifficulty\n"
            "\nReturns the proof-of-work difficulty as a multiple of the minimum difficulty.\n"
            "\nReturns the proof-of-stake difficulty as a multiple of the minimum difficulty.\n"
            "\nResult:\n"
            "n.nnn       (numeric) the proof-of-work difficulty as a multiple of the minimum difficulty.\n"
            "\nExamples:\n"
            + HelpExampleCli("getdifficulty", "")
            + HelpExampleRpc("getdifficulty", "")
        );

    LOCK(cs_main);

    UniValue obj(UniValue::VOBJ);
    obj.push_back(Pair("proof-of-work",        GetDifficulty(GetLastBlockIndex(pindexBestHeader, false))));
    obj.push_back(Pair("proof-of-stake",       GetDifficulty(GetLastBlockIndex(pindexBestHeader, true))));
    return obj;
}

std::string EntryDescriptionString()
{
    return "    \"size\" : n,             (numeric) virtual transaction size as defined in BIP 141. This is different from actual serialized size for witness transactions as witness data is discounted.\n"
           "    \"fee\" : n,              (numeric) transaction fee in " + CURRENCY_UNIT + "\n"
           "    \"modifiedfee\" : n,      (numeric) transaction fee with fee deltas used for mining priority\n"
           "    \"time\" : n,             (numeric) local time transaction entered pool in seconds since 1 Jan 1970 GMT\n"
           "    \"height\" : n,           (numeric) block height when transaction entered pool\n"
           "    \"startingpriority\" : n, (numeric) DEPRECATED. Priority when transaction entered pool\n"
           "    \"currentpriority\" : n,  (numeric) DEPRECATED. Transaction priority now\n"
           "    \"descendantcount\" : n,  (numeric) number of in-mempool descendant transactions (including this one)\n"
           "    \"descendantsize\" : n,   (numeric) virtual transaction size of in-mempool descendants (including this one)\n"
           "    \"descendantfees\" : n,   (numeric) modified fees (see above) of in-mempool descendants (including this one)\n"
           "    \"ancestorcount\" : n,    (numeric) number of in-mempool ancestor transactions (including this one)\n"
           "    \"ancestorsize\" : n,     (numeric) virtual transaction size of in-mempool ancestors (including this one)\n"
           "    \"ancestorfees\" : n,     (numeric) modified fees (see above) of in-mempool ancestors (including this one)\n"
           "    \"depends\" : [           (array) unconfirmed transactions used as inputs for this transaction\n"
           "        \"transactionid\",    (string) parent transaction id\n"
           "       ... ]\n";
}

void entryToJSON(UniValue &info, const CTxMemPoolEntry &e)
{
    AssertLockHeld(mempool.cs);

    info.push_back(Pair("size", (int)e.GetTxSize()));
    info.push_back(Pair("fee", ValueFromAmount(e.GetFee())));
    info.push_back(Pair("modifiedfee", ValueFromAmount(e.GetModifiedFee())));
    info.push_back(Pair("time", e.GetTime()));
    info.push_back(Pair("height", (int)e.GetHeight()));
    info.push_back(Pair("startingpriority", e.GetPriority(e.GetHeight())));
    info.push_back(Pair("currentpriority", e.GetPriority(chainActive.Height())));
    info.push_back(Pair("descendantcount", e.GetCountWithDescendants()));
    info.push_back(Pair("descendantsize", e.GetSizeWithDescendants()));
    info.push_back(Pair("descendantfees", e.GetModFeesWithDescendants()));
    info.push_back(Pair("ancestorcount", e.GetCountWithAncestors()));
    info.push_back(Pair("ancestorsize", e.GetSizeWithAncestors()));
    info.push_back(Pair("ancestorfees", e.GetModFeesWithAncestors()));
    const CTransaction& tx = e.GetTx();
    set<string> setDepends;
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        if (mempool.exists(txin.prevout.hash))
            setDepends.insert(txin.prevout.hash.ToString());
    }

    UniValue depends(UniValue::VARR);
    BOOST_FOREACH(const string& dep, setDepends)
    {
        depends.push_back(dep);
    }

    info.push_back(Pair("depends", depends));
}

UniValue mempoolToJSON(bool fVerbose = false)
{
    if (fVerbose)
    {
        LOCK(mempool.cs);
        UniValue o(UniValue::VOBJ);
        BOOST_FOREACH(const CTxMemPoolEntry& e, mempool.mapTx)
        {
            const uint256& hash = e.GetTx().GetHash();
            UniValue info(UniValue::VOBJ);
            entryToJSON(info, e);
            o.push_back(Pair(hash.ToString(), info));
        }
        return o;
    }
    else
    {
        vector<uint256> vtxid;
        mempool.queryHashes(vtxid);

        UniValue a(UniValue::VARR);
        BOOST_FOREACH(const uint256& hash, vtxid)
            a.push_back(hash.ToString());

        return a;
    }
}

UniValue getrawmempool(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 1)
        throw runtime_error(
            "getrawmempool ( verbose )\n"
            "\nReturns all transaction ids in memory pool as a json array of string transaction ids.\n"
            "\nArguments:\n"
            "1. verbose (boolean, optional, default=false) True for a json object, false for array of transaction ids\n"
            "\nResult: (for verbose = false):\n"
            "[                     (json array of string)\n"
            "  \"transactionid\"     (string) The transaction id\n"
            "  ,...\n"
            "]\n"
            "\nResult: (for verbose = true):\n"
            "{                           (json object)\n"
            "  \"transactionid\" : {       (json object)\n"
            + EntryDescriptionString()
            + "  }, ...\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getrawmempool", "true")
            + HelpExampleRpc("getrawmempool", "true")
        );

    bool fVerbose = false;
    if (request.params.size() > 0)
        fVerbose = request.params[0].get_bool();

    return mempoolToJSON(fVerbose);
}

UniValue getmempoolancestors(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2) {
        throw runtime_error(
            "getmempoolancestors txid (verbose)\n"
            "\nIf txid is in the mempool, returns all in-mempool ancestors.\n"
            "\nArguments:\n"
            "1. \"txid\"                 (string, required) The transaction id (must be in mempool)\n"
            "2. verbose                  (boolean, optional, default=false) True for a json object, false for array of transaction ids\n"
            "\nResult (for verbose=false):\n"
            "[                       (json array of strings)\n"
            "  \"transactionid\"           (string) The transaction id of an in-mempool ancestor transaction\n"
            "  ,...\n"
            "]\n"
            "\nResult (for verbose=true):\n"
            "{                           (json object)\n"
            "  \"transactionid\" : {       (json object)\n"
            + EntryDescriptionString()
            + "  }, ...\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getmempoolancestors", "\"mytxid\"")
            + HelpExampleRpc("getmempoolancestors", "\"mytxid\"")
            );
    }

    bool fVerbose = false;
    if (request.params.size() > 1)
        fVerbose = request.params[1].get_bool();

    uint256 hash = ParseHashV(request.params[0], "parameter 1");

    LOCK(mempool.cs);

    CTxMemPool::txiter it = mempool.mapTx.find(hash);
    if (it == mempool.mapTx.end()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not in mempool");
    }

    CTxMemPool::setEntries setAncestors;
    uint64_t noLimit = std::numeric_limits<uint64_t>::max();
    std::string dummy;
    mempool.CalculateMemPoolAncestors(*it, setAncestors, noLimit, noLimit, noLimit, noLimit, dummy, false);

    if (!fVerbose) {
        UniValue o(UniValue::VARR);
        BOOST_FOREACH(CTxMemPool::txiter ancestorIt, setAncestors) {
            o.push_back(ancestorIt->GetTx().GetHash().ToString());
        }

        return o;
    } else {
        UniValue o(UniValue::VOBJ);
        BOOST_FOREACH(CTxMemPool::txiter ancestorIt, setAncestors) {
            const CTxMemPoolEntry &e = *ancestorIt;
            const uint256& _hash = e.GetTx().GetHash();
            UniValue info(UniValue::VOBJ);
            entryToJSON(info, e);
            o.push_back(Pair(_hash.ToString(), info));
        }
        return o;
    }
}

UniValue getmempooldescendants(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2) {
        throw runtime_error(
            "getmempooldescendants txid (verbose)\n"
            "\nIf txid is in the mempool, returns all in-mempool descendants.\n"
            "\nArguments:\n"
            "1. \"txid\"                 (string, required) The transaction id (must be in mempool)\n"
            "2. verbose                  (boolean, optional, default=false) True for a json object, false for array of transaction ids\n"
            "\nResult (for verbose=false):\n"
            "[                       (json array of strings)\n"
            "  \"transactionid\"           (string) The transaction id of an in-mempool descendant transaction\n"
            "  ,...\n"
            "]\n"
            "\nResult (for verbose=true):\n"
            "{                           (json object)\n"
            "  \"transactionid\" : {       (json object)\n"
            + EntryDescriptionString()
            + "  }, ...\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getmempooldescendants", "\"mytxid\"")
            + HelpExampleRpc("getmempooldescendants", "\"mytxid\"")
            );
    }

    bool fVerbose = false;
    if (request.params.size() > 1)
        fVerbose = request.params[1].get_bool();

    uint256 hash = ParseHashV(request.params[0], "parameter 1");

    LOCK(mempool.cs);

    CTxMemPool::txiter it = mempool.mapTx.find(hash);
    if (it == mempool.mapTx.end()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not in mempool");
    }

    CTxMemPool::setEntries setDescendants;
    mempool.CalculateDescendants(it, setDescendants);
    // CTxMemPool::CalculateDescendants will include the given tx
    setDescendants.erase(it);

    if (!fVerbose) {
        UniValue o(UniValue::VARR);
        BOOST_FOREACH(CTxMemPool::txiter descendantIt, setDescendants) {
            o.push_back(descendantIt->GetTx().GetHash().ToString());
        }

        return o;
    } else {
        UniValue o(UniValue::VOBJ);
        BOOST_FOREACH(CTxMemPool::txiter descendantIt, setDescendants) {
            const CTxMemPoolEntry &e = *descendantIt;
            const uint256& _hash = e.GetTx().GetHash();
            UniValue info(UniValue::VOBJ);
            entryToJSON(info, e);
            o.push_back(Pair(_hash.ToString(), info));
        }
        return o;
    }
}

UniValue getmempoolentry(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1) {
        throw runtime_error(
            "getmempoolentry txid\n"
            "\nReturns mempool data for given transaction\n"
            "\nArguments:\n"
            "1. \"txid\"                   (string, required) The transaction id (must be in mempool)\n"
            "\nResult:\n"
            "{                           (json object)\n"
            + EntryDescriptionString()
            + "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getmempoolentry", "\"mytxid\"")
            + HelpExampleRpc("getmempoolentry", "\"mytxid\"")
        );
    }

    uint256 hash = ParseHashV(request.params[0], "parameter 1");

    LOCK(mempool.cs);

    CTxMemPool::txiter it = mempool.mapTx.find(hash);
    if (it == mempool.mapTx.end()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not in mempool");
    }

    const CTxMemPoolEntry &e = *it;
    UniValue info(UniValue::VOBJ);
    entryToJSON(info, e);
    return info;
}

UniValue getblockhash(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            "getblockhash height\n"
            "\nReturns hash of block in best-block-chain at height provided.\n"
            "\nArguments:\n"
            "1. height         (numeric, required) The height index\n"
            "\nResult:\n"
            "\"hash\"         (string) The block hash\n"
            "\nExamples:\n"
            + HelpExampleCli("getblockhash", "1000")
            + HelpExampleRpc("getblockhash", "1000")
        );

    LOCK(cs_main);

    int nHeight = request.params[0].get_int();
    if (nHeight < 0 || nHeight > chainActive.Height())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");

    CBlockIndex* pblockindex = chainActive[nHeight];
    return pblockindex->GetBlockHash().GetHex();
}

UniValue getaccountinfo(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1)
        throw runtime_error(
            "getaccountinfo \"address\"\n"
            "\nArgument:\n"
            "1. \"address\"          (string, required) The account address\n"
        );

    LOCK(cs_main);

    std::string strAddr = request.params[0].get_str();
    if(strAddr.size() != 40 || !CheckHex(strAddr))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Incorrect address");

    dev::Address addrAccount(strAddr);
    if(!globalState->addressInUse(addrAccount))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Address does not exist");
    
    UniValue result(UniValue::VOBJ);

    result.push_back(Pair("address", strAddr));
    result.push_back(Pair("balance", CAmount(globalState->balance(addrAccount))));
    std::vector<uint8_t> code(globalState->code(addrAccount));
    auto storage(globalState->storage(addrAccount));

    UniValue storageUV(UniValue::VOBJ);
    for (auto j: storage)
    {
        UniValue e(UniValue::VOBJ);
        e.push_back(Pair(dev::toHex(j.second.first), dev::toHex(j.second.second)));
        storageUV.push_back(Pair(j.first.hex(), e));
    }
        
    result.push_back(Pair("storage", storageUV));

    result.push_back(Pair("code", HexStr(code.begin(), code.end())));

    std::unordered_map<dev::Address, Vin> vins = globalState->vins();
    if(vins.count(addrAccount)){
        UniValue vin(UniValue::VOBJ);
        valtype vchHash(vins[addrAccount].hash.asBytes());
        vin.push_back(Pair("hash", HexStr(vchHash.rbegin(), vchHash.rend())));
        vin.push_back(Pair("nVout", uint64_t(vins[addrAccount].nVout)));
        vin.push_back(Pair("value", uint64_t(vins[addrAccount].value)));
        result.push_back(Pair("vin", vin));
    }
    return result;
}

UniValue getstorage(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1)
        throw runtime_error(
            "getstorage \"address\"\n"
            "\nArgument:\n"
            "1. \"address\"          (string, required) The address to get the storage from\n"
            "2. \"blockNum\"         (string, optional) Number of block to get state from, \"latest\" keyword supported. Latest if not passed.\n"
            "3. \"index\"            (number, optional) Zero-based index position of the storage\n"
        );

    LOCK(cs_main);

    std::string strAddr = request.params[0].get_str();
    if(strAddr.size() != 40 || !CheckHex(strAddr))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Incorrect address"); 

    TemporaryState ts(globalState);
    if (request.params.size() > 1)
    {
        if (request.params[1].isNum())
        {
            auto blockNum = request.params[1].get_int();
            if((blockNum < 0 && blockNum != -1) || blockNum > chainActive.Height())
                throw JSONRPCError(RPC_INVALID_PARAMS, "Incorrect block number");

            if(blockNum != -1)
                ts.SetRoot(uintToh256(chainActive[blockNum]->hashStateRoot), uintToh256(chainActive[blockNum]->hashUTXORoot));
                
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMS, "Incorrect block number");
        }
    }

    dev::Address addrAccount(strAddr);
    if(!globalState->addressInUse(addrAccount))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Address does not exist");
    
    UniValue result(UniValue::VOBJ);

    bool onlyIndex = request.params.size() > 2;
    unsigned index = 0;
    if (onlyIndex)
        index = request.params[2].get_int();

    auto storage(globalState->storage(addrAccount));

    if (onlyIndex)
    {
        if (index >= storage.size())
        {
            std::ostringstream stringStream;
            stringStream << "Storage size: " << storage.size() << " got index: " << index;
            throw JSONRPCError(RPC_INVALID_PARAMS, stringStream.str());
        }
        auto elem = std::next(storage.begin(), index);
        UniValue e(UniValue::VOBJ);

        storage = {{elem->first, {elem->second.first, elem->second.second}}};
    } 
    for (const auto& j: storage)
    {
        UniValue e(UniValue::VOBJ);
        e.push_back(Pair(dev::toHex(j.second.first), dev::toHex(j.second.second)));
        result.push_back(Pair(j.first.hex(), e));
    }
    return result;
}

UniValue getblockheader(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw runtime_error(
            "getblockheader \"hash\" ( verbose )\n"
            "\nIf verbose is false, returns a string that is serialized, hex-encoded data for blockheader 'hash'.\n"
            "If verbose is true, returns an Object with information about blockheader <hash>.\n"
            "\nArguments:\n"
            "1. \"hash\"          (string, required) The block hash\n"
            "2. verbose           (boolean, optional, default=true) true for a json object, false for the hex encoded data\n"
            "\nResult (for verbose = true):\n"
            "{\n"
            "  \"hash\" : \"hash\",     (string) the block hash (same as provided)\n"
            "  \"confirmations\" : n,   (numeric) The number of confirmations, or -1 if the block is not on the main chain\n"
            "  \"height\" : n,          (numeric) The block height or index\n"
            "  \"version\" : n,         (numeric) The block version\n"
            "  \"versionHex\" : \"00000000\", (string) The block version formatted in hexadecimal\n"
            "  \"merkleroot\" : \"xxxx\", (string) The merkle root\n"
            "  \"time\" : ttt,          (numeric) The block time in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"mediantime\" : ttt,    (numeric) The median block time in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"nonce\" : n,           (numeric) The nonce\n"
            "  \"bits\" : \"1d00ffff\", (string) The bits\n"
            "  \"difficulty\" : x.xxx,  (numeric) The difficulty\n"
            "  \"chainwork\" : \"0000...1f3\"     (string) Expected number of hashes required to produce the current chain (in hex)\n"
            "  \"previousblockhash\" : \"hash\",  (string) The hash of the previous block\n"
            "  \"nextblockhash\" : \"hash\",      (string) The hash of the next block\n"
            "}\n"
            "\nResult (for verbose=false):\n"
            "\"data\"             (string) A string that is serialized, hex-encoded data for block 'hash'.\n"
            "\nExamples:\n"
            + HelpExampleCli("getblockheader", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"")
            + HelpExampleRpc("getblockheader", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"")
        );

    LOCK(cs_main);

    std::string strHash = request.params[0].get_str();
    uint256 hash(uint256S(strHash));

    bool fVerbose = true;
    if (request.params.size() > 1)
        fVerbose = request.params[1].get_bool();

    if (mapBlockIndex.count(hash) == 0)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

    CBlockIndex* pblockindex = mapBlockIndex[hash];

    if (!fVerbose)
    {
        CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
        ssBlock << pblockindex->GetBlockHeader();
        std::string strHex = HexStr(ssBlock.begin(), ssBlock.end());
        return strHex;
    }

    return blockheaderToJSON(pblockindex);
}

UniValue getblock(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw runtime_error(
            "getblock \"blockhash\" ( verbose )\n"
            "\nIf verbose is false, returns a string that is serialized, hex-encoded data for block 'hash'.\n"
            "If verbose is true, returns an Object with information about block <hash>.\n"
            "\nArguments:\n"
            "1. \"blockhash\"          (string, required) The block hash\n"
            "2. verbose                (boolean, optional, default=true) true for a json object, false for the hex encoded data\n"
            "\nResult (for verbose = true):\n"
            "{\n"
            "  \"hash\" : \"hash\",     (string) the block hash (same as provided)\n"
            "  \"confirmations\" : n,   (numeric) The number of confirmations, or -1 if the block is not on the main chain\n"
            "  \"size\" : n,            (numeric) The block size\n"
            "  \"strippedsize\" : n,    (numeric) The block size excluding witness data\n"
            "  \"weight\" : n           (numeric) The block weight as defined in BIP 141\n"
            "  \"height\" : n,          (numeric) The block height or index\n"
            "  \"version\" : n,         (numeric) The block version\n"
            "  \"versionHex\" : \"00000000\", (string) The block version formatted in hexadecimal\n"
            "  \"merkleroot\" : \"xxxx\", (string) The merkle root\n"
            "  \"tx\" : [               (array of string) The transaction ids\n"
            "     \"transactionid\"     (string) The transaction id\n"
            "     ,...\n"
            "  ],\n"
            "  \"time\" : ttt,          (numeric) The block time in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"mediantime\" : ttt,    (numeric) The median block time in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"nonce\" : n,           (numeric) The nonce\n"
            "  \"bits\" : \"1d00ffff\", (string) The bits\n"
            "  \"difficulty\" : x.xxx,  (numeric) The difficulty\n"
            "  \"chainwork\" : \"xxxx\",  (string) Expected number of hashes required to produce the chain up to this block (in hex)\n"
            "  \"previousblockhash\" : \"hash\",  (string) The hash of the previous block\n"
            "  \"nextblockhash\" : \"hash\"       (string) The hash of the next block\n"
            "}\n"
            "\nResult (for verbose=false):\n"
            "\"data\"             (string) A string that is serialized, hex-encoded data for block 'hash'.\n"
            "\nExamples:\n"
            + HelpExampleCli("getblock", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"")
            + HelpExampleRpc("getblock", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"")
        );

    LOCK(cs_main);

    std::string strHash = request.params[0].get_str();
    uint256 hash(uint256S(strHash));

    bool fVerbose = true;
    if (request.params.size() > 1)
        fVerbose = request.params[1].get_bool();

    if (mapBlockIndex.count(hash) == 0)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];

    if (fHavePruned && !(pblockindex->nStatus & BLOCK_HAVE_DATA) && pblockindex->nTx > 0)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Block not available (pruned data)");

    if(!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus()))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");

    if (!fVerbose)
    {
        CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION | RPCSerializationFlags());
        ssBlock << block;
        std::string strHex = HexStr(ssBlock.begin(), ssBlock.end());
        return strHex;
    }

    return blockToJSON(block, pblockindex);
}

////////////////////////////////////////////////////////////////////// // qtum
UniValue callcontract(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 2)
        throw runtime_error(
             "callcontract \"address\" \"data\" ( address )\n"
             "\nArgument:\n"
             "1. \"address\"          (string, required) The account address\n"
             "2. \"data\"             (string, required) The data hex string\n"
             "3. address              (string, optional) The sender address hex string\n"
             "4. gasLimit             (string, optional) The gas limit for executing the contract\n"
         );
 
    LOCK(cs_main);
    
    std::string strAddr = request.params[0].get_str();
    std::string data = request.params[1].get_str();

    if(data.size() % 2 != 0 || !CheckHex(data))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid data (data not hex)");

    if(strAddr.size() != 40 || !CheckHex(strAddr))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Incorrect address");
 
    dev::Address addrAccount(strAddr);
    if(!globalState->addressInUse(addrAccount))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Address does not exist");
    
    dev::Address senderAddress;
    if(request.params.size() == 3){
        CBitcoinAddress qtumSenderAddress(request.params[2].get_str());
        if(qtumSenderAddress.IsValid()){
            CKeyID keyid;
            qtumSenderAddress.GetKeyID(keyid);
            senderAddress = dev::Address(HexStr(valtype(keyid.begin(),keyid.end())));
        }else{
            senderAddress = dev::Address(request.params[2].get_str());
        }

    }
    uint64_t gasLimit=0;
    if(request.params.size() == 4){
        gasLimit = request.params[3].get_int();
    }


    std::vector<ResultExecute> execResults = CallContract(addrAccount, ParseHex(data), senderAddress, gasLimit);

    if(fRecordLogOpcodes){
        writeVMlog(execResults);
    }

    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("address", strAddr));
    result.push_back(Pair("executionResult", executionResultToJSON(execResults[0].execRes)));
    result.push_back(Pair("transactionReceipt", transactionReceiptToJSON(execResults[0].txRec)));
 
    return result;
}

void transactionReceiptInfoToJSON(const TransactionReceiptInfo& resExec, UniValue& entry){
    entry.push_back(Pair("blockHash", resExec.blockHash.GetHex()));
    entry.push_back(Pair("blockNumber", uint64_t(resExec.blockNumber)));
    entry.push_back(Pair("transactionHash", resExec.transactionHash.GetHex()));
    entry.push_back(Pair("transactionIndex", uint64_t(resExec.transactionIndex)));
    entry.push_back(Pair("from", resExec.from.hex()));
    entry.push_back(Pair("to", resExec.to.hex()));
    entry.push_back(Pair("cumulativeGasUsed", CAmount(resExec.cumulativeGasUsed)));
    entry.push_back(Pair("gasUsed", CAmount(resExec.gasUsed)));
    entry.push_back(Pair("contractAddress", resExec.contractAddress.hex()));

    dev::eth::LogEntries logs = resExec.logs;
    UniValue logEntries(UniValue::VARR);
    for(dev::eth::LogEntry log : logs){
        UniValue logEntry(UniValue::VOBJ);
        logEntry.push_back(Pair("address", log.address.hex()));
        UniValue topics(UniValue::VARR);
        for(dev::h256 hash : log.topics){
            topics.push_back(hash.hex());
        }
        logEntry.push_back(Pair("topics", topics));
        logEntry.push_back(Pair("data", HexStr(log.data)));

        logEntries.push_back(logEntry);
    }
    entry.push_back(Pair("log", logEntries));
}

bool getContarctAddressesFromParams(const UniValue& params, std::vector<dev::h160> &addresses)
{
    if (params[2].isStr()) {
        auto addrStr(params[2].get_str());
        if (addrStr.length() != 40)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
        dev::h160 address(params[2].get_str());
        addresses.push_back(address);
    } else if (params[2].isObject()) {

        UniValue addressValues = find_value(params[2].get_obj(), "addresses");
        if (!addressValues.isArray()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Addresses is expected to be an array");
        }

        std::vector<UniValue> values = addressValues.getValues();

        for (std::vector<UniValue>::iterator it = values.begin(); it != values.end(); ++it) {
            auto addrStr(it->get_str());
            if (addrStr.length() != 40 || !CheckHex(addrStr))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            addresses.push_back(dev::h160(addrStr));
        }
    } else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    return true;
}

bool getTopicsFromParams(const UniValue& params, std::vector<std::pair<unsigned, dev::h256>> &topics)
{
    if (params[3].isObject()) {

        UniValue topicValues = find_value(params[3].get_obj(), "topics");
        if (!topicValues.isArray()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Topics is expected to be an array");
        }

        std::vector<UniValue> values = topicValues.getValues();

        for (size_t i = 0; i < values.size(); ++i) {
            auto topicStr(values[i].get_str());
            if (topicStr == "null")
                continue;
            if (topicStr.length() != 64 || !CheckHex(topicStr))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid topic");
            topics.push_back({i, dev::h256(topicStr)});
        }
    } else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid topic");
    }

    return true;
}

UniValue searchlogs(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 2)
        throw runtime_error(
             "searchlogs <fromBlock> <toBlock> (address) (topics)\n"
             "requires -logevents to be enabled"
             "\nArgument:\n"
             "1. \"fromBlock\"        (numeric, required) The number of the earliest block (latest may be given to mean the most recent block).\n"
             "2. \"toBlock\"          (string, required) The number of the latest block (-1 may be given to mean the most recent block).\n"
             "3. \"address\"          (string, optional) An address or a list of addresses to only get logs from particular account(s).\n"
             "4. \"topics\"           (string, optional) An array of values which must each appear in the log entries. The order is important, if you want to leave topics out use null, e.g. [\"null\", \"0x00...\"]. \n"
             "\nExamples:\n"
            + HelpExampleCli("searchlogs", "0 100 '{\"addresses\": [\"12ae42729af478ca92c8c66773a3e32115717be4\"]}' '{\"topics\": [\"null\",\"b436c2bf863ccd7b8f63171201efd4792066b4ce8e543dde9c3e9e9ab98e216c\"]}'")
            + HelpExampleRpc("searchlogs", "0 100 {\"addresses\": [\"12ae42729af478ca92c8c66773a3e32115717be4\"]} {\"topics\": [\"null\",\"b436c2bf863ccd7b8f63171201efd4792066b4ce8e543dde9c3e9e9ab98e216c\"]}")
         );

    if(!fLogEvents)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Events indexing disabled");

    LOCK(cs_main);

    int fromBlock = request.params[0].get_int();
    int toBlock = 0;
    toBlock = request.params[1].get_int();
    if(request.params[1].isNum()){
        if(toBlock == -1){
            toBlock = chainActive.Height();
        } else {
            toBlock = request.params[1].get_int();
        }
    }

    std::set<dev::h160> addresses;
    std::vector<dev::h160> vecAddresses;
    std::vector<std::pair<unsigned, dev::h256>> topics;

    if (request.params.size() > 2)
    {
        if (!getContarctAddressesFromParams(request.params, vecAddresses)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
        }
        addresses.insert(vecAddresses.begin(), vecAddresses.end());
    }

    if (request.params.size() > 3 && !getTopicsFromParams(request.params, topics))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    
    std::vector<std::vector<uint256>> hashesToBlock;
    if (!pblocktree->ReadHeightIndex(toBlock, fromBlock, hashesToBlock, addresses))
        throw JSONRPCError(RPC_INVALID_PARAMS, "Could not read tx height index");

    UniValue result(UniValue::VARR);
    boost::filesystem::path stateDir = GetDataDir() / "stateQtum";
    StorageResults storageRes(stateDir.string());

    for(const auto& hashesTx : hashesToBlock)
    {
        for(const auto& e : hashesTx)
        {
            std::vector<TransactionReceiptInfo> transactionReceiptInfo = storageRes.getResult(uintToh256(e));
            
            for(TransactionReceiptInfo& t : transactionReceiptInfo){
                if (!topics.empty())
                {
                    bool skip = true;
                    for (const auto& tc: topics)
                    {
                        for (const auto& log: t.logs)
                        {                  
                            if(tc.first < log.topics.size() && tc.second == log.topics[tc.first])
                            {
                                skip = false;
                                break;
                            }
                        }
                        if (!skip) break;
                    }
                    if (skip) continue;
                }
                
                if(!t.logs.empty()){
                    UniValue tri(UniValue::VOBJ);
                    transactionReceiptInfoToJSON(t, tri);
                    result.push_back(tri);
                }
            }
        }
    }

    return result;
}

UniValue gettransactionreceipt(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1)
        throw runtime_error(
             "gettransactionreceipt \"hash\"\n"
             "requires -logevents to be enabled"
             "\nArgument:\n"
             "1. \"hash\"          (string, required) The transaction hash\n"
         );
 
    if(!fLogEvents)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Events indexing disabled");

    LOCK(cs_main);

    std::string hashTemp = request.params[0].get_str();
    if(hashTemp.size() != 64){
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Incorrect hash");
    }
    
    uint256 hash(uint256S(hashTemp));

    boost::filesystem::path stateDir = GetDataDir() / "stateQtum";
    StorageResults storageRes(stateDir.string());

    std::vector<TransactionReceiptInfo> transactionReceiptInfo = storageRes.getResult(uintToh256(hash));

    UniValue result(UniValue::VARR);
    for(TransactionReceiptInfo& t : transactionReceiptInfo){
        UniValue tri(UniValue::VOBJ);
        transactionReceiptInfoToJSON(t, tri);
        result.push_back(tri);
    }
    return result;
}
//////////////////////////////////////////////////////////////////////

UniValue listcontracts(const JSONRPCRequest& request)
{
	if (request.fHelp)
		throw runtime_error(
				"listcontracts (start maxDisplay)\n"
				"\nArgument:\n"
				"1. start     (numeric or string, optional) The starting account index, default 1\n"
				"2. maxDisplay       (numeric or string, optional) Max accounts to list, default 20\n"
		);

	LOCK(cs_main);

	int start=1;
	if (request.params.size() > 0){
		start = request.params[0].get_int();
		if (start<= 0)
			throw JSONRPCError(RPC_TYPE_ERROR, "Invalid start, min=1");
	}

	int maxDisplay=20;
	if (request.params.size() > 1){
		maxDisplay = request.params[1].get_int();
		if (maxDisplay <= 0)
			throw JSONRPCError(RPC_TYPE_ERROR, "Invalid maxDisplay");
	}

	UniValue result(UniValue::VOBJ);

	auto map = globalState->addresses();
	int contractsCount=(int)map.size();

	if (contractsCount>0 && start > contractsCount)
		throw JSONRPCError(RPC_TYPE_ERROR, "start greater than max index "+ itostr(contractsCount));

	int itStartPos=std::min(start-1,contractsCount);
	int i=0;
	for (auto it = std::next(map.begin(),itStartPos); it!=map.end(); it++)
	{
		result.push_back(Pair(it->first.hex(),ValueFromAmount(CAmount(globalState->balance(it->first)))));
		i++;
		if(i==maxDisplay)break;
	}

	return result;
}


struct CCoinsStats
{
    int nHeight;
    uint256 hashBlock;
    uint64_t nTransactions;
    uint64_t nTransactionOutputs;
    uint64_t nSerializedSize;
    uint256 hashSerialized;
    CAmount nTotalAmount;

    CCoinsStats() : nHeight(0), nTransactions(0), nTransactionOutputs(0), nSerializedSize(0), nTotalAmount(0) {}
};

//! Calculate statistics about the unspent transaction output set
static bool GetUTXOStats(CCoinsView *view, CCoinsStats &stats)
{
    std::unique_ptr<CCoinsViewCursor> pcursor(view->Cursor());

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    stats.hashBlock = pcursor->GetBestBlock();
    {
        LOCK(cs_main);
        stats.nHeight = mapBlockIndex.find(stats.hashBlock)->second->nHeight;
    }
    ss << stats.hashBlock;
    CAmount nTotalAmount = 0;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        uint256 key;
        CCoins coins;
        if (pcursor->GetKey(key) && pcursor->GetValue(coins)) {
            stats.nTransactions++;
            ss << key;
            for (unsigned int i=0; i<coins.vout.size(); i++) {
                const CTxOut &out = coins.vout[i];
                if (!out.IsNull()) {
                    stats.nTransactionOutputs++;
                    ss << VARINT(i+1);
                    ss << out;
                    nTotalAmount += out.nValue;
                }
            }
            stats.nSerializedSize += 32 + pcursor->GetValueSize();
            ss << VARINT(0);
        } else {
            return error("%s: unable to read value", __func__);
        }
        pcursor->Next();
    }
    stats.hashSerialized = ss.GetHash();
    stats.nTotalAmount = nTotalAmount;
    return true;
}

UniValue pruneblockchain(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            "pruneblockchain\n"
            "\nArguments:\n"
            "1. \"height\"       (numeric, required) The block height to prune up to. May be set to a discrete height, or a unix timestamp\n"
            "                  to prune blocks whose block time is at least 2 hours older than the provided timestamp.\n"
            "\nResult:\n"
            "n    (numeric) Height of the last block pruned.\n"
            "\nExamples:\n"
            + HelpExampleCli("pruneblockchain", "1000")
            + HelpExampleRpc("pruneblockchain", "1000"));

    if (!fPruneMode)
        throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Cannot prune blocks because node is not in prune mode.");

    LOCK(cs_main);

    int heightParam = request.params[0].get_int();
    if (heightParam < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative block height.");

    // Height value more than a billion is too high to be a block height, and
    // too low to be a block time (corresponds to timestamp from Sep 2001).
    if (heightParam > 1000000000) {
        // Add a 2 hour buffer to include blocks which might have had old timestamps
        CBlockIndex* pindex = chainActive.FindEarliestAtLeast(heightParam - 7200);
        if (!pindex) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Could not find block with at least the specified timestamp.");
        }
        heightParam = pindex->nHeight;
    }

    unsigned int height = (unsigned int) heightParam;
    unsigned int chainHeight = (unsigned int) chainActive.Height();
    if (chainHeight < Params().PruneAfterHeight())
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Blockchain is too short for pruning.");
    else if (height > chainHeight)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Blockchain is shorter than the attempted prune height.");
    else if (height > chainHeight - MIN_BLOCKS_TO_KEEP) {
        LogPrint("rpc", "Attempt to prune blocks close to the tip.  Retaining the minimum number of blocks.");
        height = chainHeight - MIN_BLOCKS_TO_KEEP;
    }

    PruneBlockFilesManual(height);
    return uint64_t(height);
}

UniValue gettxoutsetinfo(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0)
        throw runtime_error(
            "gettxoutsetinfo\n"
            "\nReturns statistics about the unspent transaction output set.\n"
            "Note this call may take some time.\n"
            "\nResult:\n"
            "{\n"
            "  \"height\":n,     (numeric) The current block height (index)\n"
            "  \"bestblock\": \"hex\",   (string) the best block hash hex\n"
            "  \"transactions\": n,      (numeric) The number of transactions\n"
            "  \"txouts\": n,            (numeric) The number of output transactions\n"
            "  \"bytes_serialized\": n,  (numeric) The serialized size\n"
            "  \"hash_serialized\": \"hash\",   (string) The serialized hash\n"
            "  \"total_amount\": x.xxx          (numeric) The total amount\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("gettxoutsetinfo", "")
            + HelpExampleRpc("gettxoutsetinfo", "")
        );

    UniValue ret(UniValue::VOBJ);

    CCoinsStats stats;
    FlushStateToDisk();
    if (GetUTXOStats(pcoinsTip, stats)) {
        ret.push_back(Pair("height", (int64_t)stats.nHeight));
        ret.push_back(Pair("bestblock", stats.hashBlock.GetHex()));
        ret.push_back(Pair("transactions", (int64_t)stats.nTransactions));
        ret.push_back(Pair("txouts", (int64_t)stats.nTransactionOutputs));
        ret.push_back(Pair("bytes_serialized", (int64_t)stats.nSerializedSize));
        ret.push_back(Pair("hash_serialized", stats.hashSerialized.GetHex()));
        ret.push_back(Pair("total_amount", ValueFromAmount(stats.nTotalAmount)));
    } else {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Unable to read UTXO set");
    }
    return ret;
}

UniValue gettxout(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 3)
        throw runtime_error(
            "gettxout \"txid\" n ( include_mempool )\n"
            "\nReturns details about an unspent transaction output.\n"
            "\nArguments:\n"
            "1. \"txid\"       (string, required) The transaction id\n"
            "2. n              (numeric, required) vout number\n"
            "3. include_mempool  (boolean, optional) Whether to include the mempool\n"
            "\nResult:\n"
            "{\n"
            "  \"bestblock\" : \"hash\",    (string) the block hash\n"
            "  \"confirmations\" : n,       (numeric) The number of confirmations\n"
            "  \"value\" : x.xxx,           (numeric) The transaction value in " + CURRENCY_UNIT + "\n"
            "  \"scriptPubKey\" : {         (json object)\n"
            "     \"asm\" : \"code\",       (string) \n"
            "     \"hex\" : \"hex\",        (string) \n"
            "     \"reqSigs\" : n,          (numeric) Number of required signatures\n"
            "     \"type\" : \"pubkeyhash\", (string) The type, eg pubkeyhash\n"
            "     \"addresses\" : [          (array of string) array of qtum addresses\n"
            "        \"address\"     (string) qtum address\n"
            "        ,...\n"
            "     ]\n"
            "  },\n"
            "  \"version\" : n,            (numeric) The version\n"
            "  \"coinbase\" : true|false   (boolean) Coinbase or not\n"
            "}\n"

            "\nExamples:\n"
            "\nGet unspent transactions\n"
            + HelpExampleCli("listunspent", "") +
            "\nView the details\n"
            + HelpExampleCli("gettxout", "\"txid\" 1") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("gettxout", "\"txid\", 1")
        );

    LOCK(cs_main);

    UniValue ret(UniValue::VOBJ);

    std::string strHash = request.params[0].get_str();
    uint256 hash(uint256S(strHash));
    int n = request.params[1].get_int();
    bool fMempool = true;
    if (request.params.size() > 2)
        fMempool = request.params[2].get_bool();

    CCoins coins;
    if (fMempool) {
        LOCK(mempool.cs);
        CCoinsViewMemPool view(pcoinsTip, mempool);
        if (!view.GetCoins(hash, coins))
            return NullUniValue;
        mempool.pruneSpent(hash, coins); // TODO: this should be done by the CCoinsViewMemPool
    } else {
        if (!pcoinsTip->GetCoins(hash, coins))
            return NullUniValue;
    }
    if (n<0 || (unsigned int)n>=coins.vout.size() || coins.vout[n].IsNull())
        return NullUniValue;

    BlockMap::iterator it = mapBlockIndex.find(pcoinsTip->GetBestBlock());
    CBlockIndex *pindex = it->second;
    ret.push_back(Pair("bestblock", pindex->GetBlockHash().GetHex()));
    if ((unsigned int)coins.nHeight == MEMPOOL_HEIGHT)
        ret.push_back(Pair("confirmations", 0));
    else
        ret.push_back(Pair("confirmations", pindex->nHeight - coins.nHeight + 1));
    ret.push_back(Pair("value", ValueFromAmount(coins.vout[n].nValue)));
    UniValue o(UniValue::VOBJ);
    ScriptPubKeyToJSON(coins.vout[n].scriptPubKey, o, true);
    ret.push_back(Pair("scriptPubKey", o));
    ret.push_back(Pair("version", coins.nVersion));
    ret.push_back(Pair("coinbase", coins.fCoinBase));

    return ret;
}

UniValue verifychain(const JSONRPCRequest& request)
{
    int nCheckLevel = GetArg("-checklevel", DEFAULT_CHECKLEVEL);
    int nCheckDepth = GetArg("-checkblocks", DEFAULT_CHECKBLOCKS);
    if (request.fHelp || request.params.size() > 2)
        throw runtime_error(
            "verifychain ( checklevel nblocks )\n"
            "\nVerifies blockchain database.\n"
            "\nArguments:\n"
            "1. checklevel   (numeric, optional, 0-4, default=" + strprintf("%d", nCheckLevel) + ") How thorough the block verification is.\n"
            "2. nblocks      (numeric, optional, default=" + strprintf("%d", nCheckDepth) + ", 0=all) The number of blocks to check.\n"
            "\nResult:\n"
            "true|false       (boolean) Verified or not\n"
            "\nExamples:\n"
            + HelpExampleCli("verifychain", "")
            + HelpExampleRpc("verifychain", "")
        );

    LOCK(cs_main);

    if (request.params.size() > 0)
        nCheckLevel = request.params[0].get_int();
    if (request.params.size() > 1)
        nCheckDepth = request.params[1].get_int();

    return CVerifyDB().VerifyDB(Params(), pcoinsTip, nCheckLevel, nCheckDepth);
}

/** Implementation of IsSuperMajority with better feedback */
static UniValue SoftForkMajorityDesc(int version, CBlockIndex* pindex, const Consensus::Params& consensusParams)
{
    UniValue rv(UniValue::VOBJ);
    bool activated = false;
    switch(version)
    {
        case 2:
            activated = pindex->nHeight >= consensusParams.BIP34Height;
            break;
        case 3:
            activated = pindex->nHeight >= consensusParams.BIP66Height;
            break;
        case 4:
            activated = pindex->nHeight >= consensusParams.BIP65Height;
            break;
    }
    rv.push_back(Pair("status", activated));
    return rv;
}

static UniValue SoftForkDesc(const std::string &name, int version, CBlockIndex* pindex, const Consensus::Params& consensusParams)
{
    UniValue rv(UniValue::VOBJ);
    rv.push_back(Pair("id", name));
    rv.push_back(Pair("version", version));
    rv.push_back(Pair("reject", SoftForkMajorityDesc(version, pindex, consensusParams)));
    return rv;
}

static UniValue BIP9SoftForkDesc(const Consensus::Params& consensusParams, Consensus::DeploymentPos id)
{
    UniValue rv(UniValue::VOBJ);
    const ThresholdState thresholdState = VersionBitsTipState(consensusParams, id);
    switch (thresholdState) {
    case THRESHOLD_DEFINED: rv.push_back(Pair("status", "defined")); break;
    case THRESHOLD_STARTED: rv.push_back(Pair("status", "started")); break;
    case THRESHOLD_LOCKED_IN: rv.push_back(Pair("status", "locked_in")); break;
    case THRESHOLD_ACTIVE: rv.push_back(Pair("status", "active")); break;
    case THRESHOLD_FAILED: rv.push_back(Pair("status", "failed")); break;
    }
    if (THRESHOLD_STARTED == thresholdState)
    {
        rv.push_back(Pair("bit", consensusParams.vDeployments[id].bit));
    }
    rv.push_back(Pair("startTime", consensusParams.vDeployments[id].nStartTime));
    rv.push_back(Pair("timeout", consensusParams.vDeployments[id].nTimeout));
    rv.push_back(Pair("since", VersionBitsTipStateSinceHeight(consensusParams, id)));
    return rv;
}

void BIP9SoftForkDescPushBack(UniValue& bip9_softforks, const std::string &name, const Consensus::Params& consensusParams, Consensus::DeploymentPos id)
{
    // Deployments with timeout value of 0 are hidden.
    // A timeout value of 0 guarantees a softfork will never be activated.
    // This is used when softfork codes are merged without specifying the deployment schedule.
    if (consensusParams.vDeployments[id].nTimeout > 0)
        bip9_softforks.push_back(Pair(name, BIP9SoftForkDesc(consensusParams, id)));
}

UniValue getblockchaininfo(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0)
        throw runtime_error(
            "getblockchaininfo\n"
            "Returns an object containing various state info regarding blockchain processing.\n"
            "\nResult:\n"
            "{\n"
            "  \"chain\": \"xxxx\",        (string) current network name as defined in BIP70 (main, test, regtest)\n"
            "  \"blocks\": xxxxxx,         (numeric) the current number of blocks processed in the server\n"
            "  \"headers\": xxxxxx,        (numeric) the current number of headers we have validated\n"
            "  \"bestblockhash\": \"...\", (string) the hash of the currently best block\n"
            "  \"difficulty\": xxxxxx,     (numeric) the current difficulty\n"
            "  \"mediantime\": xxxxxx,     (numeric) median time for the current best block\n"
            "  \"verificationprogress\": xxxx, (numeric) estimate of verification progress [0..1]\n"
            "  \"chainwork\": \"xxxx\"     (string) total amount of work in active chain, in hexadecimal\n"
            "  \"pruned\": xx,             (boolean) if the blocks are subject to pruning\n"
            "  \"pruneheight\": xxxxxx,    (numeric) lowest-height complete block stored\n"
            "  \"softforks\": [            (array) status of softforks in progress\n"
            "     {\n"
            "        \"id\": \"xxxx\",        (string) name of softfork\n"
            "        \"version\": xx,         (numeric) block version\n"
            "        \"reject\": {            (object) progress toward rejecting pre-softfork blocks\n"
            "           \"status\": xx,       (boolean) true if threshold reached\n"
            "        },\n"
            "     }, ...\n"
            "  ],\n"
            "  \"bip9_softforks\": {          (object) status of BIP9 softforks in progress\n"
            "     \"xxxx\" : {                (string) name of the softfork\n"
            "        \"status\": \"xxxx\",    (string) one of \"defined\", \"started\", \"locked_in\", \"active\", \"failed\"\n"
            "        \"bit\": xx,             (numeric) the bit (0-28) in the block version field used to signal this softfork (only for \"started\" status)\n"
            "        \"startTime\": xx,       (numeric) the minimum median time past of a block at which the bit gains its meaning\n"
            "        \"timeout\": xx,         (numeric) the median time past of a block at which the deployment is considered failed if not yet locked in\n"
            "        \"since\": xx            (numeric) height of the first block to which the status applies\n"
            "     }\n"
            "  }\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getblockchaininfo", "")
            + HelpExampleRpc("getblockchaininfo", "")
        );

    LOCK(cs_main);

    UniValue obj(UniValue::VOBJ);
    obj.push_back(Pair("chain",                 Params().NetworkIDString()));
    obj.push_back(Pair("blocks",                (int)chainActive.Height()));
    obj.push_back(Pair("headers",               pindexBestHeader ? pindexBestHeader->nHeight : -1));
    obj.push_back(Pair("bestblockhash",         chainActive.Tip()->GetBlockHash().GetHex()));
    obj.push_back(Pair("difficulty",            (double)GetDifficulty()));
    obj.push_back(Pair("mediantime",            (int64_t)chainActive.Tip()->GetMedianTimePast()));
    obj.push_back(Pair("verificationprogress",  GuessVerificationProgress(Params().TxData(), chainActive.Tip())));
    obj.push_back(Pair("chainwork",             chainActive.Tip()->nChainWork.GetHex()));
    obj.push_back(Pair("pruned",                fPruneMode));

    const Consensus::Params& consensusParams = Params().GetConsensus();
    CBlockIndex* tip = chainActive.Tip();
    UniValue softforks(UniValue::VARR);
    UniValue bip9_softforks(UniValue::VOBJ);
    softforks.push_back(SoftForkDesc("bip34", 2, tip, consensusParams));
    softforks.push_back(SoftForkDesc("bip66", 3, tip, consensusParams));
    softforks.push_back(SoftForkDesc("bip65", 4, tip, consensusParams));
    BIP9SoftForkDescPushBack(bip9_softforks, "csv", consensusParams, Consensus::DEPLOYMENT_CSV);
    BIP9SoftForkDescPushBack(bip9_softforks, "segwit", consensusParams, Consensus::DEPLOYMENT_SEGWIT);
    obj.push_back(Pair("softforks",             softforks));
    obj.push_back(Pair("bip9_softforks", bip9_softforks));

    if (fPruneMode)
    {
        CBlockIndex *block = chainActive.Tip();
        while (block && block->pprev && (block->pprev->nStatus & BLOCK_HAVE_DATA))
            block = block->pprev;

        obj.push_back(Pair("pruneheight",        block->nHeight));
    }
    return obj;
}

/** Comparison function for sorting the getchaintips heads.  */
struct CompareBlocksByHeight
{
    bool operator()(const CBlockIndex* a, const CBlockIndex* b) const
    {
        /* Make sure that unequal blocks with the same height do not compare
           equal. Use the pointers themselves to make a distinction. */

        if (a->nHeight != b->nHeight)
          return (a->nHeight > b->nHeight);

        return a < b;
    }
};

UniValue getchaintips(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0)
        throw runtime_error(
            "getchaintips\n"
            "Return information about all known tips in the block tree,"
            " including the main chain as well as orphaned branches.\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"height\": xxxx,         (numeric) height of the chain tip\n"
            "    \"hash\": \"xxxx\",         (string) block hash of the tip\n"
            "    \"branchlen\": 0          (numeric) zero for main chain\n"
            "    \"status\": \"active\"      (string) \"active\" for the main chain\n"
            "  },\n"
            "  {\n"
            "    \"height\": xxxx,\n"
            "    \"hash\": \"xxxx\",\n"
            "    \"branchlen\": 1          (numeric) length of branch connecting the tip to the main chain\n"
            "    \"status\": \"xxxx\"        (string) status of the chain (active, valid-fork, valid-headers, headers-only, invalid)\n"
            "  }\n"
            "]\n"
            "Possible values for status:\n"
            "1.  \"invalid\"               This branch contains at least one invalid block\n"
            "2.  \"headers-only\"          Not all blocks for this branch are available, but the headers are valid\n"
            "3.  \"valid-headers\"         All blocks are available for this branch, but they were never fully validated\n"
            "4.  \"valid-fork\"            This branch is not part of the active chain, but is fully validated\n"
            "5.  \"active\"                This is the tip of the active main chain, which is certainly valid\n"
            "\nExamples:\n"
            + HelpExampleCli("getchaintips", "")
            + HelpExampleRpc("getchaintips", "")
        );

    LOCK(cs_main);

    /*
     * Idea:  the set of chain tips is chainActive.tip, plus orphan blocks which do not have another orphan building off of them. 
     * Algorithm:
     *  - Make one pass through mapBlockIndex, picking out the orphan blocks, and also storing a set of the orphan block's pprev pointers.
     *  - Iterate through the orphan blocks. If the block isn't pointed to by another orphan, it is a chain tip.
     *  - add chainActive.Tip()
     */
    std::set<const CBlockIndex*, CompareBlocksByHeight> setTips;
    std::set<const CBlockIndex*> setOrphans;
    std::set<const CBlockIndex*> setPrevs;

    BOOST_FOREACH(const PAIRTYPE(const uint256, CBlockIndex*)& item, mapBlockIndex)
    {
        if (!chainActive.Contains(item.second)) {
            setOrphans.insert(item.second);
            setPrevs.insert(item.second->pprev);
        }
    }

    for (std::set<const CBlockIndex*>::iterator it = setOrphans.begin(); it != setOrphans.end(); ++it)
    {
        if (setPrevs.erase(*it) == 0) {
            setTips.insert(*it);
        }
    }

    // Always report the currently active tip.
    setTips.insert(chainActive.Tip());

    /* Construct the output array.  */
    UniValue res(UniValue::VARR);
    BOOST_FOREACH(const CBlockIndex* block, setTips)
    {
        UniValue obj(UniValue::VOBJ);
        obj.push_back(Pair("height", block->nHeight));
        obj.push_back(Pair("hash", block->phashBlock->GetHex()));

        const int branchLen = block->nHeight - chainActive.FindFork(block)->nHeight;
        obj.push_back(Pair("branchlen", branchLen));

        string status;
        if (chainActive.Contains(block)) {
            // This block is part of the currently active chain.
            status = "active";
        } else if (block->nStatus & BLOCK_FAILED_MASK) {
            // This block or one of its ancestors is invalid.
            status = "invalid";
        } else if (block->nChainTx == 0) {
            // This block cannot be connected because full block data for it or one of its parents is missing.
            status = "headers-only";
        } else if (block->IsValid(BLOCK_VALID_SCRIPTS)) {
            // This block is fully validated, but no longer part of the active chain. It was probably the active block once, but was reorganized.
            status = "valid-fork";
        } else if (block->IsValid(BLOCK_VALID_TREE)) {
            // The headers for this block are valid, but it has not been validated. It was probably never part of the most-work chain.
            status = "valid-headers";
        } else {
            // No clue.
            status = "unknown";
        }
        obj.push_back(Pair("status", status));

        res.push_back(obj);
    }

    return res;
}

UniValue mempoolInfoToJSON()
{
    UniValue ret(UniValue::VOBJ);
    ret.push_back(Pair("size", (int64_t) mempool.size()));
    ret.push_back(Pair("bytes", (int64_t) mempool.GetTotalTxSize()));
    ret.push_back(Pair("usage", (int64_t) mempool.DynamicMemoryUsage()));
    size_t maxmempool = GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000;
    ret.push_back(Pair("maxmempool", (int64_t) maxmempool));
    ret.push_back(Pair("mempoolminfee", ValueFromAmount(mempool.GetMinFee(maxmempool).GetFeePerK())));

    return ret;
}

UniValue getmempoolinfo(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0)
        throw runtime_error(
            "getmempoolinfo\n"
            "\nReturns details on the active state of the TX memory pool.\n"
            "\nResult:\n"
            "{\n"
            "  \"size\": xxxxx,               (numeric) Current tx count\n"
            "  \"bytes\": xxxxx,              (numeric) Sum of all virtual transaction sizes as defined in BIP 141. Differs from actual serialized size because witness data is discounted\n"
            "  \"usage\": xxxxx,              (numeric) Total memory usage for the mempool\n"
            "  \"maxmempool\": xxxxx,         (numeric) Maximum memory usage for the mempool\n"
            "  \"mempoolminfee\": xxxxx       (numeric) Minimum fee for tx to be accepted\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getmempoolinfo", "")
            + HelpExampleRpc("getmempoolinfo", "")
        );

    return mempoolInfoToJSON();
}

UniValue preciousblock(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            "preciousblock \"blockhash\"\n"
            "\nTreats a block as if it were received before others with the same work.\n"
            "\nA later preciousblock call can override the effect of an earlier one.\n"
            "\nThe effects of preciousblock are not retained across restarts.\n"
            "\nArguments:\n"
            "1. \"blockhash\"   (string, required) the hash of the block to mark as precious\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("preciousblock", "\"blockhash\"")
            + HelpExampleRpc("preciousblock", "\"blockhash\"")
        );

    std::string strHash = request.params[0].get_str();
    uint256 hash(uint256S(strHash));
    CBlockIndex* pblockindex;

    {
        LOCK(cs_main);
        if (mapBlockIndex.count(hash) == 0)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

        pblockindex = mapBlockIndex[hash];
    }

    CValidationState state;
    PreciousBlock(state, Params(), pblockindex);

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, state.GetRejectReason());
    }

    return NullUniValue;
}

UniValue invalidateblock(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            "invalidateblock \"blockhash\"\n"
            "\nPermanently marks a block as invalid, as if it violated a consensus rule.\n"
            "\nArguments:\n"
            "1. \"blockhash\"   (string, required) the hash of the block to mark as invalid\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("invalidateblock", "\"blockhash\"")
            + HelpExampleRpc("invalidateblock", "\"blockhash\"")
        );

    std::string strHash = request.params[0].get_str();
    uint256 hash(uint256S(strHash));
    CValidationState state;

    {
        LOCK(cs_main);
        if (mapBlockIndex.count(hash) == 0)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

        CBlockIndex* pblockindex = mapBlockIndex[hash];
        InvalidateBlock(state, Params(), pblockindex);
    }

    if (state.IsValid()) {
        ActivateBestChain(state, Params());
    }

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, state.GetRejectReason());
    }

    return NullUniValue;
}

UniValue reconsiderblock(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            "reconsiderblock \"blockhash\"\n"
            "\nRemoves invalidity status of a block and its descendants, reconsider them for activation.\n"
            "This can be used to undo the effects of invalidateblock.\n"
            "\nArguments:\n"
            "1. \"blockhash\"   (string, required) the hash of the block to reconsider\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("reconsiderblock", "\"blockhash\"")
            + HelpExampleRpc("reconsiderblock", "\"blockhash\"")
        );

    std::string strHash = request.params[0].get_str();
    uint256 hash(uint256S(strHash));

    {
        LOCK(cs_main);
        if (mapBlockIndex.count(hash) == 0)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

        CBlockIndex* pblockindex = mapBlockIndex[hash];
        ResetBlockFailureFlags(pblockindex);
    }

    CValidationState state;
    ActivateBestChain(state, Params());

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, state.GetRejectReason());
    }

    return NullUniValue;
}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         okSafe argNames
  //  --------------------- ------------------------  -----------------------  ------ ----------
    { "blockchain",         "getblockchaininfo",      &getblockchaininfo,      true,  {} },
    { "blockchain",         "getbestblockhash",       &getbestblockhash,       true,  {} },
    { "blockchain",         "getblockcount",          &getblockcount,          true,  {} },
    { "blockchain",         "getblock",               &getblock,               true,  {"blockhash","verbose"} },
    { "blockchain",         "getblockhash",           &getblockhash,           true,  {"height"} },
    { "blockchain",         "getblockheader",         &getblockheader,         true,  {"blockhash","verbose"} },
    { "blockchain",         "getchaintips",           &getchaintips,           true,  {} },
    { "blockchain",         "getdifficulty",          &getdifficulty,          true,  {} },
    { "blockchain",         "getmempoolancestors",    &getmempoolancestors,    true,  {"txid","verbose"} },
    { "blockchain",         "getmempooldescendants",  &getmempooldescendants,  true,  {"txid","verbose"} },
    { "blockchain",         "getmempoolentry",        &getmempoolentry,        true,  {"txid"} },
    { "blockchain",         "getmempoolinfo",         &getmempoolinfo,         true,  {} },
    { "blockchain",         "getrawmempool",          &getrawmempool,          true,  {"verbose"} },
    { "blockchain",         "gettxout",               &gettxout,               true,  {"txid","n","include_mempool"} },
    { "blockchain",         "gettxoutsetinfo",        &gettxoutsetinfo,        true,  {} },
    { "blockchain",         "pruneblockchain",        &pruneblockchain,        true,  {"height"} },
    { "blockchain",         "verifychain",            &verifychain,            true,  {"checklevel","nblocks"} },
    { "blockchain",         "getaccountinfo",         &getaccountinfo,         true,  {"contract_address"} },
    { "blockchain",         "getstorage",             &getstorage,             true,  {"address, index, blockNum"} },

    { "blockchain",         "preciousblock",          &preciousblock,          true,  {"blockhash"} },

    { "blockchain",         "callcontract",           &callcontract,           true,  {"address","data"} }, // qtum

    /* Not shown in help */
    { "hidden",             "invalidateblock",        &invalidateblock,        true,  {"blockhash"} },
    { "hidden",             "reconsiderblock",        &reconsiderblock,        true,  {"blockhash"} },
    { "hidden",             "waitfornewblock",        &waitfornewblock,        true,  {"timeout"} },
    { "hidden",             "waitforblock",           &waitforblock,           true,  {"blockhash","timeout"} },
    { "hidden",             "waitforblockheight",     &waitforblockheight,     true,  {"height","timeout"} },
	{ "blockchain",         "listcontracts",          &listcontracts,          true,  {"start", "maxDisplay"} },
    { "blockchain",         "gettransactionreceipt",  &gettransactionreceipt,  true,  {"hash"} },
    { "blockchain",         "searchlogs",             &searchlogs,             true,  {"fromBlock", "toBlock", "address", "topics"} },
};

void RegisterBlockchainRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
