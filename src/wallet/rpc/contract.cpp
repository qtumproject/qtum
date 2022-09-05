#include <key_io.h>
#include <core_io.h>
#include <rpc/util.h>
#include <rpc/server.h>
#include <rpc/contract_util.h>
#include <util/moneystr.h>
#include <util/system.h>
#include <util/signstr.h>
#include <util/tokenstr.h>
#include <wallet/rpc/util.h>
#include <wallet/wallet.h>
#include <wallet/coincontrol.h>
#include <wallet/spend.h>
#include <wallet/receive.h>
#include <qtum/qtumdelegation.h>
#include <validation.h>

#include <univalue.h>

namespace wallet {

bool SetDefaultPayForContractAddress(const CWallet& wallet, CCoinControl & coinControl)
{
    // Set default coin to pay for the contract
    // Select any valid unspent output that can be used to pay for the contract
    std::vector<COutput> vecOutputs;
    coinControl.fAllowOtherInputs=true;
    coinControl.m_include_unsafe_inputs = true;

    AvailableCoins(wallet, vecOutputs, &coinControl);

    for (const COutput& out : vecOutputs) {
        CTxDestination destAdress;
        const CScript& scriptPubKey = out.tx->tx->vout[out.i].scriptPubKey;
        bool fValidAddress = out.fSpendable && ExtractDestination(scriptPubKey, destAdress)
                && IsValidContractSenderAddress(destAdress);

        if (!fValidAddress)
            continue;

        coinControl.Select(COutPoint(out.tx->GetHash(),out.i));
        break;
    }

    return coinControl.HasSelected();
}

bool SetDefaultSignSenderAddress(const CWallet& wallet, CTxDestination& destAdress, CCoinControl & coinControl)
{
    // Set default sender address if none provided
    // Select any valid unspent output that can be used for contract sender address
    std::vector<COutput> vecOutputs;
    coinControl.fAllowOtherInputs=true;
    coinControl.m_include_unsafe_inputs = true;

    AvailableCoins(wallet, vecOutputs, &coinControl);

    for (const COutput& out : vecOutputs) {
        const CScript& scriptPubKey = out.tx->tx->vout[out.i].scriptPubKey;
        bool fValidAddress = out.fSpendable && ExtractDestination(scriptPubKey, destAdress)
                && IsValidContractSenderAddress(destAdress);

        if (!fValidAddress)
            continue;
        break;
    }

    return !std::holds_alternative<CNoDestination>(destAdress);
}

PSBTOutput GetPsbtOutput(const CTxOut& v, CWallet& wallet)
{
    PSBTOutput out;
    if(v.scriptPubKey.HasOpSender())
    {
        CScript senderPubKey;
        PKHash pkhash ;
        CPubKey vchPubKeyOut;
        KeyOriginInfo info;
        bool ok = GetSenderPubKey(v.scriptPubKey, senderPubKey);
        if(ok)
        {
            pkhash = ExtractPublicKeyHash(senderPubKey);
            ok &= pkhash != PKHash();
        }
        if(ok) ok &=  wallet.GetPubKey(pkhash, vchPubKeyOut);
        if(ok) ok &=  wallet.GetKeyOrigin(pkhash, info);
        if(ok) out.hd_keypaths[vchPubKeyOut] = info;
    }

    return out;
}

void getDgpData(uint64_t& blockGasLimit, uint64_t& minGasPrice, CAmount& nGasPrice, int* pHeight = nullptr, ChainstateManager* chainman = nullptr)
{
    static uint64_t BlockGasLimit = DEFAULT_BLOCK_GAS_LIMIT_DGP;
    static uint64_t MinGasPrice = DEFAULT_MIN_GAS_PRICE_DGP;
    blockGasLimit = BlockGasLimit;
    minGasPrice = MinGasPrice;
    if(globalState.get() && chainman)
    {
        LOCK(cs_main);
        QtumDGP qtumDGP(globalState.get(), chainman->ActiveChainstate(), fGettingValuesDGP);
        int height = chainman->ActiveChain().Height();
        blockGasLimit = qtumDGP.getBlockGasLimit(height);
        minGasPrice = CAmount(qtumDGP.getMinGasPrice(height));
        if(pHeight) *pHeight = height;
        BlockGasLimit = blockGasLimit;
        MinGasPrice = minGasPrice;
    }
    nGasPrice = (minGasPrice>DEFAULT_GAS_PRICE)?minGasPrice:DEFAULT_GAS_PRICE;
}

RPCHelpMan createcontract()
{
    uint64_t blockGasLimit = 0, minGasPrice = 0;
    CAmount nGasPrice = 0;
    getDgpData(blockGasLimit, minGasPrice, nGasPrice);

    return RPCHelpMan{"createcontract",
                "\nCreate a contract with bytcode." +
                HELP_REQUIRING_PASSPHRASE,
                {
                    {"bytecode", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "contract bytcode."},
                    {"gasLimit", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "gasLimit, default: "+i64tostr(DEFAULT_GAS_LIMIT_OP_CREATE)+", max: "+i64tostr(blockGasLimit)},
                    {"gasPrice", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "gasPrice QTUM price per gas unit, default: "+FormatMoney(nGasPrice)+", min:"+FormatMoney(minGasPrice)},
                    {"senderAddress", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "The qtum address that will be used to create the contract."},
                    {"broadcast", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Whether to broadcast the transaction or not, default: true."},
                    {"changeToSender", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Return the change to the sender, default: true."},
                    {"psbt", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Create partially signed transaction."},
                },
                {
                    RPCResult{"if broadcast is set to true",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR, "psbt", "The base64-encoded unsigned PSBT of the new transaction. Only returned when wallet private keys are disabled."},
                            {RPCResult::Type::STR_HEX, "txid", "The transaction id. Only returned when wallet private keys are enabled."},
                            {RPCResult::Type::STR, "sender", CURRENCY_UNIT + " address of the sender"},
                            {RPCResult::Type::STR_HEX, "hash160", "Ripemd-160 hash of the sender"},
                            {RPCResult::Type::STR, "address", "Expected contract address"}
                        },
                    },
                    RPCResult{"if broadcast is set to false",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR_HEX, "raw transaction", "The hex string of the raw transaction"}
                        },
                    },
                },
                RPCExamples{
                HelpExampleCli("createcontract", "\"60606040525b33600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff02191690836c010000000000000000000000009081020402179055506103786001600050819055505b600c80605b6000396000f360606040526008565b600256\"")
                + HelpExampleCli("createcontract", "\"60606040525b33600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff02191690836c010000000000000000000000009081020402179055506103786001600050819055505b600c80605b6000396000f360606040526008565b600256\" 6000000 "+FormatMoney(minGasPrice)+" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" true")
                },
        [&,blockGasLimit,minGasPrice,nGasPrice](const RPCHelpMan& self, const JSONRPCRequest& request) mutable -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    ChainstateManager& chainman = pwallet->chain().chainman();
    int height = 0;
    getDgpData(blockGasLimit, minGasPrice, nGasPrice, &height, &chainman);

    LOCK(pwallet->cs_wallet);

    std::string bytecode=request.params[0].get_str();

    if(bytecode.size() % 2 != 0 || !CheckHex(bytecode))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid data (data not hex)");

    uint64_t nGasLimit=DEFAULT_GAS_LIMIT_OP_CREATE;
    if (request.params.size() > 1){
        nGasLimit = request.params[1].get_int64();
        if (nGasLimit > blockGasLimit)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit (Maximum is: "+i64tostr(blockGasLimit)+")");
        if (nGasLimit < MINIMUM_GAS_LIMIT)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit (Minimum is: "+i64tostr(MINIMUM_GAS_LIMIT)+")");
        if (nGasLimit <= 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit");
    }

    if (request.params.size() > 2){
        nGasPrice = AmountFromValue(request.params[2]);
        if (nGasPrice <= 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice");
        CAmount maxRpcGasPrice = gArgs.GetIntArg("-rpcmaxgasprice", MAX_RPC_GAS_PRICE);
        if (nGasPrice > (int64_t)maxRpcGasPrice)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice, Maximum allowed in RPC calls is: "+FormatMoney(maxRpcGasPrice)+" (use -rpcmaxgasprice to change it)");
        if (nGasPrice < (int64_t)minGasPrice)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice (Minimum is: "+FormatMoney(minGasPrice)+")");
    }

    bool fHasSender=false;
    CTxDestination senderAddress;
    if (request.params.size() > 3){
        senderAddress = DecodeDestination(request.params[3].get_str());
        if (!IsValidDestination(senderAddress))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Qtum address to send from");
        if (!IsValidContractSenderAddress(senderAddress))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid contract sender address. Only P2PK and P2PKH allowed");
        else
            fHasSender=true;
    }

    bool fBroadcast=true;
    if (request.params.size() > 4){
        fBroadcast=request.params[4].get_bool();
    }

    bool fChangeToSender=true;
    if (request.params.size() > 5){
        fChangeToSender=request.params[5].get_bool();
    }

    bool fPsbt=pwallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    if (request.params.size() > 6){
        fPsbt=request.params[6].get_bool();
    }
    if(fPsbt) fBroadcast=false;

    CCoinControl coinControl;
    if(fPsbt) coinControl.fAllowWatchOnly = true;

    CTxDestination signSenderAddress = CNoDestination();
    if(fHasSender){
        // Find a UTXO with sender address
        std::vector<COutput> vecOutputs;

        coinControl.fAllowOtherInputs=true;

        assert(pwallet != NULL);
        AvailableCoins(*pwallet, vecOutputs, NULL);

        for (const COutput& out : vecOutputs) {
            CTxDestination destAdress;
            const CScript& scriptPubKey = out.tx->tx->vout[out.i].scriptPubKey;
            bool fValidAddress = out.fSpendable && ExtractDestination(scriptPubKey, destAdress);

            if (!fValidAddress || senderAddress != destAdress)
                continue;

            coinControl.Select(COutPoint(out.tx->GetHash(),out.i));

            break;

        }

        if(coinControl.HasSelected())
        {
            // Change to the sender
            if(fChangeToSender){
                coinControl.destChange=senderAddress;
            }
        }
        else
        {
            // Create op sender transaction when op sender is activated
            if(!(height >= Params().GetConsensus().QIP5Height))
                throw JSONRPCError(RPC_TYPE_ERROR, "Sender address does not have any unspent outputs");
        }

        if(height >= Params().GetConsensus().QIP5Height)
        {
            // Set the sender address
            signSenderAddress = senderAddress;
        }
    }
    else
    {
        if(height >= Params().GetConsensus().QIP5Height)
        {
            // If no sender address provided set to the default sender address
            SetDefaultSignSenderAddress(*pwallet, signSenderAddress, coinControl);
        }
    }
    EnsureWalletIsUnlocked(*pwallet);

    CAmount nGasFee=nGasPrice*nGasLimit;

    const auto bal = GetBalance(*pwallet);
    CAmount curBalance = bal.m_mine_trusted;
    if(fPsbt) curBalance += bal.m_watchonly_trusted;

    // Check amount
    if (nGasFee <= 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid amount");

    if (nGasFee > curBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");

    // Select default coin that will pay for the contract if none selected
    if(!coinControl.HasSelected() && !SetDefaultPayForContractAddress(*pwallet, coinControl))
        throw JSONRPCError(RPC_TYPE_ERROR, "Does not have any P2PK or P2PKH unspent outputs to pay for the contract.");

    // Build OP_EXEC script
    CScript scriptPubKey = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(nGasLimit) << CScriptNum(nGasPrice) << ParseHex(bytecode) <<OP_CREATE;
    if(height >= Params().GetConsensus().QIP5Height)
    {
        if(IsValidDestination(signSenderAddress))
        {
            if (!pwallet->HasPrivateKey(signSenderAddress, coinControl.fAllowWatchOnly)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Private key not available");
            }
            CKeyID key_id = pwallet->GetKeyForDestination(signSenderAddress);
            std::vector<unsigned char> scriptSig;
            scriptPubKey = (CScript() << CScriptNum(addresstype::PUBKEYHASH) << ToByteVector(key_id) << ToByteVector(scriptSig) << OP_SENDER) + scriptPubKey;
        }
        else
        {
            // OP_SENDER will always be used when QIP5Height is active
            throw JSONRPCError(RPC_TYPE_ERROR, "Sender address fail to set for OP_SENDER.");
        }
    }

    // Create and send the transaction
    CAmount nFeeRequired;
    bilingual_str error;
    std::string strError;
    std::vector<CRecipient> vecSend;
    int nChangePosRet = -1;
    CRecipient recipient = {scriptPubKey, 0, false};
    vecSend.push_back(recipient);

    bool sign = !fPsbt;
    CTransactionRef tx;
    FeeCalculation fee_calc_out;
    if (!CreateTransaction(*pwallet, vecSend, tx, nFeeRequired, nChangePosRet, error, coinControl, fee_calc_out, sign, nGasFee, true, signSenderAddress)) {
        if (nFeeRequired > curBalance)
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds!", FormatMoney(nFeeRequired));
        else strError = error.original;
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }

    CTxDestination txSenderDest;
    pwallet->GetSenderDest(*tx, txSenderDest, sign);

    if (fHasSender && !(senderAddress == txSenderDest)){
           throw JSONRPCError(RPC_TYPE_ERROR, "Sender could not be set, transaction was not committed!");
    }

    UniValue result(UniValue::VOBJ);
    if(fPsbt){
        // Make a blank psbt
        PartiallySignedTransaction psbtx;
        CMutableTransaction rawTx = CMutableTransaction(*tx);
        psbtx.tx = rawTx;
        for (unsigned int i = 0; i < rawTx.vin.size(); ++i) {
            psbtx.inputs.push_back(PSBTInput());
        }
        for (unsigned int i = 0; i < rawTx.vout.size(); ++i) {
            psbtx.outputs.push_back(GetPsbtOutput(rawTx.vout[i], *pwallet));
        }

        // Fill transaction with out data but don't sign
        bool bip32derivs = true;
        bool complete = true;
        const TransactionError err = pwallet->FillPSBT(psbtx, complete, 1, false, bip32derivs);
        if (err != TransactionError::OK) {
            throw JSONRPCTransactionError(err);
        }

        // Serialize the PSBT
        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << psbtx;
        result.pushKV("psbt", EncodeBase64(ssTx.str()));

        // Add sender information
        CTxDestination txSenderAdress(txSenderDest);
        CKeyID keyid = pwallet->GetKeyForDestination(txSenderAdress);
        result.pushKV("sender", EncodeDestination(txSenderAdress));
        result.pushKV("hash160", HexStr(valtype(keyid.begin(),keyid.end())));
    }
    else if(fBroadcast){
    pwallet->CommitTransaction(tx, {}, {});

    std::string txId=tx->GetHash().GetHex();
    result.pushKV("txid", txId);

    CTxDestination txSenderAdress(txSenderDest);
    CKeyID keyid = pwallet->GetKeyForDestination(txSenderAdress);

    result.pushKV("sender", EncodeDestination(txSenderAdress));
    result.pushKV("hash160", HexStr(valtype(keyid.begin(),keyid.end())));

    std::vector<unsigned char> SHA256TxVout(32);
    std::vector<unsigned char> contractAddress(20);
    std::vector<unsigned char> txIdAndVout(tx->GetHash().begin(), tx->GetHash().end());
    uint32_t voutNumber=0;
    for (const CTxOut& txout : tx->vout) {
        if(txout.scriptPubKey.HasOpCreate()){
            std::vector<unsigned char> voutNumberChrs;
            if (voutNumberChrs.size() < sizeof(voutNumber))voutNumberChrs.resize(sizeof(voutNumber));
            std::memcpy(voutNumberChrs.data(), &voutNumber, sizeof(voutNumber));
            txIdAndVout.insert(txIdAndVout.end(),voutNumberChrs.begin(),voutNumberChrs.end());
            break;
        }
        voutNumber++;
    }
    CSHA256().Write(txIdAndVout.data(), txIdAndVout.size()).Finalize(SHA256TxVout.data());
    CRIPEMD160().Write(SHA256TxVout.data(), SHA256TxVout.size()).Finalize(contractAddress.data());
    result.pushKV("address", HexStr(contractAddress));
    }else{
    std::string strHex = EncodeHexTx(*tx, RPCSerializationFlags());
    result.pushKV("raw transaction", strHex);
    }
    return result;
},
    };
}

UniValue SendToContract(CWallet& wallet, const UniValue& params, ChainstateManager& chainman)
{
    uint64_t blockGasLimit = 0, minGasPrice = 0;
    CAmount nGasPrice = 0;
    int height = 0;
    getDgpData(blockGasLimit, minGasPrice, nGasPrice, &height, &chainman);

    std::string contractaddress = params[0].get_str();
    if(contractaddress.size() != 40 || !CheckHex(contractaddress))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Incorrect contract address");

    dev::Address addrAccount(contractaddress);
    {
        LOCK(cs_main);
        if(!globalState->addressInUse(addrAccount))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "contract address does not exist");
    }

    std::string datahex = params[1].get_str();
    if(datahex.size() % 2 != 0 || !CheckHex(datahex))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid data (data not hex)");

    CAmount nAmount = 0;
    if (params.size() > 2){
        nAmount = AmountFromValue(params[2]);
        if (nAmount < 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for send");
    }

    uint64_t nGasLimit=DEFAULT_GAS_LIMIT_OP_SEND;
    if (params.size() > 3){
        nGasLimit = params[3].get_int64();
        if (nGasLimit > blockGasLimit)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit (Maximum is: "+i64tostr(blockGasLimit)+")");
        if (nGasLimit < MINIMUM_GAS_LIMIT)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit (Minimum is: "+i64tostr(MINIMUM_GAS_LIMIT)+")");
        if (nGasLimit <= 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit");
    }

    if (params.size() > 4){
        nGasPrice = AmountFromValue(params[4]);
        if (nGasPrice <= 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice");
        CAmount maxRpcGasPrice = gArgs.GetIntArg("-rpcmaxgasprice", MAX_RPC_GAS_PRICE);
        if (nGasPrice > (int64_t)maxRpcGasPrice)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice, Maximum allowed in RPC calls is: "+FormatMoney(maxRpcGasPrice)+" (use -rpcmaxgasprice to change it)");
        if (nGasPrice < (int64_t)minGasPrice)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice (Minimum is: "+FormatMoney(minGasPrice)+")");
    }

    bool fHasSender=false;
    CTxDestination senderAddress;
    if (params.size() > 5){
        senderAddress = DecodeDestination(params[5].get_str());
        if (!IsValidDestination(senderAddress))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Qtum address to send from");
        if (!IsValidContractSenderAddress(senderAddress))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid contract sender address. Only P2PK and P2PKH allowed");
        else
            fHasSender=true;
    }

    bool fBroadcast=true;
    if (params.size() > 6){
        fBroadcast=params[6].get_bool();
    }

    bool fChangeToSender=true;
    if (params.size() > 7){
        fChangeToSender=params[7].get_bool();
    }

    bool fPsbt=wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    if (params.size() > 8){
        fPsbt=params[8].get_bool();
    }
    if(fPsbt) fBroadcast=false;

    CCoinControl coinControl;
    if(fPsbt) coinControl.fAllowWatchOnly = true;

    CTxDestination signSenderAddress = CNoDestination();
    if(fHasSender){
        // Find a UTXO with sender address
        std::vector<COutput> vecOutputs;

        coinControl.fAllowOtherInputs=true;

        AvailableCoins(wallet, vecOutputs, NULL);

        for (const COutput& out : vecOutputs) {

            CTxDestination destAdress;
            const CScript& scriptPubKey = out.tx->tx->vout[out.i].scriptPubKey;
            bool fValidAddress = out.fSpendable && ExtractDestination(scriptPubKey, destAdress);

            if (!fValidAddress || senderAddress != destAdress)
                continue;

            coinControl.Select(COutPoint(out.tx->GetHash(),out.i));

            break;

        }

        if(coinControl.HasSelected())
        {
            // Change to the sender
            if(fChangeToSender){
                coinControl.destChange=senderAddress;
            }
        }
        else
        {
            // Create op sender transaction when op sender is activated
            if(!(height >= Params().GetConsensus().QIP5Height))
                throw JSONRPCError(RPC_TYPE_ERROR, "Sender address does not have any unspent outputs");
        }

        if(height >= Params().GetConsensus().QIP5Height)
        {
            // Set the sender address
            signSenderAddress = senderAddress;
        }
    }
    else
    {
        if(height >= Params().GetConsensus().QIP5Height)
        {
            // If no sender address provided set to the default sender address
            SetDefaultSignSenderAddress(wallet, signSenderAddress, coinControl);
        }
    }

    EnsureWalletIsUnlocked(wallet);

    CAmount nGasFee=nGasPrice*nGasLimit;

    const auto bal = GetBalance(wallet);
    CAmount curBalance = bal.m_mine_trusted;
    if(fPsbt) curBalance += bal.m_watchonly_trusted;

    // Check amount
    if (nGasFee <= 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid amount for gas fee");

    if (nAmount+nGasFee > curBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");

    // Select default coin that will pay for the contract if none selected
    if(!coinControl.HasSelected() && !SetDefaultPayForContractAddress(wallet, coinControl))
        throw JSONRPCError(RPC_TYPE_ERROR, "Does not have any P2PK or P2PKH unspent outputs to pay for the contract.");

    // Build OP_EXEC_ASSIGN script
    CScript scriptPubKey = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(nGasLimit) << CScriptNum(nGasPrice) << ParseHex(datahex) << ParseHex(contractaddress) << OP_CALL;
    if(height >= Params().GetConsensus().QIP5Height)
    {
        if(IsValidDestination(signSenderAddress))
        {
            if (!wallet.HasPrivateKey(signSenderAddress, coinControl.fAllowWatchOnly)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Private key not available");
            }
            CKeyID key_id = wallet.GetKeyForDestination(signSenderAddress);
            std::vector<unsigned char> scriptSig;
            scriptPubKey = (CScript() << CScriptNum(addresstype::PUBKEYHASH) << ToByteVector(key_id) << ToByteVector(scriptSig) << OP_SENDER) + scriptPubKey;
        }
        else
        {
            // OP_SENDER will always be used when QIP5Height is active
            throw JSONRPCError(RPC_TYPE_ERROR, "Sender address fail to set for OP_SENDER.");
        }
    }

    // Create and send the transaction
    CAmount nFeeRequired;
    bilingual_str error;
    std::string strError;
    std::vector<CRecipient> vecSend;
    int nChangePosRet = -1;
    CRecipient recipient = {scriptPubKey, nAmount, false};
    vecSend.push_back(recipient);

    bool sign = !fPsbt;
    CTransactionRef tx;
    FeeCalculation fee_calc_out;
    if (!CreateTransaction(wallet, vecSend, tx, nFeeRequired, nChangePosRet, error, coinControl, fee_calc_out, sign, nGasFee, true, signSenderAddress)) {
        if (nFeeRequired > curBalance)
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds!", FormatMoney(nFeeRequired));
        else strError = error.original;
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }

    CTxDestination txSenderDest;
    wallet.GetSenderDest(*tx, txSenderDest, sign);

    if (fHasSender && !(senderAddress == txSenderDest)){
        throw JSONRPCError(RPC_TYPE_ERROR, "Sender could not be set, transaction was not committed!");
    }

    UniValue result(UniValue::VOBJ);

    if(fPsbt){
        // Make a blank psbt
        PartiallySignedTransaction psbtx;
        CMutableTransaction rawTx = CMutableTransaction(*tx);
        psbtx.tx = rawTx;
        for (unsigned int i = 0; i < rawTx.vin.size(); ++i) {
            psbtx.inputs.push_back(PSBTInput());
        }
        for (unsigned int i = 0; i < rawTx.vout.size(); ++i) {
            psbtx.outputs.push_back(GetPsbtOutput(rawTx.vout[i], wallet));
        }

        // Fill transaction with out data but don't sign
        bool bip32derivs = true;
        bool complete = true;
        const TransactionError err = wallet.FillPSBT(psbtx, complete, 1, false, bip32derivs);
        if (err != TransactionError::OK) {
            throw JSONRPCTransactionError(err);
        }

        // Serialize the PSBT
        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << psbtx;
        result.pushKV("psbt", EncodeBase64(ssTx.str()));

        // Add sender information
        CTxDestination txSenderAdress(txSenderDest);
        CKeyID keyid = wallet.GetKeyForDestination(txSenderAdress);
        result.pushKV("sender", EncodeDestination(txSenderAdress));
        result.pushKV("hash160", HexStr(valtype(keyid.begin(),keyid.end())));
    }
    else if(fBroadcast){
        wallet.CommitTransaction(tx, {}, {});

        std::string txId=tx->GetHash().GetHex();
        result.pushKV("txid", txId);

        CTxDestination txSenderAdress(txSenderDest);
        CKeyID keyid = wallet.GetKeyForDestination(txSenderAdress);

        result.pushKV("sender", EncodeDestination(txSenderAdress));
        result.pushKV("hash160", HexStr(valtype(keyid.begin(),keyid.end())));
    }else{
        std::string strHex = EncodeHexTx(*tx, RPCSerializationFlags());
        result.pushKV("raw transaction", strHex);
    }

    return result;
}

/**
 * @brief The SendToken class Write token data
 */
class SendToken : public CallToken
{
public:
    SendToken(CWallet& _wallet,
              ChainstateManager& _chainman):
        CallToken(_chainman),
        wallet(_wallet)
    {}

    bool execValid(const int& func, const bool& sendTo) override
    {
        return sendTo ? func != -1 : CallToken::execValid(func, sendTo);
    }

    bool exec(const bool& sendTo, const std::map<std::string, std::string>& lstParams, std::string& result, std::string& message) override
    {
        if(!sendTo)
            return CallToken::exec(sendTo, lstParams, result, message);

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

        // Set amount
        it = lstParams.find(paramAmount());
        if(it != lstParams.end())
        {
            if(params.size() == 2)
                params.push_back(it->second);
            else
                return false;
        }

        // Set gas limit
        it = lstParams.find(paramGasLimit());
        if(it != lstParams.end())
        {
            if(params.size() == 3) {
                UniValue param(UniValue::VNUM);
                param.setInt(atoi64(it->second));
                params.push_back(param);
            }
            else
                return false;
        }

        // Set gas price
        it = lstParams.find(paramGasPrice());
        if(it != lstParams.end())
        {
            if(params.size() == 4)
                params.push_back(it->second);
            else
                return false;
        }

        // Set sender
        it = lstParams.find(paramSender());
        if(it != lstParams.end())
        {
            if(params.size() == 5)
                params.push_back(it->second);
            else
                return false;
        }

        // Set broadcast
        it = lstParams.find(paramBroadcast());
        if(it != lstParams.end())
        {
            if(params.size() == 6) {
                bool val = it->second == "true" ? true : false;
                UniValue param(UniValue::VBOOL);
                param.setBool(val);
                params.push_back(param);
            }
            else
                return false;
        }

        // Set change to sender
        it = lstParams.find(paramChangeToSender());
        if(it != lstParams.end())
        {
            if(params.size() == 7) {
                bool val = it->second == "true" ? true : false;
                UniValue param(UniValue::VBOOL);
                param.setBool(val);
                params.push_back(param);
            }
            else
                return false;
        }

        // Set psbt
        it = lstParams.find(paramPsbt());
        if(it != lstParams.end())
        {
            if(params.size() == 8) {
                bool val = it->second == "true" ? true : false;
                UniValue param(UniValue::VBOOL);
                param.setBool(val);
                params.push_back(param);
            }
            else
                return false;
        }

        // Get execution result
        UniValue response = SendToContract(wallet, params, chainman);
        if(!response.isObject())
            return false;
        if(privateKeysDisabled())
        {
            if(!response.exists("psbt"))
                return false;
        }
        else
        {
            if(!response.exists("txid"))
                return false;
        }
        result = privateKeysDisabled() ? response["psbt"].get_str() : response["txid"].get_str();

        return true;
    }

    bool privateKeysDisabled() override
    {
        return wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    }

private:
    CWallet& wallet;
};

RPCHelpMan sendtocontract()
{
    uint64_t blockGasLimit = 0, minGasPrice = 0;
    CAmount nGasPrice = 0;
    getDgpData(blockGasLimit, minGasPrice, nGasPrice);

    return RPCHelpMan{"sendtocontract",
                    "\nSend funds and data to a contract." +
                    HELP_REQUIRING_PASSPHRASE,
                    {
                        {"contractaddress", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The contract address that will receive the funds and data."},
                        {"datahex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "data to send."},
                        {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "The amount in " + CURRENCY_UNIT + " to send. eg 0.1, default: 0"},
                        {"gasLimit", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "gasLimit, default: "+i64tostr(DEFAULT_GAS_LIMIT_OP_SEND)+", max: "+i64tostr(blockGasLimit)},
                        {"gasPrice", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "gasPrice Qtum price per gas unit, default: "+FormatMoney(nGasPrice)+", min:"+FormatMoney(minGasPrice)},
                        {"senderAddress", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "The qtum address that will be used as sender."},
                        {"broadcast", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Whether to broadcast the transaction or not, default: true."},
                        {"changeToSender", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Return the change to the sender, default: true."},
                        {"psbt", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Create partially signed transaction."},
                    },
                    {
                        RPCResult{"if broadcast is set to true",
                            RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR, "psbt", "The base64-encoded unsigned PSBT of the new transaction. Only returned when wallet private keys are disabled."},
                                {RPCResult::Type::STR_HEX, "txid", "The transaction id. Only returned when wallet private keys are enabled."},
                                {RPCResult::Type::STR, "sender", CURRENCY_UNIT + " address of the sender"},
                                {RPCResult::Type::STR_HEX, "hash160", "Ripemd-160 hash of the sender"}
                            },
                        },
                        RPCResult{"if broadcast is set to false",
                            RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR_HEX, "raw transaction", "The hex string of the raw transaction"}
                            },
                        },
                    },
                    RPCExamples{
                    HelpExampleCli("sendtocontract", "\"c6ca2697719d00446d4ea51f6fac8fd1e9310214\" \"54f6127f\"")
                    + HelpExampleCli("sendtocontract", "\"c6ca2697719d00446d4ea51f6fac8fd1e9310214\" \"54f6127f\" 12.0015 6000000 "+FormatMoney(minGasPrice)+" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\"")
                    },
            [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    ChainstateManager& chainman = pwallet->chain().chainman();

    LOCK(pwallet->cs_wallet);

    return SendToContract(*pwallet, request.params, chainman);
},
    };
}

} // namespace wallet
