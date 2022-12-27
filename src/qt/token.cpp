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
const char *PRC_CALL = "callcontract";
const char *PRC_SENDTO = "sendtocontract";
}

struct TokenData
{
    ExecRPCCommand* call;
    ExecRPCCommand* send;
    EventLog* eventLog;
    WalletModel* model;
    QString errorMessage;

    TokenData():
        call(0),
        send(0),
        eventLog(0),
        model(0)
    {}
};

Token::Token()
{
    d = new TokenData();

    // Create new call command line interface
    QStringList lstMandatory;
    lstMandatory.append(QtumToken::paramAddress());
    lstMandatory.append(QtumToken::paramDatahex());
    QStringList lstOptional;
    lstOptional.append(QtumToken::paramSender());
    d->call = new ExecRPCCommand(Token_NS::PRC_CALL, lstMandatory, lstOptional, QMap<QString, QString>());

    // Create new send command line interface
    lstMandatory.clear();
    lstMandatory.append(QtumToken::paramAddress());
    lstMandatory.append(QtumToken::paramDatahex());
    lstOptional.clear();
    lstOptional.append(QtumToken::paramAmount());
    lstOptional.append(QtumToken::paramGasLimit());
    lstOptional.append(QtumToken::paramGasPrice());
    lstOptional.append(QtumToken::paramSender());
    lstOptional.append(QtumToken::paramBroadcast());
    lstOptional.append(QtumToken::paramChangeToSender());
    lstOptional.append(QtumToken::paramPsbt());
    d->send = new ExecRPCCommand(Token_NS::PRC_SENDTO, lstMandatory, lstOptional, QMap<QString, QString>());

    // Create new event log interface
    d->eventLog = new EventLog();

    setQtumTokenExec(this);
}

Token::~Token()
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

void Token::setModel(WalletModel *model)
{
    d->model = model;
}

bool Token::execValid(const int &func, const bool &sendTo)
{
    ExecRPCCommand* cmd = sendTo ? d->send : d->call;
    if(func == -1 || d->model == 0 || cmd == 0)
        return false;
    return true;
}

bool Token::execEventsValid(const int &func, const int64_t &fromBlock)
{
    if(func == -1 || fromBlock < 0 || d->model == 0)
        return false;
    return true;
}

bool Token::exec(const bool &sendTo, const std::map<std::string, std::string> &lstParams, std::string &result, std::string &message)
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
        if(d->model->wallet().privateKeysDisabled())
        {
            result = variantMap.value("psbt").toString().toStdString();
        }
        else
        {
            result = variantMap.value("txid").toString().toStdString();
        }
    }

    return true;
}

bool Token::execEvents(const int64_t &fromBlock, const int64_t &toBlock, const int64_t &minconf, const std::string &eventName, const std::string &contractAddress, const std::string &senderAddress, const int &numTopics, std::vector<TokenEvent> &result)
{
    QVariant resultVar;
    if(!(d->eventLog->searchTokenTx(d->model->node(), d->model, fromBlock, toBlock, minconf, eventName, contractAddress, senderAddress, numTopics, resultVar)))
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
            if(topicsList.count() < numTopics) continue;
            if(topicsList[0].toString().toStdString() != eventName) continue;

            // Create new event
            TokenEvent tokenEvent;
            tokenEvent.address = variantMap.value("contractAddress").toString().toStdString();
            if(numTopics > 1)
            {
                tokenEvent.sender = topicsList[1].toString().toStdString().substr(24);
                Token::ToQtumAddress(tokenEvent.sender, tokenEvent.sender);
            }
            if(numTopics > 2)
            {
                tokenEvent.receiver = topicsList[2].toString().toStdString().substr(24);
                Token::ToQtumAddress(tokenEvent.receiver, tokenEvent.receiver);
            }
            tokenEvent.blockHash = uint256S(variantMap.value("blockHash").toString().toStdString());
            tokenEvent.blockNumber = variantMap.value("blockNumber").toLongLong();
            tokenEvent.transactionHash = uint256S(variantMap.value("transactionHash").toString().toStdString());

            // Parse data
            std::string data = variantLog.value("data").toString().toStdString();
            tokenEvent.value = Token::ToUint256(data);

            result.push_back(tokenEvent);
        }
    }

    return true;
}

bool Token::privateKeysDisabled()
{
    if(!d || !d->model)
        return false;
    return d->model->wallet().privateKeysDisabled();
}
