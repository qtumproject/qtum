#include <key_io.h>
#include <core_io.h>
#include <rpc/util.h>
#include <rpc/server.h>
#include <rpc/contract_util.h>
#include <util/moneystr.h>
#include <common/system.h>
#include <util/signstr.h>
#include <util/tokenstr.h>
#include <wallet/rpc/util.h>
#include <wallet/wallet.h>
#include <wallet/coincontrol.h>
#include <wallet/spend.h>
#include <wallet/receive.h>
#include <qtum/qtumdelegation.h>
#include <validation.h>
#include <wallet/rpc/contract.h>
#include <common/args.h>

#include <univalue.h>

namespace wallet {

bool SetDefaultPayForContractAddress(const CWallet& wallet, CCoinControl & coinControl)
{
    // Set default coin to pay for the contract
    // Select any valid unspent output that can be used to pay for the contract
    coinControl.m_allow_other_inputs = true;
    coinControl.m_include_unsafe_inputs = true;

    std::vector<COutput> vecOutputs = AvailableCoins(wallet, &coinControl).All();

    for (const COutput& out : vecOutputs) {
        CTxDestination destAdress;
        const CScript& scriptPubKey = out.txout.scriptPubKey;
        bool fValidAddress = out.spendable && ExtractDestination(scriptPubKey, destAdress, nullptr, true)
                && IsValidContractSenderAddress(destAdress);

        if (!fValidAddress)
            continue;

        coinControl.Select(out.outpoint);
        break;
    }

    return coinControl.HasSelected();
}

bool SetDefaultSignSenderAddress(const CWallet& wallet, CTxDestination& destAdress, CCoinControl & coinControl)
{
    // Set default sender address if none provided
    // Select any valid unspent output that can be used for contract sender address
    coinControl.m_allow_other_inputs = true;
    coinControl.m_include_unsafe_inputs = true;

    std::vector<COutput> vecOutputs = AvailableCoins(wallet, &coinControl).All();

    for (const COutput& out : vecOutputs) {
        const CScript& scriptPubKey = out.txout.scriptPubKey;
        bool fValidAddress = out.spendable && ExtractDestination(scriptPubKey, destAdress, nullptr, true)
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
                    {"gaslimit", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "gasLimit, default: "+i64tostr(DEFAULT_GAS_LIMIT_OP_CREATE)+", max: "+i64tostr(blockGasLimit)},
                    {"gasprice", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "gasPrice QTUM price per gas unit, default: "+FormatMoney(nGasPrice)+", min:"+FormatMoney(minGasPrice)},
                    {"senderaddress", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "The qtum address that will be used to create the contract."},
                    {"broadcast", RPCArg::Type::BOOL, RPCArg::Default{true}, "Whether to broadcast the transaction or not."},
                    {"changetosender", RPCArg::Type::BOOL, RPCArg::Default{true}, "Return the change to the sender."},
                    {"psbt", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Create partially signed transaction."},
                },
                {
                    RPCResult{"if broadcast is set to true",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR_HEX, "txid", "The transaction id. Only returned when wallet private keys are enabled."},
                            {RPCResult::Type::STR, "sender", CURRENCY_UNIT + " address of the sender"},
                            {RPCResult::Type::STR_HEX, "hash160", "Ripemd-160 hash of the sender"},
                            {RPCResult::Type::STR, "address", "Expected contract address"}
                        },
                    },
                    RPCResult{"if broadcast is set to false",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR_HEX, "raw transaction", "The hex string of the raw transaction"},
                        },
                    },
                    RPCResult{"if psbt is set to true",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR, "psbt", "The base64-encoded unsigned PSBT of the new transaction. Only returned when wallet private keys are disabled."},
                            {RPCResult::Type::STR, "sender", CURRENCY_UNIT + " address of the sender"},
                            {RPCResult::Type::STR_HEX, "hash160", "Ripemd-160 hash of the sender"},
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
    if (!request.params[1].isNull()){
        nGasLimit = request.params[1].getInt<int64_t>();
        if (nGasLimit > blockGasLimit)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit (Maximum is: "+i64tostr(blockGasLimit)+")");
        if (nGasLimit < MINIMUM_GAS_LIMIT)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit (Minimum is: "+i64tostr(MINIMUM_GAS_LIMIT)+")");
        if (nGasLimit <= 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit");
    }

    if (!request.params[2].isNull()){
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
    if (!request.params[3].isNull()){
        senderAddress = DecodeDestination(request.params[3].get_str());
        if (!IsValidDestination(senderAddress))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Qtum address to send from");
        if (!IsValidContractSenderAddress(senderAddress))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid contract sender address. Only P2PK and P2PKH allowed");
        else
            fHasSender=true;
    }

    bool fBroadcast=true;
    if (!request.params[4].isNull()){
        fBroadcast=request.params[4].get_bool();
    }

    bool fChangeToSender=true;
    if (!request.params[5].isNull()){
        fChangeToSender=request.params[5].get_bool();
    }

    bool fPsbt=pwallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    if (!request.params[6].isNull()){
        fPsbt=request.params[6].get_bool();
    }
    if(fPsbt) fBroadcast=false;

    CCoinControl coinControl;
    if(fPsbt) coinControl.fAllowWatchOnly = true;

    CTxDestination signSenderAddress = CNoDestination();
    if(fHasSender){
        // Find a UTXO with sender address
        coinControl.m_allow_other_inputs = true;

        assert(pwallet != NULL);
        std::vector<COutput> vecOutputs = AvailableCoins(*pwallet, &coinControl).All();

        for (const COutput& out : vecOutputs) {
            CTxDestination destAdress;
            const CScript& scriptPubKey = out.txout.scriptPubKey;
            bool fValidAddress = out.spendable && ExtractDestination(scriptPubKey, destAdress, nullptr, true);

            if (!fValidAddress || senderAddress != destAdress)
                continue;

            coinControl.Select(out.outpoint);

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
    std::vector<CRecipient> vecSend;
    CRecipient recipient = {CNoDestination(scriptPubKey), 0, false};
    vecSend.push_back(recipient);

    bool sign = !fPsbt;
    auto res = CreateTransaction(*pwallet, vecSend,  std::nullopt, coinControl, sign, nGasFee, true, signSenderAddress);
    if (!res) {
        throw JSONRPCError(RPC_WALLET_ERROR, util::ErrorString(res).original);
    }
    CTransactionRef tx = res->tx;

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
        const auto err{pwallet->FillPSBT(psbtx, complete, 1, false, bip32derivs)};
        if (err) {
            throw JSONRPCPSBTError(*err);
        }

        // Serialize the PSBT
        DataStream ssTx;
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
    uint256 txHash = tx->GetHash().ToUint256();
    std::vector<unsigned char> txIdAndVout(txHash.begin(), txHash.end());
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
    std::string strHex = EncodeHexTx(*tx);
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
    if (!params[2].isNull()){
        nAmount = AmountFromValue(params[2]);
        if (nAmount < 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for send");
    }

    uint64_t nGasLimit=DEFAULT_GAS_LIMIT_OP_SEND;
    if (!params[3].isNull()){
        nGasLimit = params[3].getInt<int64_t>();
        if (nGasLimit > blockGasLimit)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit (Maximum is: "+i64tostr(blockGasLimit)+")");
        if (nGasLimit < MINIMUM_GAS_LIMIT)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit (Minimum is: "+i64tostr(MINIMUM_GAS_LIMIT)+")");
        if (nGasLimit <= 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit");
    }

    if (!params[4].isNull()){
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
    if (!params[5].isNull()){
        senderAddress = DecodeDestination(params[5].get_str());
        if (!IsValidDestination(senderAddress))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Qtum address to send from");
        if (!IsValidContractSenderAddress(senderAddress))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid contract sender address. Only P2PK and P2PKH allowed");
        else
            fHasSender=true;
    }

    bool fBroadcast=true;
    if (!params[6].isNull()){
        fBroadcast=params[6].get_bool();
    }

    bool fChangeToSender=true;
    if (!params[7].isNull()){
        fChangeToSender=params[7].get_bool();
    }

    bool fPsbt=wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    if (!params[8].isNull()){
        fPsbt=params[8].get_bool();
    }
    if(fPsbt) fBroadcast=false;

    CCoinControl coinControl;
    if(fPsbt) coinControl.fAllowWatchOnly = true;

    CTxDestination signSenderAddress = CNoDestination();
    if(fHasSender){
        // Find a UTXO with sender address
        coinControl.m_allow_other_inputs = true;

        std::vector<COutput> vecOutputs = AvailableCoins(wallet, &coinControl).All();

        for (const COutput& out : vecOutputs) {

            CTxDestination destAdress;
            const CScript& scriptPubKey = out.txout.scriptPubKey;
            bool fValidAddress = out.spendable && ExtractDestination(scriptPubKey, destAdress, nullptr, true);

            if (!fValidAddress || senderAddress != destAdress)
                continue;

            coinControl.Select(out.outpoint);

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
    std::vector<CRecipient> vecSend;
    CRecipient recipient = {CNoDestination(scriptPubKey), nAmount, false};
    vecSend.push_back(recipient);

    bool sign = !fPsbt;
    auto res = CreateTransaction(wallet, vecSend,  std::nullopt, coinControl, sign, nGasFee, true, signSenderAddress);
    if (!res) {
        throw JSONRPCError(RPC_WALLET_ERROR, util::ErrorString(res).original);
    }
    CTransactionRef tx = res->tx;

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
        const auto err{wallet.FillPSBT(psbtx, complete, 1, false, bip32derivs)};
        if (err) {
            throw JSONRPCPSBTError(*err);
        }

        // Serialize the PSBT
        DataStream ssTx;
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
        std::string strHex = EncodeHexTx(*tx);
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
                        {"gaslimit", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "gasLimit, default: "+i64tostr(DEFAULT_GAS_LIMIT_OP_SEND)+", max: "+i64tostr(blockGasLimit)},
                        {"gasprice", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "gasPrice Qtum price per gas unit, default: "+FormatMoney(nGasPrice)+", min:"+FormatMoney(minGasPrice)},
                        {"senderaddress", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "The qtum address that will be used as sender."},
                        {"broadcast", RPCArg::Type::BOOL, RPCArg::Default{true}, "Whether to broadcast the transaction or not."},
                        {"changetosender", RPCArg::Type::BOOL, RPCArg::Default{true}, "Return the change to the sender."},
                        {"psbt", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Create partially signed transaction."},
                    },
                    {
                        RPCResult{"if broadcast is set to true",
                            RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR_HEX, "txid", "The transaction id. Only returned when wallet private keys are enabled."},
                                {RPCResult::Type::STR, "sender", CURRENCY_UNIT + " address of the sender"},
                                {RPCResult::Type::STR_HEX, "hash160", "Ripemd-160 hash of the sender"}
                            },
                        },
                        RPCResult{"if broadcast is set to false",
                            RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR_HEX, "raw transaction", "The hex string of the raw transaction"},
                            },
                        },
                        RPCResult{"if psbt is set to true",
                            RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR, "psbt", "The base64-encoded unsigned PSBT of the new transaction. Only returned when wallet private keys are disabled."},
                                {RPCResult::Type::STR, "sender", CURRENCY_UNIT + " address of the sender"},
                                {RPCResult::Type::STR_HEX, "hash160", "Ripemd-160 hash of the sender"},
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

RPCHelpMan removedelegationforaddress()
{
    uint64_t blockGasLimit = 0, minGasPrice = 0;
    CAmount nGasPrice = 0;
    getDgpData(blockGasLimit, minGasPrice, nGasPrice);

    return RPCHelpMan{"removedelegationforaddress",
                    "\nRemove delegation for address." +
                    HELP_REQUIRING_PASSPHRASE,
                    {
                        {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The qtum address to remove delegation, the address will be used as sender too."},
                        {"gaslimit", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "gasLimit, default: "+i64tostr(DEFAULT_GAS_LIMIT_OP_SEND)+", max: "+i64tostr(blockGasLimit)},
                        {"gasprice", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "gasPrice Qtum price per gas unit, default: "+FormatMoney(nGasPrice)+", min:"+FormatMoney(minGasPrice)},
                    },
                    RPCResult{
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR, "psbt", /*optional=*/true, "The base64-encoded unsigned PSBT of the new transaction. Only returned when wallet private keys are disabled."},
                            {RPCResult::Type::STR_HEX, "txid", /*optional=*/true, "The transaction id. Only returned when wallet private keys are enabled."},
                            {RPCResult::Type::STR, "sender", CURRENCY_UNIT + " address of the sender"},
                            {RPCResult::Type::STR_HEX, "hash160", "Ripemd-160 hash of the sender"}
                        }
                    },
                    RPCExamples{
                    HelpExampleCli("removedelegationforaddress", " \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 6000000 "+FormatMoney(minGasPrice))
                    },
            [&,nGasPrice](const RPCHelpMan& self, const JSONRPCRequest& request) mutable -> UniValue
{

    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    ChainstateManager& chainman = pwallet->chain().chainman();

    LOCK(pwallet->cs_wallet);

    // Get send to contract parameters for removing delegation for address
    UniValue params(UniValue::VARR);
    UniValue contractaddress = HexStr(Params().GetConsensus().delegationsAddress);
    UniValue datahex = QtumDelegation::BytecodeRemove();
    UniValue amount = 0;
    UniValue gasLimit = !request.params[1].isNull() ? request.params[1] : DEFAULT_GAS_LIMIT_OP_SEND;
    UniValue gasPrice = !request.params[2].isNull() ? request.params[2] : FormatMoney(nGasPrice);
    UniValue senderaddress = request.params[0];

    // Add the send to contract parameters to the list
    params.push_back(contractaddress);
    params.push_back(datahex);
    params.push_back(amount);
    params.push_back(gasLimit);
    params.push_back(gasPrice);
    params.push_back(senderaddress);

    // Send to contract
    return SendToContract(*pwallet, params, chainman);
},
    };
}

RPCHelpMan setdelegateforaddress()
{
    uint64_t blockGasLimit = 0, minGasPrice = 0;
    CAmount nGasPrice = 0;
    getDgpData(blockGasLimit, minGasPrice, nGasPrice);

    return RPCHelpMan{"setdelegateforaddress",
                    "\nSet delegate for address." +
                    HELP_REQUIRING_PASSPHRASE,
                    {
                        {"staker", RPCArg::Type::STR, RPCArg::Optional::NO, "The qtum address for the staker."},
                        {"fee", RPCArg::Type::NUM, RPCArg::Optional::NO, "Percentage of the reward that will be paid to the staker."},
                        {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The qtum address that contain the coins that will be delegated to the staker, the address will be used as sender too."},
                        {"gaslimit", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "gasLimit, default: "+i64tostr(DEFAULT_GAS_LIMIT_OP_CREATE)+", max: "+i64tostr(blockGasLimit)},
                        {"gasprice", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "gasPrice Qtum price per gas unit, default: "+FormatMoney(nGasPrice)+", min:"+FormatMoney(minGasPrice)},
                    },
                    RPCResult{
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR, "psbt", /*optional=*/true, "The base64-encoded unsigned PSBT of the new transaction. Only returned when wallet private keys are disabled."},
                            {RPCResult::Type::STR_HEX, "txid", /*optional=*/true, "The transaction id. Only returned when wallet private keys are enabled."},
                            {RPCResult::Type::STR, "sender", CURRENCY_UNIT + " address of the sender"},
                            {RPCResult::Type::STR_HEX, "hash160", "Ripemd-160 hash of the sender"}
                        }
                    },
                    RPCExamples{
                    HelpExampleCli("setdelegateforaddress", " \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 10 \"QX1GkJdye9WoUnrE2v6ZQhQ72EUVDtGXQX\" 6000000 "+FormatMoney(minGasPrice))
                    },
        [&,nGasPrice](const RPCHelpMan& self, const JSONRPCRequest& request) mutable -> UniValue
{

    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    ChainstateManager& chainman = pwallet->chain().chainman();

    LOCK(pwallet->cs_wallet);

    // Get send to contract parameters for add delegation for address
    UniValue params(UniValue::VARR);
    UniValue contractaddress = HexStr(Params().GetConsensus().delegationsAddress);
    UniValue amount = 0;
    UniValue gasLimit = !request.params[3].isNull() ? request.params[3] : DEFAULT_GAS_LIMIT_OP_CREATE;
    UniValue gasPrice = !request.params[4].isNull() ? request.params[4] : FormatMoney(nGasPrice);
    UniValue senderaddress = request.params[2];
    bool fPsbt=pwallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);

    // Parse the staker address
    CTxDestination destStaker = DecodeDestination(request.params[0].get_str());
    if (!std::holds_alternative<PKHash>(destStaker)) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid contract address for staker. Only P2PK and P2PKH allowed");
    }
    PKHash pkhStaker = std::get<PKHash>(destStaker);

    // Parse the staker fee
    int fee = request.params[1].getInt<int>();
    if(fee < 0 || fee > 100)
        throw JSONRPCError(RPC_PARSE_ERROR, "The staker fee need to be between 0 and 100");

    // Parse the sender address
    CTxDestination destSender = DecodeDestination(senderaddress.get_str());
    if (!std::holds_alternative<PKHash>(destSender)) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid contract sender address. Only P2PK and P2PKH allowed");
    }
    PKHash pkhSender = std::get<PKHash>(destSender);

    // Get the private key for the sender address
    if (!pwallet->HasPrivateKey(destSender, fPsbt)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key not available for the sender address");
    }

    // Sign the  staker address
    std::vector<unsigned char> PoD;
    std::string hexStaker =  ToKeyID(pkhStaker).GetReverseHex();
    if(fPsbt)
    {
        PoD.insert(PoD.end(), hexStaker.begin(), hexStaker.end());
        PoD.resize(CPubKey::COMPACT_SIGNATURE_SIZE, 0);
    }
    else
    {
        std::string str_sig;
        SigningResult res = pwallet->SignMessage(hexStaker, pkhSender, str_sig);
        if(res == SigningResult::PRIVATE_KEY_NOT_AVAILABLE)
        {
            throw JSONRPCError(RPC_WALLET_ERROR, "Private key not available for the sender address");
        }
        if(res == SigningResult::SIGNING_FAILED)
        {
            throw JSONRPCError(RPC_WALLET_ERROR, "Fail to sign the staker address");
        }
        if(auto decodePoD = DecodeBase64(str_sig))
        {
            PoD = *decodePoD;
        }
    }

    // Serialize the data
    std::string datahex;
    std::string errorMessage;
    if(!QtumDelegation::BytecodeAdd(hexStaker, fee, PoD, datahex, errorMessage))
        throw JSONRPCError(RPC_TYPE_ERROR, errorMessage);

    // Add the send to contract parameters to the list
    params.push_back(contractaddress);
    params.push_back(datahex);
    params.push_back(amount);
    params.push_back(gasLimit);
    params.push_back(gasPrice);
    params.push_back(senderaddress);

    // Send to contract
    return SendToContract(*pwallet, params, chainman);
},
    };
}

RPCHelpMan qrc20approve()
{
    uint64_t blockGasLimit = 0, minGasPrice = 0;
    CAmount nGasPrice = 0;
    getDgpData(blockGasLimit, minGasPrice, nGasPrice);

    return RPCHelpMan{"qrc20approve",
                "\nOwner approves an address to spend some amount of tokens.\n",
                {
                    {"contractaddress", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The contract address."},
                    {"owneraddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The token owner qtum address."},
                    {"spenderaddress", RPCArg::Type::STR, RPCArg::Optional::NO,  "The token spender qtum address."},
                    {"amount", RPCArg::Type::STR, RPCArg::Optional::NO,  "The amount of tokens. eg 0.1"},
                    {"gaslimit", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "The gas limit, default: "+i64tostr(DEFAULT_GAS_LIMIT_OP_SEND)+", max: "+i64tostr(blockGasLimit)},
                    {"gasprice", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "The qtum price per gas unit, default: "+FormatMoney(nGasPrice)+", min:"+FormatMoney(minGasPrice)},
                    {"checkoutputs", RPCArg::Type::BOOL, RPCArg::Default{true}, "Check outputs before send"},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR, "psbt", /*optional=*/true, "The base64-encoded unsigned PSBT of the new transaction. Only returned when wallet private keys are disabled."},
                        {RPCResult::Type::STR_HEX, "txid", /*optional=*/true, "The transaction id. Only returned when wallet private keys are enabled."},
                    }
                },
                RPCExamples{
                    HelpExampleCli("qrc20approve", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QX1GkJdye9WoUnrE2v6ZQhQ72EUVDtGXQX\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
            + HelpExampleCli("qrc20approve", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QX1GkJdye9WoUnrE2v6ZQhQ72EUVDtGXQX\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1 "+i64tostr(DEFAULT_GAS_LIMIT_OP_SEND)+" "+FormatMoney(minGasPrice)+" true")
            + HelpExampleRpc("qrc20approve", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QX1GkJdye9WoUnrE2v6ZQhQ72EUVDtGXQX\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
            + HelpExampleRpc("qrc20approve", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QX1GkJdye9WoUnrE2v6ZQhQ72EUVDtGXQX\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1 "+i64tostr(DEFAULT_GAS_LIMIT_OP_SEND)+" "+FormatMoney(minGasPrice)+" true")
                },
            [&,nGasPrice](const RPCHelpMan& self, const JSONRPCRequest& request) mutable -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    ChainstateManager& chainman = pwallet->chain().chainman();

    LOCK(pwallet->cs_wallet);

    // Get mandatory parameters
    std::string contract = request.params[0].get_str();
    std::string owner = request.params[1].get_str();
    std::string spender = request.params[2].get_str();
    std::string tokenAmount = request.params[3].get_str();

    // Get gas limit
    uint64_t nGasLimit=DEFAULT_GAS_LIMIT_OP_SEND;
    if (!request.params[4].isNull()){
        nGasLimit = request.params[4].getInt<int64_t>();
    }

    // Get gas price
    if (!request.params[5].isNull()){
        nGasPrice = AmountFromValue(request.params[5]);
    }

    // Get check outputs flag
    bool fCheckOutputs = true;
    if (!request.params[6].isNull()){
        fCheckOutputs = request.params[6].get_bool();
    }

    // Set token parameters
    SendToken token(*pwallet, chainman);
    token.setAddress(contract);
    token.setSender(owner);
    token.setGasLimit(i64tostr(nGasLimit));
    token.setGasPrice(FormatMoney(nGasPrice));

    // Get decimals
    uint32_t decimals;
    if(!token.decimals(decimals))
        throw JSONRPCError(RPC_MISC_ERROR, "Fail to get decimals");

    // Get token amount to approve
    dev::s256 nTokenAmount;
    if(!ParseToken(decimals, tokenAmount, nTokenAmount))
        throw JSONRPCError(RPC_MISC_ERROR, "Fail to get token amount");

    // Check approve offline
    std::string value = nTokenAmount.str();
    bool success = false;
    if(fCheckOutputs)
    {
        token.setCheckGasForCall(true);
        if(!token.approve(spender, value, success) || !success)
            throw JSONRPCError(RPC_MISC_ERROR, "Fail offline check for approve token amount for spending");
    }

    // Approve value to spend
    if(!token.approve(spender, value, success, true))
        throw JSONRPCError(RPC_MISC_ERROR, "Fail to approve token amount for spending");

    UniValue result(UniValue::VOBJ);
    if(token.privateKeysDisabled())
    {
        result.pushKV("psbt", token.getPsbt());
    }
    else
    {
        result.pushKV("txid", token.getTxId());
    }
    return result;
},
    };
}

RPCHelpMan qrc20transfer()
{
    uint64_t blockGasLimit = 0, minGasPrice = 0;
    CAmount nGasPrice = 0;
    getDgpData(blockGasLimit, minGasPrice, nGasPrice);

    return RPCHelpMan{"qrc20transfer",
                "\nSend token amount to a given address.\n",
                {
                    {"contractaddress", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The contract address."},
                    {"owneraddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The token owner qtum address."},
                    {"addressto", RPCArg::Type::STR, RPCArg::Optional::NO,  "The qtum address to send funds to."},
                    {"amount", RPCArg::Type::STR, RPCArg::Optional::NO,  "The amount of tokens to send. eg 0.1"},
                    {"gaslimit", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "The gas limit, default: "+i64tostr(DEFAULT_GAS_LIMIT_OP_SEND)+", max: "+i64tostr(blockGasLimit)},
                    {"gasprice", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "The qtum price per gas unit, default: "+FormatMoney(nGasPrice)+", min:"+FormatMoney(minGasPrice)},
                    {"checkoutputs", RPCArg::Type::BOOL, RPCArg::Default{true}, "Check outputs before send"},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR, "psbt", /*optional=*/true, "The base64-encoded unsigned PSBT of the new transaction. Only returned when wallet private keys are disabled."},
                        {RPCResult::Type::STR_HEX, "txid", /*optional=*/true, "The transaction id. Only returned when wallet private keys are enabled."},
                    }
                },
                RPCExamples{
                    HelpExampleCli("qrc20transfer", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QX1GkJdye9WoUnrE2v6ZQhQ72EUVDtGXQX\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
            + HelpExampleCli("qrc20transfer", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QX1GkJdye9WoUnrE2v6ZQhQ72EUVDtGXQX\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1 "+i64tostr(DEFAULT_GAS_LIMIT_OP_SEND)+" "+FormatMoney(minGasPrice)+" true")
            + HelpExampleRpc("qrc20transfer", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QX1GkJdye9WoUnrE2v6ZQhQ72EUVDtGXQX\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
            + HelpExampleRpc("qrc20transfer", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QX1GkJdye9WoUnrE2v6ZQhQ72EUVDtGXQX\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1 "+i64tostr(DEFAULT_GAS_LIMIT_OP_SEND)+" "+FormatMoney(minGasPrice)+" true")
                },
            [&,nGasPrice](const RPCHelpMan& self, const JSONRPCRequest& request) mutable -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    ChainstateManager& chainman = pwallet->chain().chainman();

    LOCK(pwallet->cs_wallet);

    // Get mandatory parameters
    std::string contract = request.params[0].get_str();
    std::string owner = request.params[1].get_str();
    std::string address = request.params[2].get_str();
    std::string tokenAmount = request.params[3].get_str();

    // Get gas limit
    uint64_t nGasLimit=DEFAULT_GAS_LIMIT_OP_SEND;
    if (!request.params[4].isNull()){
        nGasLimit = request.params[4].getInt<int64_t>();
    }

    // Get gas price
    if (!request.params[5].isNull()){
        nGasPrice = AmountFromValue(request.params[5]);
    }

    // Get check outputs flag
    bool fCheckOutputs = true;
    if (!request.params[6].isNull()){
        fCheckOutputs = request.params[6].get_bool();
    }

    // Set token parameters
    SendToken token(*pwallet, chainman);
    token.setAddress(contract);
    token.setSender(owner);
    token.setGasLimit(i64tostr(nGasLimit));
    token.setGasPrice(FormatMoney(nGasPrice));

    // Get decimals
    uint32_t decimals;
    if(!token.decimals(decimals))
        throw JSONRPCError(RPC_MISC_ERROR, "Fail to get decimals");

    // Get token amount
    dev::s256 nTokenAmount;
    if(!ParseToken(decimals, tokenAmount, nTokenAmount))
        throw JSONRPCError(RPC_MISC_ERROR, "Fail to get token amount");

    // Get token owner balance
    std::string strBalance;
    if(!token.balanceOf(strBalance))
        throw JSONRPCError(RPC_MISC_ERROR, "Fail to get balance");

    // Check if balance is enough to cover it
    dev::s256 balance(strBalance);
    if(balance < nTokenAmount)
        throw JSONRPCError(RPC_MISC_ERROR, "Not enough token balance");

    // Check transfer offline
    std::string value = nTokenAmount.str();
    bool success = false;
    if(fCheckOutputs)
    {
        token.setCheckGasForCall(true);
        if(!token.transfer(address, value, success) || !success)
            throw JSONRPCError(RPC_MISC_ERROR, "Fail offline check for transfer token");
    }

    // Send token
    if(!token.transfer(address, value, success, true))
        throw JSONRPCError(RPC_MISC_ERROR, "Fail to transfer token");

    UniValue result(UniValue::VOBJ);
    if(token.privateKeysDisabled())
    {
        result.pushKV("psbt", token.getPsbt());
    }
    else
    {
        result.pushKV("txid", token.getTxId());
    }
    return result;
},
    };
}

RPCHelpMan qrc20transferfrom()
{
    uint64_t blockGasLimit = 0, minGasPrice = 0;
    CAmount nGasPrice = 0;
    getDgpData(blockGasLimit, minGasPrice, nGasPrice);

    return RPCHelpMan{"qrc20transferfrom",
                "\nSend token amount from selected address to a given address.\n",
                {
                    {"contractaddress", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The contract address."},
                    {"owneraddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The token owner qtum address."},
                    {"spenderaddress", RPCArg::Type::STR, RPCArg::Optional::NO,  "The token spender qtum address."},
                    {"receiveraddress", RPCArg::Type::STR, RPCArg::Optional::NO,  "The token receiver qtum address."},
                    {"amount", RPCArg::Type::STR, RPCArg::Optional::NO,  "The amount of token to send. eg 0.1"},
                    {"gaslimit", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "The gas limit, default: "+i64tostr(DEFAULT_GAS_LIMIT_OP_SEND)+", max: "+i64tostr(blockGasLimit)},
                    {"gasprice", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "The qtum price per gas unit, default: "+FormatMoney(nGasPrice)+", min:"+FormatMoney(minGasPrice)},
                    {"checkoutputs", RPCArg::Type::BOOL, RPCArg::Default{true}, "Check outputs before send"},
                 },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR, "psbt", /*optional=*/true, "The base64-encoded unsigned PSBT of the new transaction. Only returned when wallet private keys are disabled."},
                        {RPCResult::Type::STR_HEX, "txid", /*optional=*/true, "The transaction id. Only returned when wallet private keys are enabled."},
                    }
                },
                RPCExamples{
                    HelpExampleCli("qrc20transferfrom", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" \"QX1GkJdye9WoUnrE2v6ZQhQ72EUVDtGXQX\" \"QhZThdumK8EFRX8MziWzvjCdiQWRt7Mxdz\" 0.1")
            + HelpExampleCli("qrc20transferfrom", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" \"QX1GkJdye9WoUnrE2v6ZQhQ72EUVDtGXQX\" \"QhZThdumK8EFRX8MziWzvjCdiQWRt7Mxdz\" 0.1 "+i64tostr(DEFAULT_GAS_LIMIT_OP_SEND)+" "+FormatMoney(minGasPrice)+" true")
            + HelpExampleRpc("qrc20transferfrom", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" \"QX1GkJdye9WoUnrE2v6ZQhQ72EUVDtGXQX\" \"QhZThdumK8EFRX8MziWzvjCdiQWRt7Mxdz\" 0.1")
            + HelpExampleRpc("qrc20transferfrom", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" \"QX1GkJdye9WoUnrE2v6ZQhQ72EUVDtGXQX\" \"QhZThdumK8EFRX8MziWzvjCdiQWRt7Mxdz\" 0.1 "+i64tostr(DEFAULT_GAS_LIMIT_OP_SEND)+" "+FormatMoney(minGasPrice)+" true")
                },
            [&,nGasPrice](const RPCHelpMan& self, const JSONRPCRequest& request) mutable -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    ChainstateManager& chainman = pwallet->chain().chainman();

    LOCK(pwallet->cs_wallet);

    // Get mandatory parameters
    std::string contract = request.params[0].get_str();
    std::string owner = request.params[1].get_str();
    std::string spender = request.params[2].get_str();
    std::string receiver = request.params[3].get_str();
    std::string tokenAmount = request.params[4].get_str();

    // Get gas limit
    uint64_t nGasLimit=DEFAULT_GAS_LIMIT_OP_SEND;
    if (!request.params[5].isNull()){
        nGasLimit = request.params[5].getInt<int64_t>();
    }

    // Get gas price
    if (!request.params[6].isNull()){
        nGasPrice = AmountFromValue(request.params[6]);
    }

    // Get check outputs flag
    bool fCheckOutputs = true;
    if (!request.params[7].isNull()){
        fCheckOutputs = request.params[7].get_bool();
    }

    // Set token parameters
    SendToken token(*pwallet, chainman);
    token.setAddress(contract);
    token.setSender(spender);
    token.setGasLimit(i64tostr(nGasLimit));
    token.setGasPrice(FormatMoney(nGasPrice));

    // Get decimals
    uint32_t decimals;
    if(!token.decimals(decimals))
        throw JSONRPCError(RPC_MISC_ERROR, "Fail to get decimals");

    // Get token amount to spend
    dev::s256 nTokenAmount;
    if(!ParseToken(decimals, tokenAmount, nTokenAmount))
        throw JSONRPCError(RPC_MISC_ERROR, "Fail to get token amount");

    // Get token spender allowance
    std::string strAllowance;
    if(!token.allowance(owner, spender, strAllowance))
        throw JSONRPCError(RPC_MISC_ERROR, "Fail to get allowance");

    // Check if allowance is enough to cover it
    dev::s256 allowance(strAllowance);
    if(allowance < nTokenAmount)
        throw JSONRPCError(RPC_MISC_ERROR, "Not enough token allowance");

    // Check transfer from offline
    std::string value = nTokenAmount.str();
    bool success = false;
    if(fCheckOutputs)
    {
        token.setCheckGasForCall(true);
        if(!token.transferFrom(owner, receiver, value, success) || !success)
            throw JSONRPCError(RPC_MISC_ERROR, "Fail offline check for spend token amount from address");
    }

    // Transfer allowed token amount
    if(!token.transferFrom(owner, receiver, value, success, true))
        throw JSONRPCError(RPC_MISC_ERROR, "Fail to spend token amount from address");

    UniValue result(UniValue::VOBJ);
    if(token.privateKeysDisabled())
    {
        result.pushKV("psbt", token.getPsbt());
    }
    else
    {
        result.pushKV("txid", token.getTxId());
    }
    return result;
},
    };
}

RPCHelpMan qrc20burn()
{
    uint64_t blockGasLimit = 0, minGasPrice = 0;
    CAmount nGasPrice = 0;
    getDgpData(blockGasLimit, minGasPrice, nGasPrice);

    return RPCHelpMan{"qrc20burn",
                "\nBurns token amount from owner address.\n",
                {
                    {"contractaddress", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The contract address."},
                    {"owneraddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The token owner qtum address."},
                    {"amount", RPCArg::Type::STR, RPCArg::Optional::NO,  "The amount of tokens to burn. eg 0.1"},
                    {"gaslimit", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "The gas limit, default: "+i64tostr(DEFAULT_GAS_LIMIT_OP_SEND)+", max: "+i64tostr(blockGasLimit)},
                    {"gasprice", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "The qtum price per gas unit, default: "+FormatMoney(nGasPrice)+", min:"+FormatMoney(minGasPrice)},
                    {"checkoutputs", RPCArg::Type::BOOL, RPCArg::Default{true}, "Check outputs before send"},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR, "psbt", /*optional=*/true, "The base64-encoded unsigned PSBT of the new transaction. Only returned when wallet private keys are disabled."},
                        {RPCResult::Type::STR_HEX, "txid", /*optional=*/true, "The transaction id. Only returned when wallet private keys are enabled."},
                    }
                },
                RPCExamples{
                    HelpExampleCli("qrc20burn", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
            + HelpExampleCli("qrc20burn", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1 "+i64tostr(DEFAULT_GAS_LIMIT_OP_SEND)+" "+FormatMoney(minGasPrice)+" true")
            + HelpExampleRpc("qrc20burn", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
            + HelpExampleRpc("qrc20burn", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1 "+i64tostr(DEFAULT_GAS_LIMIT_OP_SEND)+" "+FormatMoney(minGasPrice)+" true")
                },
            [&,nGasPrice](const RPCHelpMan& self, const JSONRPCRequest& request) mutable -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    ChainstateManager& chainman = pwallet->chain().chainman();

    LOCK(pwallet->cs_wallet);

    // Get mandatory parameters
    std::string contract = request.params[0].get_str();
    std::string owner = request.params[1].get_str();
    std::string tokenAmount = request.params[2].get_str();

    // Get gas limit
    uint64_t nGasLimit=DEFAULT_GAS_LIMIT_OP_SEND;
    if (!request.params[3].isNull()){
        nGasLimit = request.params[3].getInt<int64_t>();
    }

    // Get gas price
    if (!request.params[4].isNull()){
        nGasPrice = AmountFromValue(request.params[4]);
    }

    // Get check outputs flag
    bool fCheckOutputs = true;
    if (!request.params[5].isNull()){
        fCheckOutputs = request.params[5].get_bool();
    }

    // Set token parameters
    SendToken token(*pwallet, chainman);
    token.setAddress(contract);
    token.setSender(owner);
    token.setGasLimit(i64tostr(nGasLimit));
    token.setGasPrice(FormatMoney(nGasPrice));

    // Get decimals
    uint32_t decimals;
    if(!token.decimals(decimals))
        throw JSONRPCError(RPC_MISC_ERROR, "Fail to get decimals");

    // Get token amount to burn
    dev::s256 nTokenAmount;
    if(!ParseToken(decimals, tokenAmount, nTokenAmount))
        throw JSONRPCError(RPC_MISC_ERROR, "Fail to get token amount");

    // Get token owner balance
    std::string strBalance;
    if(!token.balanceOf(strBalance))
        throw JSONRPCError(RPC_MISC_ERROR, "Fail to get balance");

    // Check if balance is enough to cover it
    dev::s256 balance(strBalance);
    if(balance < nTokenAmount)
        throw JSONRPCError(RPC_MISC_ERROR, "Not enough token balance");

    // Check burn offline
    std::string value = nTokenAmount.str();
    bool success = false;
    if(fCheckOutputs)
    {
        token.setCheckGasForCall(true);
        if(!token.burn(value, success) || !success)
            throw JSONRPCError(RPC_MISC_ERROR, "Fail offline check for burn token amount");
    }

    // Burn token amount
    if(!token.burn(value, success, true))
        throw JSONRPCError(RPC_MISC_ERROR, "Fail to burn token amount");

    UniValue result(UniValue::VOBJ);
    if(token.privateKeysDisabled())
    {
        result.pushKV("psbt", token.getPsbt());
    }
    else
    {
        result.pushKV("txid", token.getTxId());
    }
    return result;
},
    };
}

RPCHelpMan qrc20burnfrom()
{
    uint64_t blockGasLimit = 0, minGasPrice = 0;
    CAmount nGasPrice = 0;
    getDgpData(blockGasLimit, minGasPrice, nGasPrice);

    return RPCHelpMan{"qrc20burnfrom",
                "\nBurns token amount from a given address.\n",
                {
                    {"contractaddress", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The contract address."},
                    {"owneraddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The token owner qtum address."},
                    {"spenderaddress", RPCArg::Type::STR, RPCArg::Optional::NO,  "The token spender qtum address."},
                    {"amount", RPCArg::Type::STR, RPCArg::Optional::NO,  "The amount of token to burn. eg 0.1"},
                    {"gaslimit", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "The gas limit, default: "+i64tostr(DEFAULT_GAS_LIMIT_OP_SEND)+", max: "+i64tostr(blockGasLimit)},
                    {"gasprice", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "The qtum price per gas unit, default: "+FormatMoney(nGasPrice)+", min:"+FormatMoney(minGasPrice)},
                    {"checkoutputs", RPCArg::Type::BOOL, RPCArg::Default{true}, "Check outputs before send"},
                 },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR, "psbt", /*optional=*/true, "The base64-encoded unsigned PSBT of the new transaction. Only returned when wallet private keys are disabled."},
                        {RPCResult::Type::STR_HEX, "txid", /*optional=*/true, "The transaction id. Only returned when wallet private keys are enabled."},
                    }
                },
                RPCExamples{
                    HelpExampleCli("qrc20burnfrom", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QX1GkJdye9WoUnrE2v6ZQhQ72EUVDtGXQX\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
            + HelpExampleCli("qrc20burnfrom", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QX1GkJdye9WoUnrE2v6ZQhQ72EUVDtGXQX\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1 "+i64tostr(DEFAULT_GAS_LIMIT_OP_SEND)+" "+FormatMoney(minGasPrice)+" true")
            + HelpExampleRpc("qrc20burnfrom", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QX1GkJdye9WoUnrE2v6ZQhQ72EUVDtGXQX\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
            + HelpExampleRpc("qrc20burnfrom", "\"eb23c0b3e6042821da281a2e2364feb22dd543e3\" \"QX1GkJdye9WoUnrE2v6ZQhQ72EUVDtGXQX\" \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1 "+i64tostr(DEFAULT_GAS_LIMIT_OP_SEND)+" "+FormatMoney(minGasPrice)+" true")
                },
            [&,nGasPrice](const RPCHelpMan& self, const JSONRPCRequest& request) mutable -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    ChainstateManager& chainman = pwallet->chain().chainman();

    LOCK(pwallet->cs_wallet);

    // Get mandatory parameters
    std::string contract = request.params[0].get_str();
    std::string owner = request.params[1].get_str();
    std::string spender = request.params[2].get_str();
    std::string tokenAmount = request.params[3].get_str();

    // Get gas limit
    uint64_t nGasLimit=DEFAULT_GAS_LIMIT_OP_SEND;
    if (!request.params[4].isNull()){
        nGasLimit = request.params[4].getInt<int64_t>();
    }

    // Get gas price
    if (!request.params[5].isNull()){
        nGasPrice = AmountFromValue(request.params[5]);
    }

    // Get check outputs flag
    bool fCheckOutputs = true;
    if (!request.params[6].isNull()){
        fCheckOutputs = request.params[6].get_bool();
    }

    // Set token parameters
    SendToken token(*pwallet, chainman);
    token.setAddress(contract);
    token.setSender(spender);
    token.setGasLimit(i64tostr(nGasLimit));
    token.setGasPrice(FormatMoney(nGasPrice));

    // Get decimals
    uint32_t decimals;
    if(!token.decimals(decimals))
        throw JSONRPCError(RPC_MISC_ERROR, "Fail to get decimals");

    // Get token amount to burn
    dev::s256 nTokenAmount;
    if(!ParseToken(decimals, tokenAmount, nTokenAmount))
        throw JSONRPCError(RPC_MISC_ERROR, "Fail to get token amount");

    // Get token spender allowance
    std::string strAllowance;
    if(!token.allowance(owner, spender, strAllowance))
        throw JSONRPCError(RPC_MISC_ERROR, "Fail to get allowance");

    // Check if allowance is enough to cover it
    dev::s256 allowance(strAllowance);
    if(allowance < nTokenAmount)
        throw JSONRPCError(RPC_MISC_ERROR, "Not enough token allowance");

    // Check burn from offline
    std::string value = nTokenAmount.str();
    bool success = false;
    if(fCheckOutputs)
    {
        token.setCheckGasForCall(true);
        if(!token.burnFrom(owner, value, success, false) || !success)
            throw JSONRPCError(RPC_MISC_ERROR, "Fail offline check for burn token amount from address");
    }

    // Burn token amount
    if(!token.burnFrom(owner, value, success, true))
        throw JSONRPCError(RPC_MISC_ERROR, "Fail to burn token amount from address");

    UniValue result(UniValue::VOBJ);
    if(token.privateKeysDisabled())
    {
        result.pushKV("psbt", token.getPsbt());
    }
    else
    {
        result.pushKV("txid", token.getTxId());
    }
    return result;
},
    };
}

Span<const CRPCCommand> GetContractRPCCommands()
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              actor (function)
  //  ------------------    ------------------------
    { "wallet",             &createcontract,                 },
    { "wallet",             &sendtocontract,                 },
    { "wallet",             &removedelegationforaddress,     },
    { "wallet",             &setdelegateforaddress,          },
    { "wallet",             &qrc20approve,                    },
    { "wallet",             &qrc20transfer,                   },
    { "wallet",             &qrc20transferfrom,               },
    { "wallet",             &qrc20burn,                       },
    { "wallet",             &qrc20burnfrom,                   },
};
// clang-format on
    return commands;
}

} // namespace wallet
