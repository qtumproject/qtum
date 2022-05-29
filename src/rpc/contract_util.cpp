#include <rpc/contract_util.h>
#include <rpc/util.h>
#include <util/system.h>
#include <key_io.h>
#include <rpc/server.h>
#include <txdb.h>

UniValue executionResultToJSON(const dev::eth::ExecutionResult& exRes)
{
    UniValue result(UniValue::VOBJ);
    result.pushKV("gasUsed", CAmount(exRes.gasUsed));
    std::stringstream ss;
    ss << exRes.excepted;
    result.pushKV("excepted", ss.str());
    result.pushKV("newAddress", exRes.newAddress.hex());
    result.pushKV("output", HexStr(exRes.output));
    result.pushKV("codeDeposit", static_cast<int32_t>(exRes.codeDeposit));
    result.pushKV("gasRefunded", CAmount(exRes.gasRefunded));
    result.pushKV("depositSize", static_cast<int32_t>(exRes.depositSize));
    result.pushKV("gasForDeposit", CAmount(exRes.gasForDeposit));
    result.pushKV("exceptedMessage", exceptedMessage(exRes.excepted, exRes.output));
    return result;
}

UniValue transactionReceiptToJSON(const QtumTransactionReceipt& txRec)
{
    UniValue result(UniValue::VOBJ);
    result.pushKV("stateRoot", txRec.stateRoot().hex());
    result.pushKV("utxoRoot", txRec.utxoRoot().hex());
    result.pushKV("gasUsed", CAmount(txRec.cumulativeGasUsed()));
    result.pushKV("bloom", txRec.bloom().hex());
    UniValue createdContracts(UniValue::VARR);
    for (const auto& item : txRec.createdContracts()) {
        UniValue contractItem(UniValue::VOBJ);
        contractItem.pushKV("address", item.first.hex());
        contractItem.pushKV("code", HexStr(item.second));
        createdContracts.push_back(contractItem);
    }
    result.pushKV("createdContracts", createdContracts);
    UniValue destructedContracts(UniValue::VARR);
    for (const dev::Address& contract : txRec.destructedContracts()) {
        destructedContracts.push_back(contract.hex());
    }
    result.pushKV("destructedContracts", destructedContracts);
    UniValue logEntries(UniValue::VARR);
    dev::eth::LogEntries logs = txRec.log();
    for(dev::eth::LogEntry log : logs){
        UniValue logEntrie(UniValue::VOBJ);
        logEntrie.pushKV("address", log.address.hex());
        UniValue topics(UniValue::VARR);
        for(dev::h256 l : log.topics){
            topics.push_back(l.hex());
        }
        logEntrie.pushKV("topics", topics);
        logEntrie.pushKV("data", HexStr(log.data));
        logEntries.push_back(logEntrie);
    }
    result.pushKV("log", logEntries);
    return result;
}

UniValue CallToContract(const UniValue& params, ChainstateManager &chainman)
{
    LOCK(cs_main);

    CChain& active_chain = chainman.ActiveChain();
    std::string strAddr = params[0].get_str();
    std::string data = params[1].get_str();

    if(data.size() % 2 != 0 || !CheckHex(data))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid data (data not hex)");

    dev::Address senderAddress;
    if(!params[2].isNull()){
        CTxDestination qtumSenderAddress = DecodeDestination(params[2].get_str());
        if (IsValidDestination(qtumSenderAddress)) {
            PKHash keyid = std::get<PKHash>(qtumSenderAddress);
            senderAddress = dev::Address(HexStr(valtype(keyid.begin(),keyid.end())));
        }else{
            senderAddress = dev::Address(params[2].get_str());
        }

    }
    uint64_t gasLimit=0;
    if(!params[3].isNull()){
        gasLimit = params[3].get_int64();
    }

    CAmount nAmount = 0;
    if (!params[4].isNull()){
        nAmount = AmountFromValue(params[4]);
        if (nAmount < 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for send");
    }

    TemporaryState ts(globalState);
    int blockNum;
    if (params.size() >= 6) {
        if (params[5].isNum()) {
            blockNum = params[5].get_int();
            if ((blockNum < 0 && blockNum != -1) || blockNum > active_chain.Height())
                throw JSONRPCError(RPC_INVALID_PARAMS, "Incorrect block number");
            if (blockNum != -1) {
                ts.SetRoot(uintToh256(active_chain[blockNum]->hashStateRoot), uintToh256(active_chain[blockNum]->hashUTXORoot));
            }
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMS, "Incorrect block number");
        }
    } else {
        blockNum = latestblock.height;
    }

    dev::Address addrAccount;
    if (strAddr.size() > 0) {
        if (strAddr.size() != 40 || !CheckHex(strAddr))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Incorrect address");
        addrAccount = dev::Address(strAddr);
        if (!globalState->addressInUse(addrAccount))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Address does not exist");
    }

    std::vector<ResultExecute> execResults = CallContract(addrAccount, ParseHex(data), chainman.ActiveChainstate(), blockNum, senderAddress, gasLimit, nAmount);

    if(fRecordLogOpcodes){
        writeVMlog(execResults, chainman.ActiveChain());
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("address", strAddr);
    result.pushKV("executionResult", executionResultToJSON(execResults[0].execRes));
    result.pushKV("transactionReceipt", transactionReceiptToJSON(execResults[0].txRec));

    return result;
}

void assignJSON(UniValue& entry, const TransactionReceiptInfo& resExec) {
    entry.pushKV("blockHash", resExec.blockHash.GetHex());
    entry.pushKV("blockNumber", uint64_t(resExec.blockNumber));
    entry.pushKV("transactionHash", resExec.transactionHash.GetHex());
    entry.pushKV("transactionIndex", uint64_t(resExec.transactionIndex));
    entry.pushKV("outputIndex", uint64_t(resExec.outputIndex));
    entry.pushKV("from", resExec.from.hex());
    entry.pushKV("to", resExec.to.hex());
    entry.pushKV("cumulativeGasUsed", CAmount(resExec.cumulativeGasUsed));
    entry.pushKV("gasUsed", CAmount(resExec.gasUsed));
    entry.pushKV("contractAddress", resExec.contractAddress.hex());
    std::stringstream ss;
    ss << resExec.excepted;
    entry.pushKV("excepted",ss.str());
    entry.pushKV("exceptedMessage", resExec.exceptedMessage);
    entry.pushKV("bloom", resExec.bloom.hex());
    entry.pushKV("stateRoot", resExec.stateRoot.hex());
    entry.pushKV("utxoRoot", resExec.utxoRoot.hex());
    UniValue createdContracts(UniValue::VARR);
    for (const auto& item : resExec.createdContracts) {
        UniValue contractItem(UniValue::VOBJ);
        contractItem.pushKV("address", item.first.hex());
        contractItem.pushKV("code", HexStr(item.second));
        createdContracts.push_back(contractItem);
    }
    entry.pushKV("createdContracts", createdContracts);
    UniValue destructedContracts(UniValue::VARR);
    for (const dev::Address& contract : resExec.destructedContracts) {
        destructedContracts.push_back(contract.hex());
    }
    entry.pushKV("destructedContracts", destructedContracts);
}

void assignJSON(UniValue& logEntry, const dev::eth::LogEntry& log,
        bool includeAddress) {
    if (includeAddress) {
        logEntry.pushKV("address", log.address.hex());
    }

    UniValue topics(UniValue::VARR);
    for (dev::h256 hash : log.topics) {
        topics.push_back(hash.hex());
    }
    logEntry.pushKV("topics", topics);
    logEntry.pushKV("data", HexStr(log.data));
}

void transactionReceiptInfoToJSON(const TransactionReceiptInfo& resExec, UniValue& entry) {
    assignJSON(entry, resExec);

    const auto& logs = resExec.logs;
    UniValue logEntries(UniValue::VARR);
    for(const auto&log : logs){
        UniValue logEntry(UniValue::VOBJ);
        assignJSON(logEntry, log, true);
        logEntries.push_back(logEntry);
    }
    entry.pushKV("log", logEntries);
}

size_t parseUInt(const UniValue& val, size_t defaultVal) {
    if (val.isNull()) {
        return defaultVal;
    } else {
        int n = val.get_int();
        if (n < 0) {
            throw JSONRPCError(RPC_INVALID_PARAMS, "Expects unsigned integer");
        }

        return n;
    }
}

int parseBlockHeight(const UniValue& val) {
    if (val.isStr()) {
        auto blockKey = val.get_str();

        if (blockKey == "latest") {
            return latestblock.height;
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMS, "invalid block number");
        }
    }

    if (val.isNum()) {
        int blockHeight = val.get_int();

        if (blockHeight < 0) {
            return latestblock.height;
        }

        return blockHeight;
    }

    throw JSONRPCError(RPC_INVALID_PARAMS, "invalid block number");
}

int parseBlockHeight(const UniValue& val, int defaultVal) {
    if (val.isNull()) {
        return defaultVal;
    } else {
        return parseBlockHeight(val);
    }
}

dev::h160 parseParamH160(const UniValue& val) {
    if (!val.isStr()) {
        throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid hex 160");
    }

    auto addrStr = val.get_str();

    if (addrStr.length() != 40 || !CheckHex(addrStr)) {
        throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid hex 160 string");
    }
    return dev::h160(addrStr);
}

void parseParam(const UniValue& val, std::vector<dev::h160> &h160s) {
    if (val.isNull()) {
        return;
    }

    // Treat a string as an array of length 1
    if (val.isStr()) {
        h160s.push_back(parseParamH160(val.get_str()));
        return;
    }

    if (!val.isArray()) {
        throw JSONRPCError(RPC_INVALID_PARAMS, "Expect an array of hex 160 strings");
    }

    auto vals = val.getValues();
    h160s.resize(vals.size());

    std::transform(vals.begin(), vals.end(), h160s.begin(), [](UniValue val) -> dev::h160 {
        return parseParamH160(val);
    });
}

void parseParam(const UniValue& val, std::set<dev::h160> &h160s) {
    std::vector<dev::h160> v;
    parseParam(val, v);
    h160s.insert(v.begin(), v.end());
}

void parseParam(const UniValue& val, std::vector<boost::optional<dev::h256>> &h256s) {
    if (val.isNull()) {
        return;
    }

    if (!val.isArray()) {
        throw JSONRPCError(RPC_INVALID_PARAMS, "Expect an array of hex 256 strings");
    }

    auto vals = val.getValues();
    h256s.resize(vals.size());

    std::transform(vals.begin(), vals.end(), h256s.begin(), [](UniValue val) -> boost::optional<dev::h256> {
        if (val.isNull()) {
            return boost::optional<dev::h256>();
        }

        if (!val.isStr()) {
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid hex 256 string");
        }

        auto addrStr = val.get_str();

        if (addrStr.length() != 64 || !CheckHex(addrStr)) {
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid hex 256 string");
        }

        return boost::optional<dev::h256>(dev::h256(addrStr));
    });
}

class SearchLogsParams {
public:
    size_t fromBlock;
    size_t toBlock;
    size_t minconf;

    std::set<dev::h160> addresses;
    std::vector<boost::optional<dev::h256>> topics;

    SearchLogsParams(const UniValue& params) {
        std::unique_lock<std::mutex> lock(cs_blockchange);

        setFromBlock(params[0]);
        setToBlock(params[1]);

        parseParam(params[2]["addresses"], addresses);
        parseParam(params[3]["topics"], topics);

        minconf = parseUInt(params[4], 0);
    }

private:
    void setFromBlock(const UniValue& val) {
        if (!val.isNull()) {
            fromBlock = parseBlockHeight(val);
        } else {
            fromBlock = latestblock.height;
        }
    }

    void setToBlock(const UniValue& val) {
        if (!val.isNull()) {
            toBlock = parseBlockHeight(val);
        } else {
            toBlock = latestblock.height;
        }
    }

};

UniValue SearchLogs(const UniValue& _params, ChainstateManager &chainman)
{
    if(!fLogEvents)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Events indexing disabled");

    int curheight = 0;

    LOCK(cs_main);

    SearchLogsParams params(_params);

    std::vector<std::vector<uint256>> hashesToBlock;

    curheight = pblocktree->ReadHeightIndex(params.fromBlock, params.toBlock, params.minconf, hashesToBlock, params.addresses, chainman);

    if (curheight == -1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Incorrect params");
    }

    UniValue result(UniValue::VARR);

    auto topics = params.topics;

    std::set<uint256> dupes;

    for(const auto& hashesTx : hashesToBlock)
    {
        for(const auto& e : hashesTx)
        {

            if(dupes.find(e) != dupes.end()) {
                continue;
            }
            dupes.insert(e);

            std::vector<TransactionReceiptInfo> receipts = pstorageresult->getResult(uintToh256(e));

            for(const auto& receipt : receipts) {
                if(receipt.logs.empty()) {
                    continue;
                }

                if (!topics.empty()) {
                    for (size_t i = 0; i < topics.size(); i++) {
                        const auto& tc = topics[i];

                        if (!tc) {
                            continue;
                        }

                        for (const auto& log: receipt.logs) {
                            auto filterTopicContent = tc.get();

                            if (i >= log.topics.size()) {
                                continue;
                            }

                            if (filterTopicContent == log.topics[i]) {
                                goto push;
                            }
                        }
                    }

                    // Skip the log if none of the topics are matched
                    continue;
                }

            push:

                UniValue tri(UniValue::VOBJ);
                transactionReceiptInfoToJSON(receipt, tri);
                result.push_back(tri);
            }
        }
    }

    return result;
}

CallToken::CallToken(ChainstateManager &_chainman):
    chainman(_chainman)
{
    setQtumTokenExec(this);
}

bool CallToken::execValid(const int &func, const bool &sendTo)
{
    if(func == -1 || sendTo)
        return false;
    return true;
}

bool CallToken::execEventsValid(const int &func, const int64_t &fromBlock)
{
    if(func == -1 || fromBlock < 0)
        return false;
    return true;
}

bool CallToken::exec(const bool &sendTo, const std::map<std::string, std::string> &lstParams, std::string &result, std::string &)
{
    if(sendTo)
        return false;

    UniValue params(UniValue::VARR);

    // Set address
    auto it = lstParams.find(paramAddress());
    if(it != lstParams.end())
        params.push_back(it->second);
    else
        return false;

    // Set data
    it = lstParams.find(paramDatahex());
    if(it != lstParams.end())
        params.push_back(it->second);
    else
        return false;

    // Set sender
    it = lstParams.find(paramSender());
    if(it != lstParams.end())
    {
        if(params.size() == 2)
            params.push_back(it->second);
        else
            return false;
    }

    // Set gas limit
    if(checkGasForCall)
    {
        it = lstParams.find(paramGasLimit());
        if(it != lstParams.end())
        {
            if(params.size() == 3)
            {
                UniValue param(UniValue::VNUM);
                param.setInt(atoi64(it->second));
                params.push_back(param);
            }
            else
                return false;
        }
    }

    // Get execution result
    UniValue response = CallToContract(params, chainman);
    if(!response.isObject() || !response.exists("executionResult"))
        return false;
    UniValue executionResult = response["executionResult"];

    // Get output
    if(!executionResult.isObject() || !executionResult.exists("output"))
        return false;
    UniValue output = executionResult["output"];
    result = output.get_str();

    return true;
}

bool CallToken::execEvents(const int64_t &fromBlock, const int64_t &toBlock, const int64_t& minconf, const std::string &eventName, const std::string &contractAddress, const std::string &senderAddress, const int &numTopics, std::vector<TokenEvent> &result)
{
    UniValue resultVar;
    if(!searchTokenTx(fromBlock, toBlock, minconf, eventName, contractAddress, senderAddress, numTopics, resultVar))
        return false;

    const UniValue& list = resultVar.get_array();
    for(size_t i = 0; i < list.size(); i++)
    {
        // Search the log for events
        const UniValue& eventMap = list[i].get_obj();
        const UniValue& listLog = eventMap["log"].get_array();
        for(size_t i = 0; i < listLog.size(); i++)
        {
            // Skip the not needed events
            const UniValue& eventLog = listLog[i].get_obj();
            const UniValue& topicsList = eventLog["topics"].get_array();
            if(topicsList.size() < (size_t)numTopics) continue;
            if(topicsList[0].get_str() != eventName) continue;

            // Create new event
            TokenEvent tokenEvent;
            tokenEvent.address = eventMap["contractAddress"].get_str();
            if(numTopics > 1)
            {
                tokenEvent.sender = topicsList[1].get_str().substr(24);
                ToQtumAddress(tokenEvent.sender, tokenEvent.sender);
            }
            if(numTopics > 2)
            {
                tokenEvent.receiver = topicsList[2].get_str().substr(24);
                ToQtumAddress(tokenEvent.receiver, tokenEvent.receiver);
            }
            tokenEvent.blockHash = uint256S(eventMap["blockHash"].get_str());
            tokenEvent.blockNumber = eventMap["blockNumber"].get_int64();
            tokenEvent.transactionHash = uint256S(eventMap["transactionHash"].get_str());

            // Parse data
            std::string data = eventLog["data"].get_str();
            tokenEvent.value = ToUint256(data);

            result.push_back(tokenEvent);
        }
    }

    return true;
}

bool CallToken::searchTokenTx(const int64_t &fromBlock, const int64_t &toBlock, const int64_t &minconf, const std::string &eventName, const std::string &contractAddress, const std::string &senderAddress, const int &numTopics, UniValue &resultVar)
{
    UniValue params(UniValue::VARR);
    params.push_back(fromBlock);
    params.push_back(toBlock);

    UniValue addresses(UniValue::VARR);
    addresses.push_back(contractAddress);
    UniValue addressesObj(UniValue::VOBJ);
    addressesObj.pushKV("addresses", addresses);
    params.push_back(addressesObj);

    UniValue topics(UniValue::VARR);
    // Skip the event type check
    static std::string nullRecord = uint256().ToString();
    topics.push_back(nullRecord);
    if(numTopics > 1)
    {
        // Match the log with sender address
        topics.push_back(senderAddress);
    }
    if(numTopics > 2)
    {
        // Match the log with receiver address
        topics.push_back(senderAddress);
    }
    UniValue topicsObj(UniValue::VOBJ);
    topicsObj.pushKV("topics", topics);
    params.push_back(topicsObj);

    params.push_back(minconf);

    resultVar = SearchLogs(params, chainman);

    return true;
}

void CallToken::setCheckGasForCall(bool value)
{
    checkGasForCall = value;
}
