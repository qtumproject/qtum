#include <qtum/qtumnft.h>
#include <validation.h>
#include <util/moneystr.h>
#include <util/contractabi.h>
#include <key_io.h>
#include <util/strencodings.h>
#include <util/convert.h>
#include <libethcore/ABI.h>
#include <qtum/nftconfig.h>
#include <logging.h>

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

bool QtumNftExec::execEvents(const int64_t &, const int64_t &, const int64_t&, const std::string &, const std::string &, const int &, const FunctionABI&, std::vector<NftEvent> &)
{
    return false;
}

bool QtumNftExec::privateKeysDisabled()
{
    return false;
}

bool QtumNftExec::isEventMine(const std::string &, const std::string &)
{
    return true;
}

bool QtumNftExec::filterMatch(const NftEvent &)
{
    return true;
}

bool QtumNftExec::parseEvent(const FunctionABI &func, const std::vector<std::string> &topics, const std::string &data, NftEvent &nftEvent, std::vector<uint256> &evtIds, std::vector<int32_t>& evtValues)
{
    try
    {
        std::vector<std::vector<std::string>> values;
        std::vector<ParameterABI::ErrorType> errors;
        if(func.abiOut(topics, data, values, errors) && values.size() >= 5)
        {
            if(values[1].size() > 0)
            {
                nftEvent.sender = values[1][0];
                if(!QtumNft::ToQtumAddress(nftEvent.sender, nftEvent.sender, false))
                    return false;
            }
            if(values[2].size() > 0)
            {
                nftEvent.receiver = values[2][0];
                if(!QtumNft::ToQtumAddress(nftEvent.receiver, nftEvent.receiver, false))
                    return false;
            }
            if(!isEventMine(nftEvent.sender, nftEvent.receiver))
                return true;

            if(values[3].size() != 0 && values[3].size() != values[4].size())
                return false;

            for(size_t i = 0; i < values[3].size(); i++)
            {
                uint256 id = u256Touint(dev::u256(values[3][i]));
                int32_t value = 0;
                if(!ParseInt32(values[4][i], &value))
                    return false;
                evtIds.push_back(id);
                evtValues.push_back(value);
            }
        }
    }
    catch(...)
    {
        return false;
    }

    return true;
}

bool QtumNftExec::addEvent(const FunctionABI &func, const std::vector<std::string> &topics, const std::string &data, NftEvent nftEvent, std::vector<NftEvent> &result)
{
    std::vector<uint256> evtIds;
    std::vector<int32_t> evtValues;

    bool ret = parseEvent(func, topics, data, nftEvent, evtIds, evtValues);
    if(ret)
    {
        for(size_t i = 0; i < evtIds.size(); i++)
        {
            nftEvent.id = evtIds[i];
            nftEvent.value = evtValues[i];
            if(filterMatch(nftEvent))
            {
                result.push_back(nftEvent);
            }
        }
    }
    else
    {
        LogPrintf("%s: Failed to parse NFT event. Transaction hash %s\n", __func__, nftEvent.transactionHash.ToString());
    }

    return ret;
}

QtumNftExec::~QtumNftExec()
{}

struct QtumNftData
{
    std::map<std::string, std::string> lstParams;
    std::map<std::string, bool> addressCache;
    QtumNftExec* nftExec;
    ContractABI* ABI;
    int funcBalanceOf;
    int funcCreateNFT;
    int funcIsApprovedForAll;
    int funcSafeTransferFrom;
    int funcSafeBatchTransferFrom;
    int funcWalletNFTList;
    int funcSupportsInterface;
    int evtTransferSingle;
    int evtTransferBatch;

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
        funcSafeBatchTransferFrom(-1),
        funcWalletNFTList(-1),
        evtTransferSingle(-1),
        evtTransferBatch(-1)
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

bool QtumNft::ToQtumAddress(const std::string& strHash160, std::string& strQtumAddress, bool isNullValid)
{
    uint160 key(ParseHex(strHash160.c_str()));
    if(!isNullValid && key.IsNull())
    {
        strQtumAddress = "";
        return true;
    }

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

int32_t QtumNft::ToInt32(const std::string &data)
{
    dev::bytes rawData = dev::fromHex(data);
    dev::bytesConstRef o(&rawData);
    dev::u256 outData = dev::eth::ABIDeserialiser<dev::u256>::deserialise(o);
    return (int32_t)outData;
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
            else if(func.name == "safeBatchTransferFrom")
            {
                d->funcSafeBatchTransferFrom = i;
            }
            else if(func.name == "walletNFTList")
            {
                d->funcWalletNFTList = i;
            }
            else if(func.name == "supportsInterface")
            {
                d->funcSupportsInterface = i;
            }
            else if(func.name == "TransferSingle")
            {
                d->evtTransferSingle = i;
            }
            else if(func.name == "TransferBatch")
            {
                d->evtTransferBatch = i;
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
    if(!NftConfig::Instance().GetNftAddress().IsNull())
    {
        d->lstParams[QtumNft_NS::PARAM_ADDRESS] = NftConfig::Instance().GetNftAddress().GetReverseHex();
    }
    else
    {
        d->lstParams[QtumNft_NS::PARAM_ADDRESS] = "";
    }
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

bool QtumNft::balanceOf(const std::string &_account, const uint256& _id, int32_t &result, bool sendTo)
{
    if(!supportsInterface())
        return false;

    std::string account = _account;
    if(!ToHash160(account, account))
    {
        return false;
    }
    std::string id = uintTou256(_id).str();

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
        else if(!ParseInt32(output[0], &result))
            return false;
    }

    return true;
}

bool QtumNft::balanceOf(const uint256& id, int32_t &result, bool sendTo)
{
    std::string account = d->lstParams[QtumNft_NS::PARAM_SENDER];
    return balanceOf(account, id, result, sendTo);
}

bool QtumNft::createNFT(const std::string &_owner, const std::string &name, const std::string &url, const std::string &desc, const int32_t &_count, bool sendTo)
{
    if(!supportsInterface())
        return false;

    std::string owner = _owner;
    if(!ToHash160(owner, owner))
    {
        return false;
    }
    std::string count = i64tostr(_count);

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

bool QtumNft::createNFT(const std::string &name, const std::string &url, const std::string &desc, const int32_t &count, bool sendTo)
{
    std::string owner = d->lstParams[QtumNft_NS::PARAM_SENDER];
    return createNFT(owner, name, url, desc, count, sendTo);
}

bool QtumNft::isApprovedForAll(const std::string &_account, const std::string &_operant, bool& success, bool sendTo)
{
    if(!supportsInterface())
        return false;

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

bool QtumNft::safeTransferFrom(const std::string &_from, const std::string &_to, const uint256 &_id, const int32_t &_amount, bool sendTo)
{
    if(!supportsInterface())
        return false;

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

    std::string id = uintTou256(_id).str();
    std::string amount = i64tostr(_amount);

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

bool QtumNft::safeTransfer(const std::string &to, const uint256 &id, const int32_t &amount, bool sendTo)
{
    std::string from = d->lstParams[QtumNft_NS::PARAM_SENDER];
    return safeTransferFrom(from, to, id, amount, sendTo);
}

bool QtumNft::safeBatchTransferFrom(const std::string &_from, const std::string &_to, const std::vector<uint256> &ids, const std::vector<int32_t> &amounts, bool sendTo)
{
    if(!supportsInterface())
        return false;

    // Check ids and amounts size
    if(ids.size() != amounts.size())
    {
        return false;
    }

    // Check for duplicates
    for(size_t i = 0; i < ids.size(); i++)
    {
        for(size_t j = i + 1; j < ids.size(); j++)
        {
            if(ids[i] == ids[j])
            {
                // Has duplicates
                return false;
            }
        }
    }

    // Parse from param
    std::string from = _from;
    if(!ToHash160(from, from))
    {
        return false;
    }
    std::vector<std::vector<std::string>> values;
    std::vector<std::string> paramFrom;
    paramFrom.push_back(from);
    values.push_back(paramFrom);

    // Parse to param
    std::string to = _to;
    if(!ToHash160(to, to))
    {
        return false;
    }
    std::vector<std::string> paramTo;
    paramTo.push_back(to);
    values.push_back(paramTo);

    // Parse ids param
    std::vector<std::string> paramIds;
    for(size_t i = 0; i < ids.size(); i++)
    {
        std::string id = uintTou256(ids[i]).str();
        paramIds.push_back(id);
    }
    values.push_back(paramIds);

    // Parse amounts param
    std::vector<std::string> paramAmounts;
    for(size_t i = 0; i < amounts.size(); i++)
    {
        std::string amount = i64tostr(amounts[i]);
        paramAmounts.push_back(amount);
    }
    values.push_back(paramAmounts);

    // Parse data param
    std::vector<std::string> paramData;
    paramData.push_back("");
    values.push_back(paramData);

    std::vector<std::string> output;
    if(!exec(values, d->funcSafeBatchTransferFrom, output, sendTo))
        return false;

    return output.size() == 0;
}

bool QtumNft::safeBatchTransfer(const std::string &to, const std::vector<uint256> &ids, const std::vector<int32_t> &amounts, bool sendTo)
{
    std::string from = d->lstParams[QtumNft_NS::PARAM_SENDER];
    return safeBatchTransferFrom(from, to, ids, amounts, sendTo);
}

bool QtumNft::walletNFTList(WalletNFTInfo &result, const uint256 &_id, bool sendTo)
{
    if(!supportsInterface())
        return false;

    std::string id = uintTou256(_id).str();

    std::vector<std::string> input;
    input.push_back(id);
    std::vector<std::string> output;

    if(!exec(input, d->funcWalletNFTList, output, sendTo))
        return false;

    if(!sendTo)
    {
        if(output.size() < 6)
            return false;
        else
        {
            dev::u256 NFTId(output[0]);
            result.NFTId = u256Touint(NFTId);
            result.name = output[1];
            result.url = output[2];
            result.desc = output[3];
            dev::u256 createAt(output[4]);
            result.createAt = (int64_t)createAt;
            dev::u256 count(output[5]);
            result.count = (int32_t)count;
        }
    }

    return true;
}

bool QtumNft::supportsInterface(std::string interfaceId)
{
    // Check the cached result
    std::string address = getAddress();
    if(!address.empty() && d->addressCache.find(address) != d->addressCache.end())
        return true;

    // Check if the NFT interface is supported
    std::vector<std::string> input;
    input.push_back(interfaceId);
    std::vector<std::string> output;

    if(!exec(input, d->funcSupportsInterface, output, false))
        return false;

    if(output.size() == 0)
        return false;

    bool ret = output[0] == "true";
    if(ret)
    {
        // Cache the result
        d->addressCache[address] = true;
    }
    else if(!ret && d->errorMessage.empty())
    {
        d->errorMessage = "Not a NFT contract address";
    }
    return ret;
}

bool QtumNft::transferEvents(std::vector<NftEvent> &nftEvents, int64_t fromBlock, int64_t toBlock, int64_t minconf)
{
    if(!supportsInterface())
        return false;

    bool ret = execEvents(fromBlock, toBlock, minconf, d->evtTransferSingle, nftEvents);
    ret &= execEvents(fromBlock, toBlock, minconf, d->evtTransferBatch, nftEvents);
    return ret;
}

bool QtumNft::exec(const std::vector<std::string> &input, int func, std::vector<std::string> &output, bool sendTo)
{
    std::vector<std::vector<std::string>> values;
    for(size_t i = 0; i < input.size(); i++)
    {
        std::vector<std::string> param;
        param.push_back(input[i]);
        values.push_back(param);
    }
    return exec(values, func, output, sendTo);
}

bool QtumNft::exec(const std::vector<std::vector<std::string>> &values, int func, std::vector<std::string> &output, bool sendTo)
{
    // Convert the input data into hex encoded binary data
    d->txid = "";
    d->psbt = "";
    if(d->nftExec == 0 || !(d->nftExec->execValid(func, sendTo)))
        return false;
    std::string strData;
    FunctionABI function = d->ABI->functions[func];
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
        if(nftTx.transactionHash != nftEvent.transactionHash) continue;
        if(nftTx.address != nftEvent.address) continue;
        if(nftTx.sender != nftEvent.sender) continue;
        if(nftTx.receiver != nftEvent.receiver) continue;
        if(nftTx.id != nftEvent.id) continue;
        if(nftTx.blockHash != nftEvent.blockHash) continue;
        if(nftTx.blockNumber != nftEvent.blockNumber) continue;

        // Update the value
        int32_t nftValue = nftTx.value + nftEvent.value;
        nftTx.value = nftValue;
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
    int numTopics = function.numIndexed() + 1;
    d->errorMessage.clear();
    if(!(d->nftExec->execEvents(fromBlock, toBlock, minconf, eventName, contractAddress, numTopics, function, result)))
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

std::string QtumNft::getAddress()
{
    return d->lstParams[QtumNft_NS::PARAM_ADDRESS];
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

