// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <config/bitcoin-config.h> // IWYU pragma: keep

#include <chainparams.h>
#include <httpserver.h>
#include <index/blockfilterindex.h>
#include <index/coinstatsindex.h>
#include <index/txindex.h>
#include <interfaces/chain.h>
#include <interfaces/echo.h>
#include <interfaces/init.h>
#include <interfaces/ipc.h>
#include <kernel/cs_main.h>
#include <logging.h>
#include <node/context.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <scheduler.h>
#include <univalue.h>
#include <util/any.h>
#include <util/check.h>
#include <txmempool.h>
#include <validation.h>
#include <key_io.h>
#include <common/args.h>
#include <util/time.h>

#include <stdint.h>
#ifdef HAVE_MALLOC_INFO
#include <malloc.h>
#endif

using node::NodeContext;

static RPCHelpMan setmocktime()
{
    return RPCHelpMan{"setmocktime",
        "\nSet the local time to given timestamp (-regtest only)\n",
        {
            {"timestamp", RPCArg::Type::NUM, RPCArg::Optional::NO, UNIX_EPOCH_TIME + "\n"
             "Pass 0 to go back to using the system time."},
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if (!Params().IsMockableChain()) {
        throw std::runtime_error("setmocktime is for regression testing (-regtest mode) only");
    }

    // For now, don't change mocktime if we're in the middle of validation, as
    // this could have an effect on mempool time-based eviction, as well as
    // IsCurrentForFeeEstimation() and IsInitialBlockDownload().
    // TODO: figure out the right way to synchronize around mocktime, and
    // ensure all call sites of GetTime() are accessing this safely.
    LOCK(cs_main);

    const int64_t time{request.params[0].getInt<int64_t>()};
    constexpr int64_t max_time{Ticks<std::chrono::seconds>(std::chrono::nanoseconds::max())};
    if (time < 0 || time > max_time) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Mocktime must be in the range [0, %s], not %s.", max_time, time));
    }

    SetMockTime(time);
    const NodeContext& node_context{EnsureAnyNodeContext(request.context)};
    for (const auto& chain_client : node_context.chain_clients) {
        chain_client->setMockTime(time);
    }

    return UniValue::VNULL;
},
    };
}

static RPCHelpMan mockscheduler()
{
    return RPCHelpMan{"mockscheduler",
        "\nBump the scheduler into the future (-regtest only)\n",
        {
            {"delta_time", RPCArg::Type::NUM, RPCArg::Optional::NO, "Number of seconds to forward the scheduler into the future." },
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if (!Params().IsMockableChain()) {
        throw std::runtime_error("mockscheduler is for regression testing (-regtest mode) only");
    }

    int64_t delta_seconds = request.params[0].getInt<int64_t>();
    if (delta_seconds <= 0 || delta_seconds > 3600) {
        throw std::runtime_error("delta_time must be between 1 and 3600 seconds (1 hr)");
    }

    const NodeContext& node_context{EnsureAnyNodeContext(request.context)};
    CHECK_NONFATAL(node_context.scheduler)->MockForward(std::chrono::seconds{delta_seconds});
    CHECK_NONFATAL(node_context.validation_signals)->SyncWithValidationInterfaceQueue();
    for (const auto& chain_client : node_context.chain_clients) {
        chain_client->schedulerMockForward(std::chrono::seconds(delta_seconds));
    }

    return UniValue::VNULL;
},
    };
}

static UniValue RPCLockedMemoryInfo()
{
    LockedPool::Stats stats = LockedPoolManager::Instance().stats();
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("used", uint64_t(stats.used));
    obj.pushKV("free", uint64_t(stats.free));
    obj.pushKV("total", uint64_t(stats.total));
    obj.pushKV("locked", uint64_t(stats.locked));
    obj.pushKV("chunks_used", uint64_t(stats.chunks_used));
    obj.pushKV("chunks_free", uint64_t(stats.chunks_free));
    return obj;
}

#ifdef HAVE_MALLOC_INFO
static std::string RPCMallocInfo()
{
    char *ptr = nullptr;
    size_t size = 0;
    FILE *f = open_memstream(&ptr, &size);
    if (f) {
        malloc_info(0, f);
        fclose(f);
        if (ptr) {
            std::string rv(ptr, size);
            free(ptr);
            return rv;
        }
    }
    return "";
}
#endif

static RPCHelpMan getmemoryinfo()
{
    /* Please, avoid using the word "pool" here in the RPC interface or help,
     * as users will undoubtedly confuse it with the other "memory pool"
     */
    return RPCHelpMan{"getmemoryinfo",
                "Returns an object containing information about memory usage.\n",
                {
                    {"mode", RPCArg::Type::STR, RPCArg::Default{"stats"}, "determines what kind of information is returned.\n"
            "  - \"stats\" returns general statistics about memory usage in the daemon.\n"
            "  - \"mallocinfo\" returns an XML string describing low-level heap state (only available if compiled with glibc)."},
                },
                {
                    RPCResult{"mode \"stats\"",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::OBJ, "locked", "Information about locked memory manager",
                            {
                                {RPCResult::Type::NUM, "used", "Number of bytes used"},
                                {RPCResult::Type::NUM, "free", "Number of bytes available in current arenas"},
                                {RPCResult::Type::NUM, "total", "Total number of bytes managed"},
                                {RPCResult::Type::NUM, "locked", "Amount of bytes that succeeded locking. If this number is smaller than total, locking pages failed at some point and key data could be swapped to disk."},
                                {RPCResult::Type::NUM, "chunks_used", "Number allocated chunks"},
                                {RPCResult::Type::NUM, "chunks_free", "Number unused chunks"},
                            }},
                        }
                    },
                    RPCResult{"mode \"mallocinfo\"",
                        RPCResult::Type::STR, "", "\"<malloc version=\"1\">...\""
                    },
                },
                RPCExamples{
                    HelpExampleCli("getmemoryinfo", "")
            + HelpExampleRpc("getmemoryinfo", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string mode = request.params[0].isNull() ? "stats" : request.params[0].get_str();
    if (mode == "stats") {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("locked", RPCLockedMemoryInfo());
        return obj;
    } else if (mode == "mallocinfo") {
#ifdef HAVE_MALLOC_INFO
        return RPCMallocInfo();
#else
        throw JSONRPCError(RPC_INVALID_PARAMETER, "mallocinfo mode not available");
#endif
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "unknown mode " + mode);
    }
},
    };
}

static void EnableOrDisableLogCategories(UniValue cats, bool enable) {
    cats = cats.get_array();
    for (unsigned int i = 0; i < cats.size(); ++i) {
        std::string cat = cats[i].get_str();

        bool success;
        if (enable) {
            success = LogInstance().EnableCategory(cat);
        } else {
            success = LogInstance().DisableCategory(cat);
        }

        if (!success) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "unknown logging category " + cat);
        }
    }
}

static RPCHelpMan logging()
{
    return RPCHelpMan{"logging",
            "Gets and sets the logging configuration.\n"
            "When called without an argument, returns the list of categories with status that are currently being debug logged or not.\n"
            "When called with arguments, adds or removes categories from debug logging and return the lists above.\n"
            "The arguments are evaluated in order \"include\", \"exclude\".\n"
            "If an item is both included and excluded, it will thus end up being excluded.\n"
            "The valid logging categories are: " + LogInstance().LogCategoriesString() + "\n"
            "In addition, the following are available as category names with special meanings:\n"
            "  - \"all\",  \"1\" : represent all logging categories.\n"
            ,
                {
                    {"include", RPCArg::Type::ARR, RPCArg::Optional::OMITTED, "The categories to add to debug logging",
                        {
                            {"include_category", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "the valid logging category"},
                        }},
                    {"exclude", RPCArg::Type::ARR, RPCArg::Optional::OMITTED, "The categories to remove from debug logging",
                        {
                            {"exclude_category", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "the valid logging category"},
                        }},
                },
                RPCResult{
                    RPCResult::Type::OBJ_DYN, "", "keys are the logging categories, and values indicates its status",
                    {
                        {RPCResult::Type::BOOL, "category", "if being debug logged or not. false:inactive, true:active"},
                    }
                },
                RPCExamples{
                    HelpExampleCli("logging", "\"[\\\"all\\\"]\" \"[\\\"http\\\"]\"")
            + HelpExampleRpc("logging", "[\"all\"], [\"libevent\"]")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    uint64_t original_log_categories = LogInstance().GetCategoryMask();
    if (request.params[0].isArray()) {
        EnableOrDisableLogCategories(request.params[0], true);
    }
    if (request.params[1].isArray()) {
        EnableOrDisableLogCategories(request.params[1], false);
    }
    uint64_t updated_log_categories = LogInstance().GetCategoryMask();
    uint64_t changed_log_categories = original_log_categories ^ updated_log_categories;

    // Update libevent logging if BCLog::LIBEVENT has changed.
    if (changed_log_categories & BCLog::LIBEVENT) {
        UpdateHTTPServerLogging(LogInstance().WillLogCategory(BCLog::LIBEVENT));
    }

    UniValue result(UniValue::VOBJ);
    for (const auto& logCatActive : LogInstance().LogCategoriesList()) {
        result.pushKV(logCatActive.category, logCatActive.active);
    }

    return result;
},
    };
}

static RPCHelpMan echo(const std::string& name)
{
    return RPCHelpMan{name,
                "\nSimply echo back the input arguments. This command is for testing.\n"
                "\nIt will return an internal bug report when arg9='trigger_internal_bug' is passed.\n"
                "\nThe difference between echo and echojson is that echojson has argument conversion enabled in the client-side table in "
                "qtum-cli and the GUI. There is no server-side difference.",
        {
            {"arg0", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
            {"arg1", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
            {"arg2", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
            {"arg3", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
            {"arg4", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
            {"arg5", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
            {"arg6", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
            {"arg7", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
            {"arg8", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
            {"arg9", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
        },
                RPCResult{RPCResult::Type::ANY, "", "Returns whatever was passed in"},
                RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if (request.params[9].isStr()) {
        CHECK_NONFATAL(request.params[9].get_str() != "trigger_internal_bug");
    }

    return request.params;
},
    };
}

static RPCHelpMan echo() { return echo("echo"); }
static RPCHelpMan echojson() { return echo("echojson"); }

static RPCHelpMan echoipc()
{
    return RPCHelpMan{
        "echoipc",
        "\nEcho back the input argument, passing it through a spawned process in a multiprocess build.\n"
        "This command is for testing.\n",
        {{"arg", RPCArg::Type::STR, RPCArg::Optional::NO, "The string to echo",}},
        RPCResult{RPCResult::Type::STR, "echo", "The echoed string."},
        RPCExamples{HelpExampleCli("echo", "\"Hello world\"") +
                    HelpExampleRpc("echo", "\"Hello world\"")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            interfaces::Init& local_init = *EnsureAnyNodeContext(request.context).init;
            std::unique_ptr<interfaces::Echo> echo;
            if (interfaces::Ipc* ipc = local_init.ipc()) {
                // Spawn a new bitcoin-node process and call makeEcho to get a
                // client pointer to a interfaces::Echo instance running in
                // that process. This is just for testing. A slightly more
                // realistic test spawning a different executable instead of
                // the same executable would add a new bitcoin-echo executable,
                // and spawn bitcoin-echo below instead of bitcoin-node. But
                // using bitcoin-node avoids the need to build and install a
                // new executable just for this one test.
                auto init = ipc->spawnProcess("qtum-node");
                echo = init->makeEcho();
                ipc->addCleanup(*echo, [init = init.release()] { delete init; });
            } else {
                // IPC support is not available because this is a bitcoind
                // process not a bitcoind-node process, so just create a local
                // interfaces::Echo object and return it so the `echoipc` RPC
                // method will work, and the python test calling `echoipc`
                // can expect the same result.
                echo = local_init.makeEcho();
            }
            return echo->echo(request.params[0].get_str());
        },
    };
}

static UniValue SummaryToJSON(const IndexSummary&& summary, std::string index_name)
{
    UniValue ret_summary(UniValue::VOBJ);
    if (!index_name.empty() && index_name != summary.name) return ret_summary;

    UniValue entry(UniValue::VOBJ);
    entry.pushKV("synced", summary.synced);
    entry.pushKV("best_block_height", summary.best_block_height);
    ret_summary.pushKV(summary.name, std::move(entry));
    return ret_summary;
}

static RPCHelpMan getindexinfo()
{
    return RPCHelpMan{"getindexinfo",
                "\nReturns the status of one or all available indices currently running in the node.\n",
                {
                    {"index_name", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Filter results for an index with a specific name."},
                },
                RPCResult{
                    RPCResult::Type::OBJ_DYN, "", "", {
                        {
                            RPCResult::Type::OBJ, "name", "The name of the index",
                            {
                                {RPCResult::Type::BOOL, "synced", "Whether the index is synced or not"},
                                {RPCResult::Type::NUM, "best_block_height", "The block height to which the index is synced"},
                            }
                        },
                    },
                },
                RPCExamples{
                    HelpExampleCli("getindexinfo", "")
                  + HelpExampleRpc("getindexinfo", "")
                  + HelpExampleCli("getindexinfo", "txindex")
                  + HelpExampleRpc("getindexinfo", "txindex")
                },
                [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    UniValue result(UniValue::VOBJ);
    const std::string index_name = request.params[0].isNull() ? "" : request.params[0].get_str();

    if (g_txindex) {
        result.pushKVs(SummaryToJSON(g_txindex->GetSummary(), index_name));
    }

    if (g_coin_stats_index) {
        result.pushKVs(SummaryToJSON(g_coin_stats_index->GetSummary(), index_name));
    }

    ForEachBlockFilterIndex([&result, &index_name](const BlockFilterIndex& index) {
        result.pushKVs(SummaryToJSON(index.GetSummary(), index_name));
    });

    return result;
},
    };
}

static RPCHelpMan getdgpinfo()
{
    return RPCHelpMan{"getdgpinfo",
                "\nReturns an object containing DGP state info.\n",
                {},
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::NUM, "maxblocksize", "Current maximum block size"},
                        {RPCResult::Type::NUM, "mingasprice", "Current minimum gas price"},
                        {RPCResult::Type::NUM, "blockgaslimit", "Current block gas limit"},
                    }
                },
                RPCExamples{
                    HelpExampleCli("getdgpinfo", "")
            + HelpExampleRpc("getdgpinfo", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    LOCK(cs_main);

    CChain& active_chain = chainman.ActiveChain();
    QtumDGP qtumDGP(globalState.get(), chainman.ActiveChainstate());

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("maxblocksize", (uint64_t)qtumDGP.getBlockSize(active_chain.Height()));
    obj.pushKV("mingasprice", (uint64_t)qtumDGP.getMinGasPrice(active_chain.Height()));
    obj.pushKV("blockgaslimit", (uint64_t)qtumDGP.getBlockGasLimit(active_chain.Height()));

    return obj;
},
    };
}

static RPCHelpMan getblockhashes()
{
    return RPCHelpMan{"getblockhashes",
                "\nReturns array of hashes of blocks within the timestamp range provided.\n",
                {
                    {"high", RPCArg::Type::NUM, RPCArg::Optional::NO, "The newer block timestamp"},
                    {"low", RPCArg::Type::NUM, RPCArg::Optional::NO, "The older block timestamp"},
                    {"options", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "An object with options",
                        {
                            {"noOrphans", RPCArg::Type::BOOL, RPCArg::Default{"false"}, "Will only include blocks on the main chain"},
                            {"logicalTimes", RPCArg::Type::BOOL, RPCArg::Default{"false"}, "Will include logical timestamps with hashes"},
                        },
                    },
                },
                {
                    RPCResult{"if logicalTimes is set to false",
                        RPCResult::Type::ARR, "", "",
                        {
                            {RPCResult::Type::STR_HEX, "", "The block hash"}
                        },
                    },
                    RPCResult{"if logicalTimes is set to true",
                        RPCResult::Type::ARR, "", "",
                        {
                            {RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR_HEX, "blockhash", "The block hash"},
                                {RPCResult::Type::NUM, "logicalts", "The logical timestamp"},
                            }}
                        },
                    },
                },
                RPCExamples{
                    HelpExampleCli("getblockhashes", "1231614698 1231024505")
                    + HelpExampleCli("getblockhashes", "1231614698 1231024505 '{\"noOrphans\":false, \"logicalTimes\":true}'")
            + HelpExampleRpc("getblockhashes", "1231614698, 1231024505")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    ChainstateManager& chainman = EnsureAnyChainman(request.context);

    unsigned int high = request.params[0].getInt<int>();
    unsigned int low = request.params[1].getInt<int>();
    bool fActiveOnly = false;
    bool fLogicalTS = false;

    if (!request.params[2].isNull()) {
        if (request.params[2].isObject()) {
            UniValue noOrphans = request.params[2].get_obj().find_value("noOrphans");
            UniValue returnLogical = request.params[2].get_obj().find_value("logicalTimes");

            if (noOrphans.isBool())
                fActiveOnly = noOrphans.get_bool();

            if (returnLogical.isBool())
                fLogicalTS = returnLogical.get_bool();
        }
    }

    std::vector<std::pair<uint256, unsigned int> > blockHashes;
    bool found = false;

    found = GetTimestampIndex(high, low, fActiveOnly, blockHashes, chainman);

    if (!found) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for block hashes");
    }

    UniValue result(UniValue::VARR);

    for (std::vector<std::pair<uint256, unsigned int> >::const_iterator it=blockHashes.begin(); it!=blockHashes.end(); it++) {
        if (fLogicalTS) {
            UniValue item(UniValue::VOBJ);
            item.pushKV("blockhash", it->first.GetHex());
            item.pushKV("logicalts", (int)it->second);
            result.push_back(item);
        } else {
            result.push_back(it->first.GetHex());
        }
    }

    return result;
},
    };
}

bool getAddressesFromParams(const UniValue& params, std::vector<std::pair<uint256, int> > &addresses)
{
    if (params[0].isStr()) {
        uint256 hashBytes;
        int type = 0;
        if (!DecodeIndexKey(params[0].get_str(), hashBytes, type)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
        }
        addresses.push_back(std::make_pair(hashBytes, type));
    } else if (params[0].isObject()) {

        UniValue addressValues = params[0].get_obj().find_value("addresses");
        if (!addressValues.isArray()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Addresses is expected to be an array");
        }

        std::vector<UniValue> values = addressValues.getValues();

        for (std::vector<UniValue>::iterator it = values.begin(); it != values.end(); ++it) {

            uint256 hashBytes;
            int type = 0;
            if (!DecodeIndexKey(it->get_str(), hashBytes, type)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            }
            addresses.push_back(std::make_pair(hashBytes, type));
        }
    } else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    return true;
}

bool heightSort(std::pair<CAddressUnspentKey, CAddressUnspentValue> a,
                std::pair<CAddressUnspentKey, CAddressUnspentValue> b) {
    return a.second.blockHeight < b.second.blockHeight;
}

bool timestampSort(std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> a,
                   std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> b) {
    return a.second.time < b.second.time;
}

bool getAddressFromIndex(const int &type, const uint256 &hash, std::string &address)
{
    if (type == 2) {
        std::vector<unsigned char> addressBytes(hash.begin(), hash.begin() + 20);
        address = EncodeDestination(ScriptHash(uint160(addressBytes)));
    } else if (type == 1) {
        std::vector<unsigned char> addressBytes(hash.begin(), hash.begin() + 20);
        address = EncodeDestination(PKHash(uint160(addressBytes)));
    } else if (type == 3) {
        address = EncodeDestination(WitnessV0ScriptHash(hash));
    } else if (type == 4) {
        std::vector<unsigned char> addressBytes(hash.begin(), hash.begin() + 20);
        address = EncodeDestination(WitnessV0KeyHash(uint160(addressBytes)));
    } else if (type == 5) {
        WitnessV1Taproot tap;
        std::copy(hash.begin(), hash.end(), tap.begin());
        address = EncodeDestination(tap);
    } else {
        return false;
    }
    return true;
}

static RPCHelpMan getaddressdeltas()
{
    return RPCHelpMan{"getaddressdeltas",
            "\nReturns all changes for an address (requires addressindex to be enabled).\n",
            {
                {"argument", RPCArg::Type::OBJ, RPCArg::Optional::NO, "Json object",
                    {
                        {"addresses", RPCArg::Type::ARR, RPCArg::Optional::NO, "The qtum addresses",
                            {
                                {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "The qtum address"},
                            }
                        },
                        {"start", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "The start block height"},
                        {"end", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "The end block height"},
                        {"chainInfo", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Include chain info in results, only applies if start and end specified"},
                    }
                }
            },
            {
                RPCResult{"if chainInfo is set to false",
                    RPCResult::Type::ARR, "", "",
                    {
                        {RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::NUM, "satoshis", "The difference of satoshis"},
                            {RPCResult::Type::STR_HEX, "txid", "The related txid"},
                            {RPCResult::Type::NUM, "index", "The related input or output index"},
                            {RPCResult::Type::NUM, "blockindex", "The transaction index in block"},
                            {RPCResult::Type::NUM, "height", "The block height"},
                            {RPCResult::Type::STR, "address", "The qtum address"},
                        }}
                    },
                },
                RPCResult{"if chainInfo is set to true",
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::ARR, "deltas", "List of delta",
                        {
                            {RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::NUM, "satoshis", "The difference of satoshis"},
                                {RPCResult::Type::STR_HEX, "txid", "The related txid"},
                                {RPCResult::Type::NUM, "index", "The related input or output index"},
                                {RPCResult::Type::NUM, "blockindex", "The transaction index in block"},
                                {RPCResult::Type::NUM, "height", "The block height"},
                                {RPCResult::Type::STR, "address", "The qtum address"},
                            }}
                        }},
                        {RPCResult::Type::OBJ, "start", "Start block",
                        {
                            {RPCResult::Type::STR_HEX, "hash", "The block hash"},
                            {RPCResult::Type::NUM, "height", "The block height"},
                        }},
                        {RPCResult::Type::OBJ, "end", "End block",
                        {
                            {RPCResult::Type::STR_HEX, "hash", "The block hash"},
                            {RPCResult::Type::NUM, "height", "The block height"},
                        }},
                    },
                },
            },
            RPCExamples{
                HelpExampleCli("getaddressdeltas", "'{\"addresses\": [\"QD1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"]}'")
        + HelpExampleRpc("getaddressdeltas", "{\"addresses\": [\"QD1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"]}") +
                HelpExampleCli("getaddressdeltas", "'{\"addresses\": [\"QD1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"], \"start\": 5000, \"end\": 5500, \"chainInfo\": true}'")
        + HelpExampleRpc("getaddressdeltas", "{\"addresses\": [\"QD1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"], \"start\": 5000, \"end\": 5500, \"chainInfo\": true}")
            },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    ChainstateManager& chainman = EnsureAnyChainman(request.context);

    UniValue startValue = request.params[0].get_obj().find_value("start");
    UniValue endValue = request.params[0].get_obj().find_value("end");

    UniValue chainInfo = request.params[0].get_obj().find_value("chainInfo");
    bool includeChainInfo = false;
    if (chainInfo.isBool()) {
        includeChainInfo = chainInfo.get_bool();
    }

    int start = 0;
    int end = 0;

    if (startValue.isNum() && endValue.isNum()) {
        start = startValue.getInt<int>();
        end = endValue.getInt<int>();
        if (start <= 0 || end <= 0) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Start and end is expected to be greater than zero");
        }
        if (end < start) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "End value is expected to be greater than start");
        }
    }

    std::vector<std::pair<uint256, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;

    for (std::vector<std::pair<uint256, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (start > 0 && end > 0) {
            if (!GetAddressIndex((*it).first, (*it).second, addressIndex, chainman.m_blockman, start, end)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        } else {
            if (!GetAddressIndex((*it).first, (*it).second, addressIndex, chainman.m_blockman)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        }
    }

    UniValue deltas(UniValue::VARR);

    for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
        std::string address;
        if (!getAddressFromIndex(it->first.type, it->first.hashBytes, address)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
        }

        UniValue delta(UniValue::VOBJ);
        delta.pushKV("satoshis", it->second);
        delta.pushKV("txid", it->first.txhash.GetHex());
        delta.pushKV("index", (int)it->first.index);
        delta.pushKV("blockindex", (int)it->first.txindex);
        delta.pushKV("height", it->first.blockHeight);
        delta.pushKV("address", address);
        deltas.push_back(delta);
    }

    UniValue result(UniValue::VOBJ);

    if (includeChainInfo && start > 0 && end > 0) {
        ChainstateManager& chainman = EnsureAnyChainman(request.context);
        LOCK(cs_main);

        CChain& active_chain = chainman.ActiveChain();
        if (start > active_chain.Height() || end > active_chain.Height()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Start or end is outside chain range");
        }

        CBlockIndex* startIndex = active_chain[start];
        CBlockIndex* endIndex = active_chain[end];

        UniValue startInfo(UniValue::VOBJ);
        UniValue endInfo(UniValue::VOBJ);

        startInfo.pushKV("hash", startIndex->GetBlockHash().GetHex());
        startInfo.pushKV("height", start);

        endInfo.pushKV("hash", endIndex->GetBlockHash().GetHex());
        endInfo.pushKV("height", end);

        result.pushKV("deltas", deltas);
        result.pushKV("start", startInfo);
        result.pushKV("end", endInfo);

        return result;
    } else {
        return deltas;
    }
},
    };
}

static RPCHelpMan getaddressbalance()
{
    return RPCHelpMan{"getaddressbalance",
                "\nReturns the balance for an address(es) (requires addressindex to be enabled).\n",
                {
                    {"argument", RPCArg::Type::OBJ, RPCArg::Optional::NO, "Json object",
                        {
                            {"addresses", RPCArg::Type::ARR, RPCArg::Optional::NO, "The qtum addresses",
                                {
                                    {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "The qtum address"},
                                }
                            },
                        }
                    }
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::NUM, "balance", "The current balance in satoshis"},
                        {RPCResult::Type::NUM, "received", "The total number of satoshis received (including change)"},
                        {RPCResult::Type::NUM, "immature", "The immature balance in satoshis"},
                    }
                },
                RPCExamples{
                    HelpExampleCli("getaddressbalance", "'{\"addresses\": [\"QD1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"]}'")
            + HelpExampleRpc("getaddressbalance", "{\"addresses\": [\"QD1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"]}")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    ChainstateManager& chainman = EnsureAnyChainman(request.context);

    std::vector<std::pair<uint256, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;

    for (std::vector<std::pair<uint256, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (!GetAddressIndex((*it).first, (*it).second, addressIndex, chainman.m_blockman)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
        }
    }

    CAmount balance = 0;
    CAmount received = 0;
    CAmount immature = 0;

    LOCK(cs_main);
    CChain& active_chain = chainman.ActiveChain();
    for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
        if (it->second > 0) {
            received += it->second;
        }
        balance += it->second;
        int nHeight = active_chain.Height();
        if (it->first.txindex == 1 && ((nHeight - it->first.blockHeight) < Params().GetConsensus().CoinbaseMaturity(nHeight)))
            immature += it->second; //immature stake outputs
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("balance", balance);
    result.pushKV("received", received);
    result.pushKV("immature", immature);

    return result;
},
    };
}

static RPCHelpMan getaddressutxos()
{
    return RPCHelpMan{"getaddressutxos",
                "\nReturns all unspent outputs for an address (requires addressindex to be enabled).\n",
                {
                    {"argument", RPCArg::Type::OBJ, RPCArg::Optional::NO, "Json object",
                        {
                            {"addresses", RPCArg::Type::ARR, RPCArg::Optional::NO, "The qtum addresses",
                                {
                                    {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "The qtum address"},
                                }
                            },
                            {"chainInfo", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Include chain info with results"},
                        }
                    }
                },
                {
                    RPCResult{"if chainInfo is set to false",
                        RPCResult::Type::ARR, "", "",
                        {
                            {RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR, "address", "The address base58check encoded"},
                                {RPCResult::Type::STR_HEX, "txid", "The output txid"},
                                {RPCResult::Type::NUM, "height", "The block height"},
                                {RPCResult::Type::NUM, "outputIndex", "The output index"},
                                {RPCResult::Type::STR_HEX, "script", "The script hex encoded"},
                                {RPCResult::Type::NUM, "satoshis", "The number of satoshis of the output"},
                                {RPCResult::Type::BOOL, "isStake", "Is coinstake output"},
                            }}
                        },
                    },
                    RPCResult{"if chainInfo is set to true",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::ARR, "utxos", "List of utxo",
                            {
                                {RPCResult::Type::OBJ, "", "",
                                {
                                    {RPCResult::Type::STR, "address", "The address base58check encoded"},
                                    {RPCResult::Type::STR_HEX, "txid", "The output txid"},
                                    {RPCResult::Type::NUM, "height", "The block height"},
                                    {RPCResult::Type::NUM, "outputIndex", "The output index"},
                                    {RPCResult::Type::STR_HEX, "script", "The script hex encoded"},
                                    {RPCResult::Type::NUM, "satoshis", "The number of satoshis of the output"},
                                    {RPCResult::Type::BOOL, "isStake", "Is coinstake output"},
                                }}
                            }},
                            {RPCResult::Type::STR_HEX, "hash", "The tip block hash"},
                            {RPCResult::Type::NUM, "height", "The tip block height"},
                        },
                    },
                },
                RPCExamples{
                    HelpExampleCli("getaddressutxos", "'{\"addresses\": [\"QD1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"]}'")
            + HelpExampleRpc("getaddressutxos", "{\"addresses\": [\"QD1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"]}") +
                    HelpExampleCli("getaddressutxos", "'{\"addresses\": [\"QD1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"], \"chainInfo\": true}'")
            + HelpExampleRpc("getaddressutxos", "{\"addresses\": [\"QD1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"], \"chainInfo\": true}")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    ChainstateManager& chainman = EnsureAnyChainman(request.context);

    bool includeChainInfo = false;
    if (request.params[0].isObject()) {
        UniValue chainInfo = request.params[0].get_obj().find_value("chainInfo");
        if (chainInfo.isBool()) {
            includeChainInfo = chainInfo.get_bool();
        }
    }

    std::vector<std::pair<uint256, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;

    for (std::vector<std::pair<uint256, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (!GetAddressUnspent((*it).first, (*it).second, unspentOutputs, chainman.m_blockman)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
        }
    }

    std::sort(unspentOutputs.begin(), unspentOutputs.end(), heightSort);

    UniValue utxos(UniValue::VARR);

    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it=unspentOutputs.begin(); it!=unspentOutputs.end(); it++) {
        UniValue output(UniValue::VOBJ);
        std::string address;
        if (!getAddressFromIndex(it->first.type, it->first.hashBytes, address)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
        }

        output.pushKV("address", address);
        output.pushKV("txid", it->first.txhash.GetHex());
        output.pushKV("outputIndex", (int)it->first.index);
        output.pushKV("script", HexStr(MakeUCharSpan(it->second.script)));
        output.pushKV("satoshis", it->second.satoshis);
        output.pushKV("height", it->second.blockHeight);
        output.pushKV("isStake", it->second.coinStake);
        utxos.push_back(output);
    }

    if (includeChainInfo) {
        UniValue result(UniValue::VOBJ);
        result.pushKV("utxos", utxos);

        ChainstateManager& chainman = EnsureAnyChainman(request.context);
        LOCK(cs_main);
        CChain& active_chain = chainman.ActiveChain();
        result.pushKV("hash", active_chain.Tip()->GetBlockHash().GetHex());
        result.pushKV("height", (int)active_chain.Height());
        return result;
    } else {
        return utxos;
    }
},
    };
}

static RPCHelpMan getaddressmempool()
{
    return RPCHelpMan{"getaddressmempool",
                "\nReturns all mempool deltas for an address (requires addressindex to be enabled).\n",
                {
                    {"argument", RPCArg::Type::OBJ, RPCArg::Optional::NO, "Json object",
                        {
                            {"addresses", RPCArg::Type::ARR, RPCArg::Optional::NO, "The qtum addresses",
                                {
                                    {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "The qtum address"},
                                }
                            },
                        }
                    }
                },
                RPCResult{
                    RPCResult::Type::ARR, "", "",
                    {
                        {RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR, "address", "The qtum address"},
                            {RPCResult::Type::STR_HEX, "txid", "The related txid"},
                            {RPCResult::Type::NUM, "index", "The related input or output index"},
                            {RPCResult::Type::NUM, "satoshis", "The difference of satoshis"},
                            {RPCResult::Type::NUM, "timestamp", "The time the transaction entered the mempool (seconds)"},
                            {RPCResult::Type::STR_HEX, "prevtxid", /*optional=*/true, "The previous txid (if spending)"},
                            {RPCResult::Type::NUM, "prevout", /*optional=*/true, "The previous transaction output index (if spending)"},
                        }}
                    }
                },
                RPCExamples{
                    HelpExampleCli("getaddressmempool", "'{\"addresses\": [\"QD1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"]}'")
            + HelpExampleRpc("getaddressmempool", "{\"addresses\": [\"QD1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"]}")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    std::vector<std::pair<uint256, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> > indexes;

    if (!node.mempool->getAddressIndex(addresses, indexes)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
    }

    std::sort(indexes.begin(), indexes.end(), timestampSort);

    UniValue result(UniValue::VARR);

    for (std::vector<std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> >::iterator it = indexes.begin();
         it != indexes.end(); it++) {

        std::string address;
        if (!getAddressFromIndex(it->first.type, it->first.addressBytes, address)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
        }

        UniValue delta(UniValue::VOBJ);
        delta.pushKV("address", address);
        delta.pushKV("txid", it->first.txhash.GetHex());
        delta.pushKV("index", (int)it->first.index);
        delta.pushKV("satoshis", it->second.amount);
        delta.pushKV("timestamp", it->second.time);
        if (it->second.amount < 0) {
            delta.pushKV("prevtxid", it->second.prevhash.GetHex());
            delta.pushKV("prevout", (int)it->second.prevout);
        }
        result.push_back(delta);
    }

    return result;
},
    };
}

static RPCHelpMan getspentinfo()
{
    return RPCHelpMan{"getspentinfo",
                "\nReturns the txid and index where an output is spent.\n",
                {
                    {"argument", RPCArg::Type::OBJ, RPCArg::Optional::NO, "Transaction data",
                        {
                            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hex string of the txid"},
                            {"index", RPCArg::Type::NUM, RPCArg::Optional::NO, "The start block height"},
                        },
                    },
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
                        {RPCResult::Type::NUM, "index", "The spending input index"},
                        {RPCResult::Type::NUM, "height", "The spending block height"},
                    }
                },
                RPCExamples{
                    HelpExampleCli("getspentinfo", "'{\"txid\": \"0437cd7f8525ceed2324359c2d0ba26006d92d856a9c20fa0241106ee5a597c9\", \"index\": 0}'")
            + HelpExampleRpc("getspentinfo", "{\"txid\": \"0437cd7f8525ceed2324359c2d0ba26006d92d856a9c20fa0241106ee5a597c9\", \"index\": 0}")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const CTxMemPool& mempool = EnsureMemPool(node);
    ChainstateManager& chainman = EnsureAnyChainman(request.context);

    UniValue txidValue = request.params[0].get_obj().find_value("txid");
    UniValue indexValue = request.params[0].get_obj().find_value("index");

    if (!txidValue.isStr() || !indexValue.isNum()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid txid or index");
    }

    uint256 txid = ParseHashV(txidValue, "txid");
    int outputIndex = indexValue.getInt<int>();

    CSpentIndexKey key(txid, outputIndex);
    CSpentIndexValue value;

    if (!GetSpentIndex(key, value, mempool, chainman.m_blockman)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unable to get spent info");
    }

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("txid", value.txid.GetHex());
    obj.pushKV("index", (int)value.inputIndex);
    obj.pushKV("height", value.blockHeight);

    return obj;
},
    };
}

static RPCHelpMan getaddresstxids()
{
    return RPCHelpMan{"getaddresstxids",
                "\nReturns the txids for an address(es) (requires addressindex to be enabled).\n",
                {
                    {"argument", RPCArg::Type::OBJ, RPCArg::Optional::NO, "Json object",
                        {
                            {"addresses", RPCArg::Type::ARR, RPCArg::Optional::NO, "The qtum addresses",
                                {
                                    {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "The qtum address"},
                                }
                            },
                            {"start", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "The start block height"},
                            {"end", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "The end block height"},
                        }
                    }
                },
                RPCResult{
                    RPCResult::Type::ARR, "", "",
                    {
                        {RPCResult::Type::STR_HEX, "transactionid", "The transaction id"},
                    }
                },
                RPCExamples{
                    HelpExampleCli("getaddresstxids", "'{\"addresses\": [\"QD1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"]}'")
            + HelpExampleRpc("getaddresstxids", "{\"addresses\": [\"QD1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"]}") +
                    HelpExampleCli("getaddresstxids", "'{\"addresses\": [\"QD1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"], \"start\": 5000, \"end\": 5500}'")
            + HelpExampleRpc("getaddresstxids", "{\"addresses\": [\"QD1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"], \"start\": 5000, \"end\": 5500}")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    ChainstateManager& chainman = EnsureAnyChainman(request.context);

    std::vector<std::pair<uint256, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    int start = 0;
    int end = 0;
    if (request.params[0].isObject()) {
        UniValue startValue = request.params[0].get_obj().find_value("start");
        UniValue endValue = request.params[0].get_obj().find_value("end");
        if (startValue.isNum() && endValue.isNum()) {
            start = startValue.getInt<int>();
            end = endValue.getInt<int>();
        }
    }

    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;

    for (std::vector<std::pair<uint256, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (start > 0 && end > 0) {
            if (!GetAddressIndex((*it).first, (*it).second, addressIndex, chainman.m_blockman, start, end)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        } else {
            if (!GetAddressIndex((*it).first, (*it).second, addressIndex, chainman.m_blockman)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        }
    }

    std::set<std::pair<int, std::string> > txids;
    UniValue result(UniValue::VARR);

    for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
        int height = it->first.blockHeight;
        std::string txid = it->first.txhash.GetHex();

        if (addresses.size() > 1) {
            txids.insert(std::make_pair(height, txid));
        } else {
            if (txids.insert(std::make_pair(height, txid)).second) {
                result.push_back(txid);
            }
        }
    }

    if (addresses.size() > 1) {
        for (std::set<std::pair<int, std::string> >::const_iterator it=txids.begin(); it!=txids.end(); it++) {
            result.push_back(it->second);
        }
    }

    return result;
},
    };
}

std::vector<std::string> getListArgsType()
{
    std::vector<std::string> ret = { "-rpcwallet",
                                     "-rpcauth",
                                     "-rpcwhitelist",
                                     "-rpcallowip",
                                     "-rpcbind",
                                     "-blockfilterindex",
                                     "-whitebind",
                                     "-bind",
                                     "-debug",
                                     "-debugexclude",
                                     "-stakingallowlist",
                                     "-stakingexcludelist",
                                     "-uacomment",
                                     "-onlynet",
                                     "-externalip",
                                     "-loadblock",
                                     "-addnode",
                                     "-whitelist",
                                     "-seednode",
                                     "-connect",
                                     "-deprecatedrpc",
                                     "-wallet" };
    return ret;
}

static RPCHelpMan listconf()
{
    return RPCHelpMan{"listconf",
                "\nReturns the current options that qtumd was started with.\n",
                {},
                RPCResult{
                    RPCResult::Type::OBJ_DYN, "", "",
                    {
                        {RPCResult::Type::STR, "param", "Value for param"},
                    }
                },
                RPCExamples{
                    HelpExampleCli("listconf", "")
            + HelpExampleRpc("listconf", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    UniValue ret(UniValue::VOBJ);

    std::vector<std::string> paramListType = getListArgsType();
    for (const auto& arg : gArgs.getArgsList(paramListType)) {
        UniValue listValues(UniValue::VARR);
        for (const auto& value : arg.second) {
            std::optional<unsigned int> flags = gArgs.GetArgFlags('-' + arg.first);
            if (flags) {
                UniValue value_param = (*flags & gArgs.SENSITIVE) ? "****" : value;
                listValues.push_back(value_param);
            }
        }

        int size = listValues.size();
        if(size > 0)
        {
            ret.pushKV(arg.first, size == 1 ? listValues[0] : listValues);
        }
    }
    return ret;
},
    };
}

void RegisterNodeRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        {"control", &getmemoryinfo},
        {"control", &logging},
        {"control", &getdgpinfo},
        {"util", &getindexinfo},
        {"util", &getblockhashes},
        {"util", &getaddresstxids},
        {"util", &getaddressdeltas},
        {"util", &getaddressbalance},
        {"util", &getaddressutxos},
        {"util", &getaddressmempool},
        {"util", &getspentinfo},
        {"util", &listconf},
        {"hidden", &setmocktime},
        {"hidden", &mockscheduler},
        {"hidden", &echo},
        {"hidden", &echojson},
        {"hidden", &echoipc},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
