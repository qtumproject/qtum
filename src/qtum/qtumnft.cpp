#include <qtum/qtumnft.h>
#include <validation.h>
#include <util/moneystr.h>
#include <util/contractabi.h>
#include <key_io.h>
#include <util/strencodings.h>
#include <util/convert.h>
#include <libethcore/ABI.h>
#include <chainparams.h>

namespace QtumNft_NS
{
const char *NFT_ABI = "[{\"inputs\":[],\"stateMutability\":\"nonpayable\",\"type\":\"constructor\"},{\"anonymous\":false,\"inputs\":[{\"indexed\":true,\"internalType\":\"address\",\"name\":\"account\",\"type\":\"address\"},{\"indexed\":true,\"internalType\":\"address\",\"name\":\"operator\",\"type\":\"address\"},{\"indexed\":false,\"internalType\":\"bool\",\"name\":\"approved\",\"type\":\"bool\"}],\"name\":\"ApprovalForAll\",\"type\":\"event\"},{\"anonymous\":false,\"inputs\":[{\"indexed\":true,\"internalType\":\"address\",\"name\":\"operator\",\"type\":\"address\"},{\"indexed\":true,\"internalType\":\"address\",\"name\":\"from\",\"type\":\"address\"},{\"indexed\":true,\"internalType\":\"address\",\"name\":\"to\",\"type\":\"address\"},{\"indexed\":false,\"internalType\":\"uint256[]\",\"name\":\"ids\",\"type\":\"uint256[]\"},{\"indexed\":false,\"internalType\":\"uint256[]\",\"name\":\"values\",\"type\":\"uint256[]\"}],\"name\":\"TransferBatch\",\"type\":\"event\"},{\"anonymous\":false,\"inputs\":[{\"indexed\":true,\"internalType\":\"address\",\"name\":\"operator\",\"type\":\"address\"},{\"indexed\":true,\"internalType\":\"address\",\"name\":\"from\",\"type\":\"address\"},{\"indexed\":true,\"internalType\":\"address\",\"name\":\"to\",\"type\":\"address\"},{\"indexed\":false,\"internalType\":\"uint256\",\"name\":\"id\",\"type\":\"uint256\"},{\"indexed\":false,\"internalType\":\"uint256\",\"name\":\"value\",\"type\":\"uint256\"}],\"name\":\"TransferSingle\",\"type\":\"event\"},{\"anonymous\":false,\"inputs\":[{\"indexed\":false,\"internalType\":\"string\",\"name\":\"value\",\"type\":\"string\"},{\"indexed\":true,\"internalType\":\"uint256\",\"name\":\"id\",\"type\":\"uint256\"}],\"name\":\"URI\",\"type\":\"event\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"account\",\"type\":\"address\"},{\"internalType\":\"uint256\",\"name\":\"id\",\"type\":\"uint256\"}],\"name\":\"balanceOf\",\"outputs\":[{\"internalType\":\"uint256\",\"name\":\"\",\"type\":\"uint256\"}],\"stateMutability\":\"view\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address[]\",\"name\":\"accounts\",\"type\":\"address[]\"},{\"internalType\":\"uint256[]\",\"name\":\"ids\",\"type\":\"uint256[]\"}],\"name\":\"balanceOfBatch\",\"outputs\":[{\"internalType\":\"uint256[]\",\"name\":\"\",\"type\":\"uint256[]\"}],\"stateMutability\":\"view\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"owner\",\"type\":\"address\"},{\"internalType\":\"uint256[]\",\"name\":\"_ids\",\"type\":\"uint256[]\"}],\"name\":\"batchNFTInfoByIds\",\"outputs\":[{\"components\":[{\"internalType\":\"uint256\",\"name\":\"NFTId\",\"type\":\"uint256\"},{\"internalType\":\"string\",\"name\":\"name\",\"type\":\"string\"},{\"internalType\":\"string\",\"name\":\"url\",\"type\":\"string\"},{\"internalType\":\"string\",\"name\":\"desc\",\"type\":\"string\"},{\"internalType\":\"uint256\",\"name\":\"createAt\",\"type\":\"uint256\"},{\"internalType\":\"uint32\",\"name\":\"count\",\"type\":\"uint32\"}],\"internalType\":\"struct QtumNFT.WalletNFTInfo[]\",\"name\":\"\",\"type\":\"tuple[]\"}],\"stateMutability\":\"view\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"owner\",\"type\":\"address\"},{\"internalType\":\"string\",\"name\":\"name\",\"type\":\"string\"},{\"internalType\":\"string\",\"name\":\"url\",\"type\":\"string\"},{\"internalType\":\"string\",\"name\":\"desc\",\"type\":\"string\"},{\"internalType\":\"uint32\",\"name\":\"count\",\"type\":\"uint32\"}],\"name\":\"createNFT\",\"outputs\":[],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"owner\",\"type\":\"address\"},{\"internalType\":\"uint256\",\"name\":\"fromIndex\",\"type\":\"uint256\"},{\"internalType\":\"uint256\",\"name\":\"take\",\"type\":\"uint256\"}],\"name\":\"getNFTListByOwner\",\"outputs\":[{\"internalType\":\"uint256[]\",\"name\":\"\",\"type\":\"uint256[]\"},{\"internalType\":\"uint256\",\"name\":\"\",\"type\":\"uint256\"}],\"stateMutability\":\"view\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"account\",\"type\":\"address\"},{\"internalType\":\"address\",\"name\":\"operator\",\"type\":\"address\"}],\"name\":\"isApprovedForAll\",\"outputs\":[{\"internalType\":\"bool\",\"name\":\"\",\"type\":\"bool\"}],\"stateMutability\":\"view\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"from\",\"type\":\"address\"},{\"internalType\":\"address\",\"name\":\"to\",\"type\":\"address\"},{\"internalType\":\"uint256[]\",\"name\":\"ids\",\"type\":\"uint256[]\"},{\"internalType\":\"uint256[]\",\"name\":\"amounts\",\"type\":\"uint256[]\"},{\"internalType\":\"bytes\",\"name\":\"data\",\"type\":\"bytes\"}],\"name\":\"safeBatchTransferFrom\",\"outputs\":[],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"from\",\"type\":\"address\"},{\"internalType\":\"address\",\"name\":\"to\",\"type\":\"address\"},{\"internalType\":\"uint256\",\"name\":\"id\",\"type\":\"uint256\"},{\"internalType\":\"uint256\",\"name\":\"amount\",\"type\":\"uint256\"}],\"name\":\"safeTransferFrom\",\"outputs\":[],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"operator\",\"type\":\"address\"},{\"internalType\":\"bool\",\"name\":\"approved\",\"type\":\"bool\"}],\"name\":\"setApprovalForAll\",\"outputs\":[],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"bytes4\",\"name\":\"interfaceId\",\"type\":\"bytes4\"}],\"name\":\"supportsInterface\",\"outputs\":[{\"internalType\":\"bool\",\"name\":\"\",\"type\":\"bool\"}],\"stateMutability\":\"view\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"uint256\",\"name\":\"\",\"type\":\"uint256\"}],\"name\":\"uri\",\"outputs\":[{\"internalType\":\"string\",\"name\":\"\",\"type\":\"string\"}],\"stateMutability\":\"view\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"uint256\",\"name\":\"\",\"type\":\"uint256\"}],\"name\":\"walletNFTList\",\"outputs\":[{\"internalType\":\"uint256\",\"name\":\"NFTId\",\"type\":\"uint256\"},{\"internalType\":\"string\",\"name\":\"name\",\"type\":\"string\"},{\"internalType\":\"string\",\"name\":\"url\",\"type\":\"string\"},{\"internalType\":\"string\",\"name\":\"desc\",\"type\":\"string\"},{\"internalType\":\"uint256\",\"name\":\"createAt\",\"type\":\"uint256\"},{\"internalType\":\"uint32\",\"name\":\"count\",\"type\":\"uint32\"}],\"stateMutability\":\"view\",\"type\":\"function\"}]";
const char *PARAM_ADDRESS = "address";
const char *PARAM_DATAHEX = "datahex";
const char *PARAM_AMOUNT = "amount";
const char *PARAM_GASLIMIT = "gaslimit";
const char *PARAM_GASPRICE = "gasprice";
const char *PARAM_SENDER = "sender";
const char *PARAM_BROADCAST = "broadcast";
const char *PARAM_CHANGE_TO_SENDER = "changeToSender";
const char *PARAM_PSBT = "psbt";
}

bool QtumNftExec::execValid(const int &, const bool &)
{
    return false;
}

bool QtumNftExec::execEventsValid(const int &, const int64_t &)
{
    return false;
}

bool QtumNftExec::exec(const bool &, const std::map<std::string, std::string> &, std::string &, std::string &)
{
    return false;
}

bool QtumNftExec::execEvents(const int64_t &, const int64_t &, const int64_t&, const std::string &, const std::string &, const std::string &, const int &, std::vector<NftEvent> &)
{
    return false;
}

bool QtumNftExec::privateKeysDisabled()
{
    return false;
}

QtumNftExec::~QtumNftExec()
{}

struct QtumNftData
{
    std::map<std::string, std::string> lstParams;
    std::string address;
    QtumNftExec* nftExec;
    ContractABI* ABI;
    int funcBalanceOf;
    int funcCreateNFT;
    int funcIsApprovedForAll;
    int funcSafeTransferFrom;
    int evtTransfer;
    int evtBurn;

    std::string txid;
    std::string psbt;
    std::string errorMessage;

    QtumNftData():
        nftExec(0),
        ABI(0),
        funcBalanceOf(-1),
        funcCreateNFT(-1),
        funcIsApprovedForAll(-1),
        funcSafeTransferFrom(-1),
        evtTransfer(-1),
        evtBurn(-1)
    {}
};

bool QtumNft::ToHash160(const std::string& strQtumAddress, std::string& strHash160)
{
    CTxDestination qtumAddress = DecodeDestination(strQtumAddress);
    if(!IsValidDestination(qtumAddress))
        return false;
    if(std::holds_alternative<PKHash>(qtumAddress)){
        PKHash keyid = std::get<PKHash>(qtumAddress);
        strHash160 = HexStr(valtype(keyid.begin(),keyid.end()));
    }else{
        return false;
    }
    return true;
}

bool QtumNft::ToQtumAddress(const std::string& strHash160, std::string& strQtumAddress)
{
    uint160 key(ParseHex(strHash160.c_str()));
    PKHash keyid(key);
    CTxDestination qtumAddress = keyid;
    if(IsValidDestination(qtumAddress)){
        strQtumAddress = EncodeDestination(qtumAddress);
        return true;
    }
    return false;
}

uint256 QtumNft::ToUint256(const std::string &data)
{
    dev::bytes rawData = dev::fromHex(data);
    dev::bytesConstRef o(&rawData);
    dev::u256 outData = dev::eth::ABIDeserialiser<dev::u256>::deserialise(o);
    return u256Touint(outData);
}

QtumNft::QtumNft():
    d(0)
{
    d = new QtumNftData();
    clear();

    // Compute functions indexes
    d->ABI = new ContractABI();
    if(d->ABI->loads(QtumNft_NS::NFT_ABI))
    {
        for(size_t i = 0; i < d->ABI->functions.size(); i++)
        {
            FunctionABI func = d->ABI->functions[i];
            if(func.name == "balanceOf")
            {
                d->funcBalanceOf = i;
            }
            else if(func.name == "createNFT")
            {
                d->funcCreateNFT = i;
            }
            else if(func.name == "isApprovedForAll")
            {
                d->funcIsApprovedForAll = i;
            }
            else if(func.name == "safeTransferFrom")
            {
                d->funcSafeTransferFrom = i;
            }
            else if(func.name == "Transfer")
            {
                d->evtTransfer = i;
            }
            else if(func.name == "Burn")
            {
                d->evtBurn = i;
            }
        }
    }
}

QtumNft::~QtumNft()
{
    d->nftExec = 0;

    if(d)
        delete d;
    d = 0;
}

void QtumNft::setAddress(const std::string &address)
{
    d->lstParams[QtumNft_NS::PARAM_ADDRESS] = address;
}

void QtumNft::setDataHex(const std::string &datahex)
{
    d->lstParams[QtumNft_NS::PARAM_DATAHEX] = datahex;
}

void QtumNft::setAmount(const std::string &amount)
{
    d->lstParams[QtumNft_NS::PARAM_AMOUNT] = amount;
}

void QtumNft::setGasLimit(const std::string &gaslimit)
{
    d->lstParams[QtumNft_NS::PARAM_GASLIMIT] = gaslimit;
}

void QtumNft::setGasPrice(const std::string &gasPrice)
{
    d->lstParams[QtumNft_NS::PARAM_GASPRICE] = gasPrice;
}

void QtumNft::setSender(const std::string &sender)
{
    d->lstParams[QtumNft_NS::PARAM_SENDER] = sender;
}

void QtumNft::clear()
{
    d->lstParams.clear();

    setAmount("0");
    setGasPrice(FormatMoney(DEFAULT_GAS_PRICE));
    setGasLimit(std::to_string(DEFAULT_GAS_LIMIT_OP_SEND));

    d->lstParams[QtumNft_NS::PARAM_BROADCAST] = "true";
    d->lstParams[QtumNft_NS::PARAM_CHANGE_TO_SENDER] = "true";
    d->lstParams[QtumNft_NS::PARAM_ADDRESS] = Params().GetConsensus().nftAddress.GetHex();
}

std::string QtumNft::getTxId()
{
    return d->txid;
}

std::string QtumNft::getPsbt()
{
    return d->psbt;
}

void QtumNft::setTxId(const std::string& txid)
{
    d->txid = txid;
}

bool QtumNft::balanceOf(const std::string &_account, const std::string& id, std::string &result, bool sendTo)
{
    std::string account = _account;
    if(!ToHash160(account, account))
    {
        return false;
    }

    std::vector<std::string> input;
    input.push_back(account);
    input.push_back(id);
    std::vector<std::string> output;

    if(!exec(input, d->funcBalanceOf, output, sendTo))
        return false;

    if(!sendTo)
    {
        if(output.size() == 0)
            return false;
        else
            result = output[0];
    }

    return true;
}

bool QtumNft::balanceOf(std::string &result, const std::string& id, bool sendTo)
{
    std::string account = d->lstParams[QtumNft_NS::PARAM_SENDER];
    return balanceOf(account, id, result, sendTo);
}

bool QtumNft::createNFT(const std::string &_owner, const std::string &name, const std::string &url, const std::string &desc, const std::string &count, bool sendTo)
{
    std::string owner = _owner;
    if(!ToHash160(owner, owner))
    {
        return false;
    }

    std::vector<std::string> input;
    input.push_back(owner);
    input.push_back(name);
    input.push_back(url);
    input.push_back(desc);
    input.push_back(count);
    std::vector<std::string> output;

    if(!exec(input, d->funcCreateNFT, output, sendTo))
        return false;

    return output.size() == 0;
}

bool QtumNft::createNFT(const std::string &name, const std::string &url, const std::string &desc, const std::string &count, bool sendTo)
{
    std::string owner = d->lstParams[QtumNft_NS::PARAM_SENDER];
    return createNFT(owner, name, url, desc, count, sendTo);
}

bool QtumNft::isApprovedForAll(const std::string &_account, const std::string &_operant, bool& success, bool sendTo)
{
    std::string account = _account;
    if(!ToHash160(account, account))
    {
        return false;
    }

    std::string operant = _operant;
    if(!ToHash160(operant, operant))
    {
        return false;
    }

    std::vector<std::string> input;
    input.push_back(account);
    input.push_back(operant);
    std::vector<std::string> output;

    if(!exec(input, d->funcIsApprovedForAll, output, sendTo))
        return false;

    if(!sendTo)
    {
        if(output.size() == 0)
            return false;
        else
            success = output[0] == "true";
    }

    return true;
}

bool QtumNft::safeTransferFrom(const std::string &_from, const std::string &_to, const std::string &id, const std::string &amount, bool sendTo)
{
    std::string from = _from;
    if(!ToHash160(from, from))
    {
        return false;
    }

    std::string to = _to;
    if(!ToHash160(to, to))
    {
        return false;
    }

    std::vector<std::string> input;
    input.push_back(from);
    input.push_back(to);
    input.push_back(id);
    input.push_back(amount);
    std::vector<std::string> output;

    if(!exec(input, d->funcSafeTransferFrom, output, sendTo))
        return false;

    return output.size() == 0;
}

bool QtumNft::safeTransfer(const std::string &to, const std::string &id, const std::string &amount, bool sendTo)
{
    std::string from = d->lstParams[QtumNft_NS::PARAM_SENDER];
    return safeTransferFrom(from, to, id, amount, sendTo);
}

bool QtumNft::transferEvents(std::vector<NftEvent> &nftEvents, int64_t fromBlock, int64_t toBlock, int64_t minconf)
{
    return execEvents(fromBlock, toBlock, minconf, d->evtTransfer, nftEvents);
}

bool QtumNft::burnEvents(std::vector<NftEvent> &nftEvents, int64_t fromBlock, int64_t toBlock, int64_t minconf)
{
    return execEvents(fromBlock, toBlock, minconf, d->evtBurn, nftEvents);
}

bool QtumNft::exec(const std::vector<std::string> &input, int func, std::vector<std::string> &output, bool sendTo)
{
    // Convert the input data into hex encoded binary data
    d->txid = "";
    d->psbt = "";
    if(d->nftExec == 0 || !(d->nftExec->execValid(func, sendTo)))
        return false;
    std::string strData;
    FunctionABI function = d->ABI->functions[func];
    std::vector<std::vector<std::string>> values;
    for(size_t i = 0; i < input.size(); i++)
    {
        std::vector<std::string> param;
        param.push_back(input[i]);
        values.push_back(param);
    }
    std::vector<ParameterABI::ErrorType> errors;
    if(!function.abiIn(values, strData, errors))
        return false;
    setDataHex(strData);

    // Execute the command and get the result
    std::string result;
    d->errorMessage.clear();    
    if(!(d->nftExec->exec(sendTo, d->lstParams, result, d->errorMessage)))
        return false;

    // Get the result from calling function
    if(!sendTo)
    {
        std::string rawData = result;
        std::vector<std::vector<std::string>> values;
        std::vector<ParameterABI::ErrorType> errors;
        if(!function.abiOut(rawData, values, errors))
            return false;
        for(size_t i = 0; i < values.size(); i++)
        {
            std::vector<std::string> param = values[i];
            output.push_back(param.size() ? param[0] : "");
        }
    }
    else
    {
        if(d->nftExec->privateKeysDisabled())
        {
            d->psbt = result;
        }
        else
        {
            d->txid = result;
        }
    }

    return true;
}

void QtumNft::addNftEvent(std::vector<NftEvent> &nftEvents, NftEvent nftEvent)
{
    // Check if the event is from an existing nft transaction and update the value
    bool found = false;
    for(size_t i = 0; i < nftEvents.size(); i++)
    {
        // Compare the event data
        NftEvent nftTx = nftEvents[i];
        if(nftTx.address != nftEvent.address) continue;
        if(nftTx.sender != nftEvent.sender) continue;
        if(nftTx.receiver != nftEvent.receiver) continue;
        if(nftTx.blockHash != nftEvent.blockHash) continue;
        if(nftTx.blockNumber != nftEvent.blockNumber) continue;
        if(nftTx.transactionHash != nftEvent.transactionHash) continue;

        // Update the value
        dev::u256 nftValue = uintTou256(nftTx.value) + uintTou256(nftEvent.value);
        nftTx.value = u256Touint(nftValue);
        nftEvents[i] = nftTx;
        found = true;
        break;
    }

    // Add new event
    if(!found)
        nftEvents.push_back(nftEvent);
}

bool QtumNft::execEvents(int64_t fromBlock, int64_t toBlock, int64_t minconf, int func, std::vector<NftEvent> &nftEvents)
{
    // Check parameters
    if(d->nftExec == 0 || !(d->nftExec->execEventsValid(func, fromBlock)))
        return false;

    //  Get function
    FunctionABI function = d->ABI->functions[func];

    // Search for events
    std::vector<NftEvent> result;
    std::string eventName = function.selector();
    std::string contractAddress = d->lstParams[QtumNft_NS::PARAM_ADDRESS];
    std::string senderAddress = d->lstParams[QtumNft_NS::PARAM_SENDER];
    ToHash160(senderAddress, senderAddress);
    senderAddress  = "000000000000000000000000" + senderAddress;
    int numTopics = function.numIndexed() + 1;
    if(!(d->nftExec->execEvents(fromBlock, toBlock, minconf, eventName, contractAddress, senderAddress, numTopics, result)))
        return false;

    // Parse the result events
    for(const NftEvent& nftEvent : result)
    {
        addNftEvent(nftEvents, nftEvent);
    }

    return true;
}

std::string QtumNft::getErrorMessage()
{
    return d->errorMessage;
}

void QtumNft::setQtumNftExec(QtumNftExec *nftExec)
{
    d->nftExec = nftExec;
}

const char* QtumNft::paramAddress()
{
    return QtumNft_NS::PARAM_ADDRESS;
}

const char* QtumNft::paramDatahex()
{
    return QtumNft_NS::PARAM_DATAHEX;
}

const char* QtumNft::paramAmount()
{
    return QtumNft_NS::PARAM_AMOUNT;
}

const char* QtumNft::paramGasLimit()
{
    return QtumNft_NS::PARAM_GASLIMIT;
}

const char* QtumNft::paramGasPrice()
{
    return QtumNft_NS::PARAM_GASPRICE;
}

const char* QtumNft::paramSender()
{
    return QtumNft_NS::PARAM_SENDER;
}

const char* QtumNft::paramBroadcast()
{
    return QtumNft_NS::PARAM_BROADCAST;
}

const char* QtumNft::paramChangeToSender()
{
    return QtumNft_NS::PARAM_CHANGE_TO_SENDER;
}

const char* QtumNft::paramPsbt()
{
    return QtumNft_NS::PARAM_PSBT;
}
