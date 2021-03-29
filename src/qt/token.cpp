#include <qt/token.h>
#include <qt/execrpccommand.h>
#include <qt/contractutil.h>
#include <validation.h>
#include <util/moneystr.h>
#include <key_io.h>
#include <util/strencodings.h>
#include <util/convert.h>
#include <qt/eventlog.h>
#include <libethcore/ABI.h>
#include <qt/walletmodel.h>

namespace Token_NS
{
static const char *TOKEN_ABI = "[{\"constant\":true,\"inputs\":[],\"name\":\"name\",\"outputs\":[{\"name\":\"\",\"type\":\"string\"}],\"payable\":false,\"stateMutability\":\"view\",\"type\":\"function\"},{\"constant\":false,\"inputs\":[{\"name\":\"_spender\",\"type\":\"address\"},{\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"approve\",\"outputs\":[{\"name\":\"success\",\"type\":\"bool\"}],\"payable\":false,\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"constant\":true,\"inputs\":[],\"name\":\"totalSupply\",\"outputs\":[{\"name\":\"\",\"type\":\"uint256\"}],\"payable\":false,\"stateMutability\":\"view\",\"type\":\"function\"},{\"constant\":false,\"inputs\":[{\"name\":\"_from\",\"type\":\"address\"},{\"name\":\"_to\",\"type\":\"address\"},{\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"transferFrom\",\"outputs\":[{\"name\":\"success\",\"type\":\"bool\"}],\"payable\":false,\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"constant\":true,\"inputs\":[],\"name\":\"decimals\",\"outputs\":[{\"name\":\"\",\"type\":\"uint8\"}],\"payable\":false,\"stateMutability\":\"view\",\"type\":\"function\"},{\"constant\":false,\"inputs\":[{\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"burn\",\"outputs\":[{\"name\":\"success\",\"type\":\"bool\"}],\"payable\":false,\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"constant\":true,\"inputs\":[{\"name\":\"\",\"type\":\"address\"}],\"name\":\"balanceOf\",\"outputs\":[{\"name\":\"\",\"type\":\"uint256\"}],\"payable\":false,\"stateMutability\":\"view\",\"type\":\"function\"},{\"constant\":false,\"inputs\":[{\"name\":\"_from\",\"type\":\"address\"},{\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"burnFrom\",\"outputs\":[{\"name\":\"success\",\"type\":\"bool\"}],\"payable\":false,\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"constant\":true,\"inputs\":[],\"name\":\"symbol\",\"outputs\":[{\"name\":\"\",\"type\":\"string\"}],\"payable\":false,\"stateMutability\":\"view\",\"type\":\"function\"},{\"constant\":false,\"inputs\":[{\"name\":\"_to\",\"type\":\"address\"},{\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"transfer\",\"outputs\":[],\"payable\":false,\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"constant\":false,\"inputs\":[{\"name\":\"_spender\",\"type\":\"address\"},{\"name\":\"_value\",\"type\":\"uint256\"},{\"name\":\"_extraData\",\"type\":\"bytes\"}],\"name\":\"approveAndCall\",\"outputs\":[{\"name\":\"success\",\"type\":\"bool\"}],\"payable\":false,\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"constant\":true,\"inputs\":[{\"name\":\"\",\"type\":\"address\"},{\"name\":\"\",\"type\":\"address\"}],\"name\":\"allowance\",\"outputs\":[{\"name\":\"\",\"type\":\"uint256\"}],\"payable\":false,\"stateMutability\":\"view\",\"type\":\"function\"},{\"inputs\":[{\"name\":\"initialSupply\",\"type\":\"uint256\"},{\"name\":\"tokenName\",\"type\":\"string\"},{\"name\":\"decimalUnits\",\"type\":\"uint8\"},{\"name\":\"tokenSymbol\",\"type\":\"string\"}],\"payable\":false,\"stateMutability\":\"nonpayable\",\"type\":\"constructor\"},{\"anonymous\":false,\"inputs\":[{\"indexed\":true,\"name\":\"from\",\"type\":\"address\"},{\"indexed\":true,\"name\":\"to\",\"type\":\"address\"},{\"indexed\":false,\"name\":\"value\",\"type\":\"uint256\"}],\"name\":\"Transfer\",\"type\":\"event\"},{\"anonymous\":false,\"inputs\":[{\"indexed\":true,\"name\":\"from\",\"type\":\"address\"},{\"indexed\":false,\"name\":\"value\",\"type\":\"uint256\"}],\"name\":\"Burn\",\"type\":\"event\"}]";

static const char *PRC_CALL = "callcontract";
static const char *PRC_SENDTO = "sendtocontract";
static const char *PARAM_ADDRESS = "address";
static const char *PARAM_DATAHEX = "datahex";
static const char *PARAM_AMOUNT = "amount";
static const char *PARAM_GASLIMIT = "gaslimit";
static const char *PARAM_GASPRICE = "gasprice";
static const char *PARAM_SENDER = "sender";
static const char *PARAM_BROADCAST = "broadcast";
static const char *PARAM_CHANGE_TO_SENDER = "changeToSender";

}
using namespace Token_NS;

struct TokenExecData
{
    ExecRPCCommand* call;
    ExecRPCCommand* send;
    EventLog* eventLog;
    WalletModel* model;
    QString errorMessage;

    TokenExecData():
        call(0),
        send(0),
        eventLog(0),
        model(0)
    {}
};

TokenExec::TokenExec()
{
    d = new TokenExecData();

    // Create new call command line interface
    QStringList lstMandatory;
    lstMandatory.append(PARAM_ADDRESS);
    lstMandatory.append(PARAM_DATAHEX);
    QStringList lstOptional;
    lstOptional.append(PARAM_SENDER);
    d->call = new ExecRPCCommand(PRC_CALL, lstMandatory, lstOptional, QMap<QString, QString>());

    // Create new send command line interface
    lstMandatory.clear();
    lstMandatory.append(PARAM_ADDRESS);
    lstMandatory.append(PARAM_DATAHEX);
    lstOptional.clear();
    lstOptional.append(PARAM_AMOUNT);
    lstOptional.append(PARAM_GASLIMIT);
    lstOptional.append(PARAM_GASPRICE);
    lstOptional.append(PARAM_SENDER);
    lstOptional.append(PARAM_BROADCAST);
    lstOptional.append(PARAM_CHANGE_TO_SENDER);
    d->send = new ExecRPCCommand(PRC_SENDTO, lstMandatory, lstOptional, QMap<QString, QString>());

    // Create new event log interface
    d->eventLog = new EventLog();
}

TokenExec::~TokenExec()
{
    if(d->call)
        delete d->call;
    d->call = 0;

    if(d->send)
        delete d->send;
    d->send = 0;

    if(d->eventLog)
        delete d->eventLog;
    d->eventLog = 0;

    if(d)
        delete d;
    d = 0;
}

void TokenExec::setModel(WalletModel *model)
{
    d->model = model;
}

bool TokenExec::execValid(const int &func, const bool &sendTo)
{
    ExecRPCCommand* cmd = sendTo ? d->send : d->call;
    if(func == -1 || d->model == 0 || cmd == 0)
        return false;
    return true;
}

bool TokenExec::execEventsValid(const int &func, const int64_t &fromBlock)
{
    if(func == -1 || fromBlock < 0 || d->model == 0)
        return false;
    return true;
}

bool TokenExec::exec(const bool &sendTo, const std::map<std::string, std::string> &lstParams, std::string &result, std::string &message)
{
    ExecRPCCommand* cmd = sendTo ? d->send : d->call;
    QVariant resultVar;
    QString resultJson;
    QString errorMessage;
    if(!cmd->exec(d->model->node(), d->model, ContractUtil::fromStdMap(lstParams), resultVar, resultJson, errorMessage))
    {
        message = errorMessage.toStdString();
        return false;
    }

    if(!sendTo)
    {
        QVariantMap variantMap = resultVar.toMap();
        QVariantMap executionResultMap = variantMap.value("executionResult").toMap();
        result = executionResultMap.value("output").toString().toStdString();
    }
    else
    {
        QVariantMap variantMap = resultVar.toMap();
        result = variantMap.value("txid").toString().toStdString();
    }

    return true;
}

bool TokenExec::execEvents(const int64_t &fromBlock, const int64_t &toBlock, const std::string &eventName, const std::string &contractAddress, const std::string &senderAddress, std::vector<TokenEvent> &result)
{
    QVariant resultVar;
    if(!(d->eventLog->searchTokenTx(d->model->node(), d->model, fromBlock, toBlock, contractAddress, senderAddress, resultVar)))
        return false;

    QList<QVariant> list = resultVar.toList();
    for(int i = 0; i < list.size(); i++)
    {
        // Search the log for events
        QVariantMap variantMap = list[i].toMap();
        QList<QVariant> listLog = variantMap.value("log").toList();
        for(int i = 0; i < listLog.size(); i++)
        {
            // Skip the not needed events
            QVariantMap variantLog = listLog[i].toMap();
            QList<QVariant> topicsList = variantLog.value("topics").toList();
            if(topicsList.count() < 3) continue;
            if(topicsList[0].toString().toStdString() != eventName) continue;

            // Create new event
            TokenEvent tokenEvent;
            tokenEvent.address = variantMap.value("contractAddress").toString().toStdString();
            tokenEvent.sender = topicsList[1].toString().toStdString().substr(24);
            Token::ToQtumAddress(tokenEvent.sender, tokenEvent.sender);
            tokenEvent.receiver = topicsList[2].toString().toStdString().substr(24);
            Token::ToQtumAddress(tokenEvent.receiver, tokenEvent.receiver);
            tokenEvent.blockHash = uint256S(variantMap.value("blockHash").toString().toStdString());
            tokenEvent.blockNumber = variantMap.value("blockNumber").toLongLong();
            tokenEvent.transactionHash = uint256S(variantMap.value("transactionHash").toString().toStdString());

            // Parse data
            std::string data = variantLog.value("data").toString().toStdString();
            dev::bytes rawData = dev::fromHex(data);
            dev::bytesConstRef o(&rawData);
            dev::u256 outData = dev::eth::ABIDeserialiser<dev::u256>::deserialise(o);
            tokenEvent.value = u256Touint(outData);

            result.push_back(tokenEvent);
        }
    }

    return true;
}


struct TokenData
{
    std::map<std::string, std::string> lstParams;
    std::string address;
    TokenExec* tokenExec;
    ContractABI* ABI;
    int funcName;
    int funcApprove;
    int funcTotalSupply;
    int funcTransferFrom;
    int funcDecimals;
    int funcBurn;
    int funcBalanceOf;
    int funcBurnFrom;
    int funcSymbol;
    int funcTransfer;
    int funcApproveAndCall;
    int funcAllowance;
    int evtTransfer;
    int evtBurn;

    std::string txid;
    std::string errorMessage;

    TokenData():
        tokenExec(0),
        ABI(0),
        funcName(-1),
        funcApprove(-1),
        funcTotalSupply(-1),
        funcTransferFrom(-1),
        funcDecimals(-1),
        funcBurn(-1),
        funcBalanceOf(-1),
        funcBurnFrom(-1),
        funcSymbol(-1),
        funcTransfer(-1),
        funcApproveAndCall(-1),
        funcAllowance(-1),
        evtTransfer(-1),
        evtBurn(-1)
    {}
};

bool ToHash160(const std::string& strQtumAddress, std::string& strHash160)
{
    CTxDestination qtumAddress = DecodeDestination(strQtumAddress);
    if(!IsValidDestination(qtumAddress))
        return false;
    const PKHash * keyid = boost::get<PKHash>(&qtumAddress);
    if(keyid){
        strHash160 = HexStr(valtype(keyid->begin(),keyid->end()));
    }else{
        return false;
    }
    return true;
}

bool Token::ToQtumAddress(const std::string& strHash160, std::string& strQtumAddress)
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

Token::Token():
    d(0)
{
    d = new TokenData();
    clear();

    // Create new command line interface
    d->tokenExec = new TokenExec();

    // Compute functions indexes
    d->ABI = new ContractABI();
    if(d->ABI->loads(TOKEN_ABI))
    {
        for(size_t i = 0; i < d->ABI->functions.size(); i++)
        {
            FunctionABI func = d->ABI->functions[i];
            if(func.name == "name")
            {
                d->funcName = i;
            }
            else if(func.name == "approve")
            {
                d->funcApprove = i;
            }
            else if(func.name == "totalSupply")
            {
                d->funcTotalSupply = i;
            }
            else if(func.name == "transferFrom")
            {
                d->funcTransferFrom = i;
            }
            else if(func.name == "decimals")
            {
                d->funcDecimals = i;
            }
            else if(func.name == "burn")
            {
                d->funcBurn = i;
            }
            else if(func.name == "balanceOf")
            {
                d->funcBalanceOf = i;
            }
            else if(func.name == "burnFrom")
            {
                d->funcBurnFrom = i;
            }
            else if(func.name == "symbol")
            {
                d->funcSymbol = i;
            }
            else if(func.name == "transfer")
            {
                d->funcTransfer = i;
            }
            else if(func.name == "approveAndCall")
            {
                d->funcApproveAndCall = i;
            }
            else if(func.name == "allowance")
            {
                d->funcAllowance = i;
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

Token::~Token()
{
    if(d->tokenExec)
        delete d->tokenExec;
    d->tokenExec = 0;

    if(d)
        delete d;
    d = 0;
}

void Token::setAddress(const std::string &address)
{
    d->lstParams[PARAM_ADDRESS] = address;
}

void Token::setDataHex(const std::string &datahex)
{
    d->lstParams[PARAM_DATAHEX] = datahex;
}

void Token::setAmount(const std::string &amount)
{
    d->lstParams[PARAM_AMOUNT] = amount;
}

void Token::setGasLimit(const std::string &gaslimit)
{
    d->lstParams[PARAM_GASLIMIT] = gaslimit;
}

void Token::setGasPrice(const std::string &gasPrice)
{
    d->lstParams[PARAM_GASPRICE] = gasPrice;
}

void Token::setSender(const std::string &sender)
{
    d->lstParams[PARAM_SENDER] = sender;
}

void Token::clear()
{
    d->lstParams.clear();

    setAmount("0");
    setGasPrice(FormatMoney(DEFAULT_GAS_PRICE));
    setGasLimit(std::to_string(DEFAULT_GAS_LIMIT_OP_SEND));

    d->lstParams[PARAM_BROADCAST] = "true";
    d->lstParams[PARAM_CHANGE_TO_SENDER] = "true";
}

std::string Token::getTxId()
{
    return d->txid;
}

bool Token::name(std::string &result, bool sendTo)
{
    std::vector<std::string> input;
    std::vector<std::string> output;
    if(!exec(input, d->funcName, output, sendTo))
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

bool Token::approve(const std::string &_spender, const std::string &_value, bool &success, bool sendTo)
{
    std::vector<std::string> input;
    input.push_back(_spender);
    input.push_back(_value);
    std::vector<std::string> output;

    if(!exec(input, d->funcApprove, output, sendTo))
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

bool Token::totalSupply(std::string &result, bool sendTo)
{
    std::vector<std::string> input;
    std::vector<std::string> output;
    if(!exec(input, d->funcTotalSupply, output, sendTo))
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

bool Token::transferFrom(const std::string &_from, const std::string &_to, const std::string &_value, bool &success, bool sendTo)
{
    std::vector<std::string> input;
    input.push_back(_from);
    input.push_back(_to);
    input.push_back(_value);
    std::vector<std::string> output;

    if(!exec(input, d->funcTransferFrom, output, sendTo))
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

bool Token::decimals(std::string &result, bool sendTo)
{
    std::vector<std::string> input;
    std::vector<std::string> output;
    if(!exec(input, d->funcDecimals, output, sendTo))
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

bool Token::burn(const std::string &_value, bool &success, bool sendTo)
{
    std::vector<std::string> input;
    input.push_back(_value);
    std::vector<std::string> output;

    if(!exec(input, d->funcBurn, output, sendTo))
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

bool Token::balanceOf(std::string &result, bool sendTo)
{
    std::string spender = d->lstParams[PARAM_SENDER];
    if(!ToHash160(spender, spender))
    {
        return false;
    }

    return balanceOf(spender, result, sendTo);
}

bool Token::balanceOf(const std::string &spender, std::string &result, bool sendTo)
{
    std::vector<std::string> input;
    input.push_back(spender);
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

bool Token::burnFrom(const std::string &_from, const std::string &_value, bool &success, bool sendTo)
{
    std::vector<std::string> input;
    input.push_back(_from);
    input.push_back(_value);
    std::vector<std::string> output;

    if(!exec(input, d->funcBurnFrom, output, sendTo))
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

bool Token::symbol(std::string &result, bool sendTo)
{
    std::vector<std::string> input;
    std::vector<std::string> output;
    if(!exec(input, d->funcSymbol, output, sendTo))
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

bool Token::transfer(const std::string &_to, const std::string &_value, bool sendTo)
{
    std::string to = _to;
    if(!ToHash160(to, to))
    {
        return false;
    }

    std::vector<std::string> input;
    input.push_back(to);
    input.push_back(_value);
    std::vector<std::string> output;

    return exec(input, d->funcTransfer, output, sendTo);
}

bool Token::approveAndCall(const std::string &_spender, const std::string &_value, const std::string &_extraData, bool &success, bool sendTo)
{
    std::vector<std::string> input;
    input.push_back(_spender);
    input.push_back(_value);
    input.push_back(_extraData);
    std::vector<std::string> output;

    if(!exec(input, d->funcApproveAndCall, output, sendTo))
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

bool Token::allowance(const std::string &_from, const std::string &_to, std::string &result, bool sendTo)
{
    std::vector<std::string> input;
    input.push_back(_from);
    input.push_back(_to);
    std::vector<std::string> output;

    if(!exec(input, d->funcAllowance, output, sendTo))
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

bool Token::transferEvents(std::vector<TokenEvent> &tokenEvents, int64_t fromBlock, int64_t toBlock)
{
    return execEvents(fromBlock, toBlock, d->evtTransfer, tokenEvents);
}

bool Token::burnEvents(std::vector<TokenEvent> &tokenEvents, int64_t fromBlock, int64_t toBlock)
{
    return execEvents(fromBlock, toBlock, d->evtBurn, tokenEvents);
}

bool Token::exec(const std::vector<std::string> &input, int func, std::vector<std::string> &output, bool sendTo)
{
    // Convert the input data into hex encoded binary data
    d->txid = "";
    if(d->tokenExec == 0 || !(d->tokenExec->execValid(func, sendTo)))
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
    if(!(d->tokenExec->exec(sendTo, d->lstParams, result, d->errorMessage)))
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
        d->txid = result;
    }

    return true;
}

void addTokenEvent(std::vector<TokenEvent> &tokenEvents, TokenEvent tokenEvent)
{
    // Check if the event is from an existing token transaction and update the value
    bool found = false;
    for(size_t i = 0; i < tokenEvents.size(); i++)
    {
        // Compare the event data
        TokenEvent tokenTx = tokenEvents[i];
        if(tokenTx.address != tokenEvent.address) continue;
        if(tokenTx.sender != tokenEvent.sender) continue;
        if(tokenTx.receiver != tokenEvent.receiver) continue;
        if(tokenTx.blockHash != tokenEvent.blockHash) continue;
        if(tokenTx.blockNumber != tokenEvent.blockNumber) continue;
        if(tokenTx.transactionHash != tokenEvent.transactionHash) continue;

        // Update the value
        dev::u256 tokenValue = uintTou256(tokenTx.value) + uintTou256(tokenEvent.value);
        tokenTx.value = u256Touint(tokenValue);
        tokenEvents[i] = tokenTx;
        found = true;
        break;
    }

    // Add new event
    if(!found)
        tokenEvents.push_back(tokenEvent);
}

bool Token::execEvents(int64_t fromBlock, int64_t toBlock, int func, std::vector<TokenEvent> &tokenEvents)
{
    // Check parameters
    if(d->tokenExec == 0 || !(d->tokenExec->execEventsValid(func, fromBlock)))
        return false;

    //  Get function
    FunctionABI function = d->ABI->functions[func];

    // Search for events
    std::vector<TokenEvent> result;
    std::string eventName = function.selector();
    std::string contractAddress = d->lstParams[PARAM_ADDRESS];
    std::string senderAddress = d->lstParams[PARAM_SENDER];
    ToHash160(senderAddress, senderAddress);
    senderAddress  = "000000000000000000000000" + senderAddress;
    if(!(d->tokenExec->execEvents( fromBlock, toBlock, eventName, contractAddress, senderAddress, result)))
        return false;

    // Parse the result events
    for(const TokenEvent& tokenEvent : result)
    {
        addTokenEvent(tokenEvents, tokenEvent);
    }

    return true;
}

void Token::setModel(WalletModel *model)
{
    d->tokenExec->setModel(model);
}

std::string Token::getErrorMessage()
{
    return d->errorMessage;
}

void Token::setTokenExec(TokenExec *tokenExec)
{
    d->tokenExec = tokenExec;
}
