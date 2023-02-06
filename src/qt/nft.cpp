#include <qt/nft.h>
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
#include <util/contractabi.h>
#include <vector>
#include <string>

namespace Nft_NS
{
const char *PRC_CALL = "callcontract";
const char *PRC_SENDTO = "sendtocontract";
}

struct NftData
{
    ExecRPCCommand* call;
    ExecRPCCommand* send;
    EventLog* eventLog;
    WalletModel* model;
    QString errorMessage;

    NftData():
        call(0),
        send(0),
        eventLog(0),
        model(0)
    {}
};

Nft::Nft()
{
    d = new NftData();

    // Create new call command line interface
    QStringList lstMandatory;
    lstMandatory.append(QtumNft::paramAddress());
    lstMandatory.append(QtumNft::paramDatahex());
    QStringList lstOptional;
    lstOptional.append(QtumNft::paramSender());
    d->call = new ExecRPCCommand(Nft_NS::PRC_CALL, lstMandatory, lstOptional, QMap<QString, QString>());

    // Create new send command line interface
    lstMandatory.clear();
    lstMandatory.append(QtumNft::paramAddress());
    lstMandatory.append(QtumNft::paramDatahex());
    lstOptional.clear();
    lstOptional.append(QtumNft::paramAmount());
    lstOptional.append(QtumNft::paramGasLimit());
    lstOptional.append(QtumNft::paramGasPrice());
    lstOptional.append(QtumNft::paramSender());
    lstOptional.append(QtumNft::paramBroadcast());
    lstOptional.append(QtumNft::paramChangeToSender());
    lstOptional.append(QtumNft::paramPsbt());
    d->send = new ExecRPCCommand(Nft_NS::PRC_SENDTO, lstMandatory, lstOptional, QMap<QString, QString>());

    // Create new event log interface
    d->eventLog = new EventLog();

    setQtumNftExec(this);
}

Nft::~Nft()
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

void Nft::setModel(WalletModel *model)
{
    d->model = model;
}

bool Nft::execValid(const int &func, const bool &sendTo)
{
    ExecRPCCommand* cmd = sendTo ? d->send : d->call;
    if(func == -1 || d->model == 0 || cmd == 0)
        return false;
    return true;
}

bool Nft::execEventsValid(const int &func, const int64_t &fromBlock)
{
    if(func == -1 || fromBlock < 0 || d->model == 0)
        return false;
    return true;
}

bool Nft::exec(const bool &sendTo, const std::map<std::string, std::string> &lstParams, std::string &result, std::string &message)
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

bool Nft::execEvents(const int64_t &fromBlock, const int64_t &toBlock, const int64_t &minconf, const std::string &eventName, const std::string &contractAddress, const int &numTopics, const FunctionABI &func, std::vector<NftEvent> &result)
{
    QVariant resultVar;
    if(!(d->eventLog->searchNftTx(d->model->node(), d->model, fromBlock, toBlock, minconf, eventName, contractAddress, resultVar)))
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
            NftEvent nftEvent;
            nftEvent.address = variantMap.value("contractAddress").toString().toStdString();
            nftEvent.blockHash = uint256S(variantMap.value("blockHash").toString().toStdString());
            nftEvent.blockNumber = variantMap.value("blockNumber").toLongLong();
            nftEvent.transactionHash = uint256S(variantMap.value("transactionHash").toString().toStdString());

            // Parse data
            std::vector<std::string> topics;
            for(QVariant topic : topicsList)
            {
                topics.push_back(topic.toString().toStdString());
            }
            std::string data = variantLog.value("data").toString().toStdString();
            addEvent(func, topics, data, nftEvent, result);
        }
    }

    return true;
}

bool Nft::privateKeysDisabled()
{
    if(!d || !d->model)
        return false;
    return d->model->wallet().privateKeysDisabled();
}
