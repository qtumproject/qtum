#include <rpc/util.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/mining.h>
#include <wallet/rpc/util.h>
#include <wallet/rpc/mining.h>
#include <wallet/wallet.h>
#include <node/miner.h>
#include <node/context.h>
#include <pow.h>
#include <node/warnings.h>
#include <chainparams.h>
#include <common/args.h>
#include <chainparamsbase.h>
#include <policy/feerate.h>

#include <univalue.h>

using node::BlockAssembler;

namespace wallet {

UniValue GetReqNetworkHashPS(const JSONRPCRequest& request, ChainstateManager& chainman)
{
    return GetNetworkHashPS(!request.params[0].isNull() ? request.params[0].getInt<int>() : 120, !request.params[1].isNull() ? request.params[1].getInt<int>() : -1, chainman.ActiveChain());
}

RPCHelpMan getmininginfo()
{
    return RPCHelpMan{
        "getmininginfo",
        "Returns a json object containing mining-related information.",
                {},
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::NUM, "blocks", "The current block"},
                        {RPCResult::Type::NUM, "currentblockweight", /*optional=*/true, "The block weight (including reserved weight for block header, txs count and coinbase tx) of the last assembled block (only present if a block was ever assembled)"},
                        {RPCResult::Type::NUM, "currentblocktx", /*optional=*/true, "The number of block transactions (excluding coinbase) of the last assembled block (only present if a block was ever assembled)"},
                        {RPCResult::Type::STR_HEX, "bits", "The current nBits, compact representation of the block difficulty target"},
                        {RPCResult::Type::OBJ, "difficulty", "The current difficulty",
                        {
                            {RPCResult::Type::NUM, "proof-of-work", "Coinbase difficulty"},
                            {RPCResult::Type::NUM, "proof-of-stake", "Coinstake difficulty"},
                            {RPCResult::Type::NUM, "search-interval", "The search interval"},
                        }},
                        {RPCResult::Type::STR_HEX, "target", "The current target"},
                        {RPCResult::Type::NUM, "networkhashps", "The network hashes per second"},
                        {RPCResult::Type::NUM, "pooledtx", "The size of the mempool"},
                        {RPCResult::Type::STR_AMOUNT, "blockmintxfee", "Minimum feerate of packages selected for block inclusion in " + CURRENCY_UNIT + "/kvB"},
                        {RPCResult::Type::STR, "chain", "current network name (" LIST_CHAIN_NAMES ")"},
                        {RPCResult::Type::STR_HEX, "signet_challenge", /*optional=*/true, "The block challenge (aka. block script), in hexadecimal (only present if the current network is a signet)"},
                        {RPCResult::Type::OBJ, "next", "The next block",
                        {
                            {RPCResult::Type::NUM, "height", "The next height"},
                            {RPCResult::Type::STR_HEX, "bits", "The next target nBits"},
                            {RPCResult::Type::NUM, "difficulty", "The next difficulty"},
                            {RPCResult::Type::STR_HEX, "target", "The next target"}
                        }},
                        (IsDeprecatedRPCEnabled("warnings") ?
                            RPCResult{RPCResult::Type::STR, "warnings", "any network and blockchain warnings (DEPRECATED)"} :
                            RPCResult{RPCResult::Type::ARR, "warnings", "any network and blockchain warnings (run with `-deprecatedrpc=warnings` to return the latest warning as a single string)",
                            {
                                {RPCResult::Type::STR, "", "warning"},
                            }
                            }
                        ),
                        {RPCResult::Type::NUM, "blockvalue", "The block subsidy"},
                        {RPCResult::Type::NUM, "netmhashps", "Network PoW hash power"},
                        {RPCResult::Type::NUM, "netstakeweight", "Network stake weight"},
                        {RPCResult::Type::STR, "errors", "Error messages"},
                        {RPCResult::Type::OBJ, "stakeweight", "The stake weight",
                        {
                            {RPCResult::Type::NUM, "minimum", "The minimum stake weight"},
                            {RPCResult::Type::NUM, "maximum", "The maximum stake weight"},
                            {RPCResult::Type::NUM, "combined", "The combined stake weight"},
                        }},
                    }},
                RPCExamples{
                    HelpExampleCli("getmininginfo", "")
            + HelpExampleRpc("getmininginfo", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    node::NodeContext& node = *pwallet->chain().context();
    const CTxMemPool& mempool = pwallet->chain().mempool();
    ChainstateManager& chainman = pwallet->chain().chainman();

    uint64_t nWeight = 0;
    uint64_t lastCoinStakeSearchInterval = 0;
    if (pwallet)
    {
        LOCK(pwallet->cs_wallet);
        nWeight = pwallet->GetStakeWeight();
        lastCoinStakeSearchInterval = pwallet->m_last_coin_stake_search_interval;
    }

    LOCK(cs_main);
    const CChain& active_chain = chainman.ActiveChain();
    CBlockIndex& tip{*CHECK_NONFATAL(active_chain.Tip())};

    UniValue obj(UniValue::VOBJ);
    UniValue diff(UniValue::VOBJ);
    UniValue weight(UniValue::VOBJ);

    obj.pushKV("blocks",           active_chain.Height());
    if (BlockAssembler::m_last_block_weight) obj.pushKV("currentblockweight", *BlockAssembler::m_last_block_weight);
    if (BlockAssembler::m_last_block_num_txs) obj.pushKV("currentblocktx", *BlockAssembler::m_last_block_num_txs);

    obj.pushKV("bits", strprintf("%08x", tip.nBits));
    diff.pushKV("proof-of-work",   GetDifficulty(*CHECK_NONFATAL(GetLastBlockIndex(chainman.m_best_header, false))));
    diff.pushKV("proof-of-stake",  GetDifficulty(*CHECK_NONFATAL(GetLastBlockIndex(chainman.m_best_header, true))));
    diff.pushKV("search-interval", (int)lastCoinStakeSearchInterval);
    obj.pushKV("difficulty",       diff);

    const Consensus::Params& consensusParams = Params().GetConsensus();
    obj.pushKV("blockvalue",    (uint64_t)GetBlockSubsidy(active_chain.Height(), consensusParams));

    obj.pushKV("netmhashps",       GetPoWMHashPS(chainman));
    obj.pushKV("netstakeweight",   GetPoSKernelPS(chainman));
    obj.pushKV("errors",           pwallet->chain().getWarnings().original);
    obj.pushKV("target", GetTarget(tip, chainman.GetConsensus().powLimit).GetHex());
    obj.pushKV("networkhashps",    GetReqNetworkHashPS(request, chainman));
    obj.pushKV("pooledtx",         (uint64_t)mempool.size());
    BlockAssembler::Options assembler_options;
    ApplyArgsManOptions(*node.args, assembler_options);
    obj.pushKV("blockmintxfee", ValueFromAmount(assembler_options.blockMinFeeRate.GetFeePerK()));


    weight.pushKV("minimum",       (uint64_t)nWeight);
    weight.pushKV("maximum",       (uint64_t)0);
    weight.pushKV("combined",      (uint64_t)nWeight);
    obj.pushKV("stakeweight",      weight);

    obj.pushKV("chain", chainman.GetParams().GetChainTypeString());

    UniValue next(UniValue::VOBJ);
    CBlockIndex next_index;
    NextEmptyBlockIndex(tip, chainman.GetConsensus(), next_index);

    next.pushKV("height", next_index.nHeight);
    next.pushKV("bits", strprintf("%08x", next_index.nBits));
    next.pushKV("difficulty", GetDifficulty(next_index));
    next.pushKV("target", GetTarget(next_index, chainman.GetConsensus().powLimit).GetHex());
    obj.pushKV("next", next);

    if (chainman.GetParams().GetChainType() == ChainType::SIGNET) {
        const std::vector<uint8_t>& signet_challenge =
            chainman.GetConsensus().signet_challenge;
        obj.pushKV("signet_challenge", HexStr(signet_challenge));
    }
    obj.pushKV("warnings", node::GetWarningsForRpc(*CHECK_NONFATAL(node.warnings), IsDeprecatedRPCEnabled("warnings")));
    return obj;
},
    };
}

RPCHelpMan getstakinginfo()
{
    return RPCHelpMan{
        "getstakinginfo",
        "Returns an object containing staking-related information.",
                {},
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::BOOL, "enabled", "'true' if staking is enabled"},
                        {RPCResult::Type::BOOL, "staking", "'true' if wallet is currently staking"},
                        {RPCResult::Type::STR, "errors", "Error messages"},
                        {RPCResult::Type::NUM, "currentblocktx", /*optional=*/true, "The number of block transactions of the last assembled block (only present if a block was ever assembled)"},
                        {RPCResult::Type::NUM, "pooledtx", "The size of the mempool"},
                        {RPCResult::Type::NUM, "difficulty", "The current difficulty"},
                        {RPCResult::Type::NUM, "search-interval", "The staker search interval"},
                        {RPCResult::Type::NUM, "weight", "The staker weight"},
                        {RPCResult::Type::NUM, "delegateweight", "Delegate weight"},
                        {RPCResult::Type::NUM, "netstakeweight", "Network stake weight"},
                        {RPCResult::Type::NUM, "expectedtime", "Expected time to earn reward"},
                    }
                },
                RPCExamples{
                    HelpExampleCli("getstakinginfo", "")
            + HelpExampleRpc("getstakinginfo", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    const CTxMemPool& mempool = pwallet->chain().mempool();
    ChainstateManager& chainman = pwallet->chain().chainman();

    uint64_t nWeight = 0;
    uint64_t nStakerWeight = 0;
    uint64_t nDelegateWeight = 0;
    uint64_t lastCoinStakeSearchInterval = 0;

    if (pwallet)
    {
        LOCK(pwallet->cs_wallet);
        nWeight = pwallet->GetStakeWeight(&nStakerWeight, &nDelegateWeight);
        lastCoinStakeSearchInterval = pwallet->m_enabled_staking ? pwallet->m_last_coin_stake_search_interval : 0;
    }

    LOCK(cs_main);
    uint64_t nNetworkWeight = GetPoSKernelPS(chainman);
    bool staking = lastCoinStakeSearchInterval && nWeight;
    const Consensus::Params& consensusParams = Params().GetConsensus();
    int64_t nTargetSpacing = consensusParams.TargetSpacing(chainman.m_best_header->nHeight);
    uint64_t nExpectedTime = staking ? (nTargetSpacing * nNetworkWeight / nWeight) : 0;

    UniValue obj(UniValue::VOBJ);

    obj.pushKV("enabled", gArgs.GetBoolArg("-staking", true));
    obj.pushKV("staking", staking);
    obj.pushKV("errors", pwallet->chain().getWarnings().original);

    if (BlockAssembler::m_last_block_num_txs) obj.pushKV("currentblocktx", *BlockAssembler::m_last_block_num_txs);
    obj.pushKV("pooledtx", (uint64_t)mempool.size());

    obj.pushKV("difficulty", GetDifficulty(*CHECK_NONFATAL(GetLastBlockIndex(chainman.m_best_header, true))));
    obj.pushKV("search-interval", (int)lastCoinStakeSearchInterval);

    obj.pushKV("weight", (uint64_t)nStakerWeight);
    obj.pushKV("delegateweight", (uint64_t)nDelegateWeight);
    obj.pushKV("netstakeweight", (uint64_t)nNetworkWeight);

    obj.pushKV("expectedtime", nExpectedTime);

    return obj;
},
    };
}

std::span<const CRPCCommand> GetMiningRPCCommands()
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              actor (function)
  //  ------------------    ------------------------
    { "mining",             &getmininginfo,                  },
    { "mining",             &getstakinginfo,                 },
};
// clang-format on
    return commands;
}

} // namespace wallet
