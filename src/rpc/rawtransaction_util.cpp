// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/rawtransaction_util.h>

#include <coins.h>
#include <core_io.h>
#include <key_io.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <rpc/request.h>
#include <rpc/util.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <tinyformat.h>
#include <univalue.h>
#include <util/rbf.h>
#include <util/strencodings.h>
#include <validation.h>
#include <util/moneystr.h>

CMutableTransaction ConstructTransaction(const UniValue& inputs_in, const UniValue& outputs_in, const UniValue& locktime, bool rbf)
{
    if (inputs_in.isNull() || outputs_in.isNull())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, arguments 1 and 2 must be non-null");

    UniValue inputs = inputs_in.get_array();
    const bool outputs_is_obj = outputs_in.isObject();
    UniValue outputs = outputs_is_obj ? outputs_in.get_obj() : outputs_in.get_array();

    CMutableTransaction rawTx;

    if (!locktime.isNull()) {
        int64_t nLockTime = locktime.get_int64();
        if (nLockTime < 0 || nLockTime > LOCKTIME_MAX)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, locktime out of range");
        rawTx.nLockTime = nLockTime;
    }

    for (unsigned int idx = 0; idx < inputs.size(); idx++) {
        const UniValue& input = inputs[idx];
        const UniValue& o = input.get_obj();

        uint256 txid = ParseHashO(o, "txid");

        const UniValue& vout_v = find_value(o, "vout");
        if (!vout_v.isNum())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, missing vout key");
        int nOutput = vout_v.get_int();
        if (nOutput < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, vout must be positive");

        uint32_t nSequence;
        if (rbf) {
            nSequence = MAX_BIP125_RBF_SEQUENCE; /* CTxIn::SEQUENCE_FINAL - 2 */
        } else if (rawTx.nLockTime) {
            nSequence = CTxIn::SEQUENCE_FINAL - 1;
        } else {
            nSequence = CTxIn::SEQUENCE_FINAL;
        }

        // set the sequence number if passed in the parameters object
        const UniValue& sequenceObj = find_value(o, "sequence");
        if (sequenceObj.isNum()) {
            int64_t seqNr64 = sequenceObj.get_int64();
            if (seqNr64 < 0 || seqNr64 > CTxIn::SEQUENCE_FINAL) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, sequence number is out of range");
            } else {
                nSequence = (uint32_t)seqNr64;
            }
        }

        CTxIn in(COutPoint(txid, nOutput), CScript(), nSequence);

        rawTx.vin.push_back(in);
    }

    if (!outputs_is_obj) {
        // Translate array of key-value pairs into dict
        UniValue outputs_dict = UniValue(UniValue::VOBJ);
        for (size_t i = 0; i < outputs.size(); ++i) {
            const UniValue& output = outputs[i];
            if (!output.isObject()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, key-value pair not an object as expected");
            }
            if (output.size() != 1) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, key-value pair must contain exactly one key");
            }
            outputs_dict.pushKVs(output);
        }
        outputs = std::move(outputs_dict);
    }

    // Duplicate checking
    std::set<CTxDestination> destinations;
    bool has_data{false};

    int i = 0;
    for (const std::string& name_ : outputs.getKeys()) {
        if (name_ == "data") {
            if (has_data) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, duplicate key: data");
            }
            has_data = true;
            std::vector<unsigned char> data = ParseHexV(outputs[name_].getValStr(), "Data");

            CTxOut out(0, CScript() << OP_RETURN << data);
            rawTx.vout.push_back(out);
       } else if (name_ == "contract") {
            // Get the contract object
            UniValue Contract = outputs[i];
            if(!Contract.isObject())
                throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, need to be object: ")+name_);

            // Get dgp gas limit and gas price
            LOCK(cs_main);
            QtumDGP qtumDGP(globalState.get(), fGettingValuesDGP);
            uint64_t blockGasLimit = qtumDGP.getBlockGasLimit(::ChainActive().Height());
            uint64_t minGasPrice = CAmount(qtumDGP.getMinGasPrice(::ChainActive().Height()));
            CAmount nGasPrice = (minGasPrice>DEFAULT_GAS_PRICE)?minGasPrice:DEFAULT_GAS_PRICE;

            bool createContract = Contract.exists("bytecode") && Contract["bytecode"].isStr();
            CScript scriptPubKey;
            CAmount nAmount = 0;

            // Get gas limit
            uint64_t nGasLimit=createContract ? DEFAULT_GAS_LIMIT_OP_CREATE : DEFAULT_GAS_LIMIT_OP_SEND;
            if (Contract.exists("gasLimit")){
                nGasLimit = Contract["gasLimit"].get_int64();
                if (nGasLimit > blockGasLimit)
                    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit (Maximum is: "+i64tostr(blockGasLimit)+")");
                if (nGasLimit < MINIMUM_GAS_LIMIT)
                    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit (Minimum is: "+i64tostr(MINIMUM_GAS_LIMIT)+")");
                if (nGasLimit <= 0)
                    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit");
            }

            // Get gas price
            if (Contract.exists("gasPrice")){
                UniValue uGasPrice = Contract["gasPrice"];
                if(!ParseMoney(uGasPrice.getValStr(), nGasPrice))
                {
                    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice");
                }
                CAmount maxRpcGasPrice = gArgs.GetArg("-rpcmaxgasprice", MAX_RPC_GAS_PRICE);
                if (nGasPrice > (int64_t)maxRpcGasPrice)
                    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice, Maximum allowed in RPC calls is: "+FormatMoney(maxRpcGasPrice)+" (use -rpcmaxgasprice to change it)");
                if (nGasPrice < (int64_t)minGasPrice)
                    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice (Minimum is: "+FormatMoney(minGasPrice)+")");
                if (nGasPrice <= 0)
                    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice");
            }

            // Get sender address
            bool fHasSender=false;
            CTxDestination senderAddress;
            if (Contract.exists("senderAddress")){
                senderAddress = DecodeDestination(Contract["senderAddress"].get_str());
                if (!IsValidDestination(senderAddress))
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Qtum address to send from");
                if (!IsValidContractSenderAddress(senderAddress))
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid contract sender address. Only P2PK and P2PKH allowed");
                else
                    fHasSender=true;
            }

            if(createContract)
            {
                // Get the new contract bytecode
                if(!Contract.exists("bytecode") || !Contract["bytecode"].isStr())
                    throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, bytecode is mandatory."));

                std::string bytecodehex = Contract["bytecode"].get_str();
                if(bytecodehex.size() % 2 != 0 || !CheckHex(bytecodehex))
                    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid bytecode (bytecode not hex)");

                // Add create contract output
                scriptPubKey = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(nGasLimit) << CScriptNum(nGasPrice) << ParseHex(bytecodehex) <<OP_CREATE;
            }
            else
            {
                // Get the contract address
                if(!Contract.exists("contractAddress") || !Contract["contractAddress"].isStr())
                    throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, contract address is mandatory."));

                std::string contractaddress = Contract["contractAddress"].get_str();
                if(contractaddress.size() != 40 || !CheckHex(contractaddress))
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Incorrect contract address");

                dev::Address addrAccount(contractaddress);
                if(!globalState->addressInUse(addrAccount))
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "contract address does not exist");

                // Get the contract data
                if(!Contract.exists("data") || !Contract["data"].isStr())
                    throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, contract data is mandatory."));

                std::string datahex = Contract["data"].get_str();
                if(datahex.size() % 2 != 0 || !CheckHex(datahex))
                    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid data (data not hex)");

                // Get amount
                if (Contract.exists("amount")){
                    nAmount = AmountFromValue(Contract["amount"]);
                    if (nAmount < 0)
                        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for call contract");
                }

                // Add call contract output
                scriptPubKey = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(nGasLimit) << CScriptNum(nGasPrice) << ParseHex(datahex) << ParseHex(contractaddress) << OP_CALL;
            }

             // Build op_sender script
            if(fHasSender && ::ChainActive().Height() >= Params().GetConsensus().QIP5Height)
            {
                const PKHash *keyID = boost::get<PKHash>(&senderAddress);
                if(!keyID)
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Only pubkeyhash addresses are supported");
                std::vector<unsigned char> scriptSig;
                scriptPubKey = (CScript() << CScriptNum(addresstype::PUBKEYHASH) << ToByteVector(*keyID) << ToByteVector(scriptSig) << OP_SENDER) + scriptPubKey;
            }

            CTxOut out(nAmount, scriptPubKey);
            rawTx.vout.push_back(out);
        } else {
            CTxDestination destination = DecodeDestination(name_);
            if (!IsValidDestination(destination)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Qtum address: ") + name_);
            }

            if (!destinations.insert(destination).second) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, duplicated address: ") + name_);
            }

            CScript scriptPubKey = GetScriptForDestination(destination);
            CAmount nAmount = AmountFromValue(outputs[name_]);

            CTxOut out(nAmount, scriptPubKey);
            rawTx.vout.push_back(out);
        }
        ++i;
    }

    if (rbf && rawTx.vin.size() > 0 && !SignalsOptInRBF(CTransaction(rawTx))) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter combination: Sequence number(s) contradict replaceable option");
    }

    return rawTx;
}

/** Pushes a JSON object for script verification or signing errors to vErrorsRet. */
static void TxInErrorToJSON(const CTxIn& txin, UniValue& vErrorsRet, const std::string& strMessage)
{
    UniValue entry(UniValue::VOBJ);
    entry.pushKV("txid", txin.prevout.hash.ToString());
    entry.pushKV("vout", (uint64_t)txin.prevout.n);
    UniValue witness(UniValue::VARR);
    for (unsigned int i = 0; i < txin.scriptWitness.stack.size(); i++) {
        witness.push_back(HexStr(txin.scriptWitness.stack[i].begin(), txin.scriptWitness.stack[i].end()));
    }
    entry.pushKV("witness", witness);
    entry.pushKV("scriptSig", HexStr(txin.scriptSig.begin(), txin.scriptSig.end()));
    entry.pushKV("sequence", (uint64_t)txin.nSequence);
    entry.pushKV("error", strMessage);
    vErrorsRet.push_back(entry);
}

void ParsePrevouts(const UniValue& prevTxsUnival, FillableSigningProvider* keystore, std::map<COutPoint, Coin>& coins)
{
    if (!prevTxsUnival.isNull()) {
        UniValue prevTxs = prevTxsUnival.get_array();
        for (unsigned int idx = 0; idx < prevTxs.size(); ++idx) {
            const UniValue& p = prevTxs[idx];
            if (!p.isObject()) {
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected object with {\"txid'\",\"vout\",\"scriptPubKey\"}");
            }

            UniValue prevOut = p.get_obj();

            RPCTypeCheckObj(prevOut,
                {
                    {"txid", UniValueType(UniValue::VSTR)},
                    {"vout", UniValueType(UniValue::VNUM)},
                    {"scriptPubKey", UniValueType(UniValue::VSTR)},
                });

            uint256 txid = ParseHashO(prevOut, "txid");

            int nOut = find_value(prevOut, "vout").get_int();
            if (nOut < 0) {
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "vout must be positive");
            }

            COutPoint out(txid, nOut);
            std::vector<unsigned char> pkData(ParseHexO(prevOut, "scriptPubKey"));
            CScript scriptPubKey(pkData.begin(), pkData.end());

            {
                auto coin = coins.find(out);
                if (coin != coins.end() && !coin->second.IsSpent() && coin->second.out.scriptPubKey != scriptPubKey) {
                    std::string err("Previous output scriptPubKey mismatch:\n");
                    err = err + ScriptToAsmStr(coin->second.out.scriptPubKey) + "\nvs:\n"+
                        ScriptToAsmStr(scriptPubKey);
                    throw JSONRPCError(RPC_DESERIALIZATION_ERROR, err);
                }
                Coin newcoin;
                newcoin.out.scriptPubKey = scriptPubKey;
                newcoin.out.nValue = MAX_MONEY;
                if (prevOut.exists("amount")) {
                    newcoin.out.nValue = AmountFromValue(find_value(prevOut, "amount"));
                }
                newcoin.nHeight = 1;
                coins[out] = std::move(newcoin);
            }

            // if redeemScript and private keys were given, add redeemScript to the keystore so it can be signed
            const bool is_p2sh = scriptPubKey.IsPayToScriptHash();
            const bool is_p2wsh = scriptPubKey.IsPayToWitnessScriptHash();
            if (keystore && (is_p2sh || is_p2wsh)) {
                RPCTypeCheckObj(prevOut,
                    {
                        {"redeemScript", UniValueType(UniValue::VSTR)},
                        {"witnessScript", UniValueType(UniValue::VSTR)},
                    }, true);
                UniValue rs = find_value(prevOut, "redeemScript");
                UniValue ws = find_value(prevOut, "witnessScript");
                if (rs.isNull() && ws.isNull()) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Missing redeemScript/witnessScript");
                }

                // work from witnessScript when possible
                std::vector<unsigned char> scriptData(!ws.isNull() ? ParseHexV(ws, "witnessScript") : ParseHexV(rs, "redeemScript"));
                CScript script(scriptData.begin(), scriptData.end());
                keystore->AddCScript(script);
                // Automatically also add the P2WSH wrapped version of the script (to deal with P2SH-P2WSH).
                // This is done for redeemScript only for compatibility, it is encouraged to use the explicit witnessScript field instead.
                CScript witness_output_script{GetScriptForDestination(WitnessV0ScriptHash(script))};
                keystore->AddCScript(witness_output_script);

                if (!ws.isNull() && !rs.isNull()) {
                    // if both witnessScript and redeemScript are provided,
                    // they should either be the same (for backwards compat),
                    // or the redeemScript should be the encoded form of
                    // the witnessScript (ie, for p2sh-p2wsh)
                    if (ws.get_str() != rs.get_str()) {
                        std::vector<unsigned char> redeemScriptData(ParseHexV(rs, "redeemScript"));
                        CScript redeemScript(redeemScriptData.begin(), redeemScriptData.end());
                        if (redeemScript != witness_output_script) {
                            throw JSONRPCError(RPC_INVALID_PARAMETER, "redeemScript does not correspond to witnessScript");
                        }
                    }
                }

                if (is_p2sh) {
                    const CTxDestination p2sh{ScriptHash(script)};
                    const CTxDestination p2sh_p2wsh{ScriptHash(witness_output_script)};
                    if (scriptPubKey == GetScriptForDestination(p2sh)) {
                        // traditional p2sh; arguably an error if
                        // we got here with rs.IsNull(), because
                        // that means the p2sh script was specified
                        // via witnessScript param, but for now
                        // we'll just quietly accept it
                    } else if (scriptPubKey == GetScriptForDestination(p2sh_p2wsh)) {
                        // p2wsh encoded as p2sh; ideally the witness
                        // script was specified in the witnessScript
                        // param, but also support specifying it via
                        // redeemScript param for backwards compat
                        // (in which case ws.IsNull() == true)
                    } else {
                        // otherwise, can't generate scriptPubKey from
                        // either script, so we got unusable parameters
                        throw JSONRPCError(RPC_INVALID_PARAMETER, "redeemScript/witnessScript does not match scriptPubKey");
                    }
                } else if (is_p2wsh) {
                    // plain p2wsh; could throw an error if script
                    // was specified by redeemScript rather than
                    // witnessScript (ie, ws.IsNull() == true), but
                    // accept it for backwards compat
                    const CTxDestination p2wsh{WitnessV0ScriptHash(script)};
                    if (scriptPubKey != GetScriptForDestination(p2wsh)) {
                        throw JSONRPCError(RPC_INVALID_PARAMETER, "redeemScript/witnessScript does not match scriptPubKey");
                    }
                }
            }
        }
    }
}

void CheckSenderSignatures(CMutableTransaction& mtx)
{
    // Check the sender signatures are inside the outputs, before signing the inputs
    if(mtx.HasOpSender())
    {
        int nOut = 0;
        for (const auto& output : mtx.vout)
        {
            if(output.scriptPubKey.HasOpSender())
            {
                CScript senderPubKey, senderSig;
                if(!ExtractSenderData(output.scriptPubKey, &senderPubKey, &senderSig))
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Missing contract sender signature,"
                                                              "use signrawsendertransactionwithwallet or signrawsendertransactionwithkey to sign the outputs");
            }
            nOut++;
        }
    }
}

void SignTransaction(CMutableTransaction& mtx, const SigningProvider* keystore, const std::map<COutPoint, Coin>& coins, const UniValue& hashType, UniValue& result)
{
    int nHashType = ParseSighashString(hashType);

    // Script verification errors
    std::map<int, std::string> input_errors;

    bool complete = SignTransaction(mtx, keystore, coins, nHashType, input_errors);
    SignTransactionResultToJSON(mtx, complete, coins, input_errors, result);
}

void SignTransactionResultToJSON(CMutableTransaction& mtx, bool complete, const std::map<COutPoint, Coin>& coins, std::map<int, std::string>& input_errors, UniValue& result)
{
    // Make errors UniValue
    UniValue vErrors(UniValue::VARR);
    for (const auto& err_pair : input_errors) {
        if (err_pair.second == "Missing amount") {
            // This particular error needs to be an exception for some reason
            throw JSONRPCError(RPC_TYPE_ERROR, strprintf("Missing amount for %s", coins.at(mtx.vin.at(err_pair.first).prevout).out.ToString()));
        }
        TxInErrorToJSON(mtx.vin.at(err_pair.first), vErrors, err_pair.second);
    }

    result.pushKV("hex", EncodeHexTx(CTransaction(mtx)));
    result.pushKV("complete", complete);
    if (!vErrors.empty()) {
        if (result.exists("errors")) {
            vErrors.push_backV(result["errors"].getValues());
        }
        result.pushKV("errors", vErrors);
    }
}

static void TxOutErrorToJSON(const CTxOut& output, UniValue& vErrorsRet, const std::string& strMessage)
{
    UniValue entry(UniValue::VOBJ);
    entry.pushKV("amount", ValueFromAmount(output.nValue));
    entry.pushKV("scriptPubKey", HexStr(output.scriptPubKey.begin(), output.scriptPubKey.end()));
    entry.pushKV("error", strMessage);
    vErrorsRet.push_back(entry);
}

UniValue SignTransactionSender(CMutableTransaction& mtx, FillableSigningProvider *keystore, const UniValue& hashType)
{
    int nHashType = ParseSighashString(hashType);

    // Script verification errors
    UniValue vErrors(UniValue::VARR);

    // Signing transaction outputs
    int nOut = 0;
    for (const auto& output : mtx.vout)
    {
        if(output.scriptPubKey.HasOpSender())
        {
            CScript scriptPubKey;
            if(!GetSenderPubKey(output.scriptPubKey, scriptPubKey))
            {
                TxOutErrorToJSON(output, vErrors, "Fail to get sender public key");
                continue;
            }

            SignatureData sigdata;

            if (!ProduceSignature(*keystore, MutableTransactionSignatureOutputCreator(&mtx, nOut, output.nValue, nHashType), scriptPubKey, sigdata))
            {
                TxOutErrorToJSON(output, vErrors, "Signing transaction output failed");
                continue;
            }
            else
            {
                if(!UpdateOutput(mtx.vout.at(nOut), sigdata))
                {
                    TxOutErrorToJSON(output, vErrors, "Update transaction output failed");
                    continue;
                }
            }
        }
        nOut++;
    }

    bool fComplete = vErrors.empty();

    UniValue result(UniValue::VOBJ);
    result.pushKV("hex", EncodeHexTx(CTransaction(mtx)));
    result.pushKV("complete", fComplete);
    if (!vErrors.empty()) {
        result.pushKV("errors", vErrors);
    }

    return result;
}
