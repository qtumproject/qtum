#include <qt/eventlog.h>
#include <qt/execrpccommand.h>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <uint256.h>

namespace EventLog_NS
{
static const QString RPC_SERACH_LOGS = "searchlogs";
static const QString PARAM_FROM_BLOCK = "fromBlock";
static const QString PARAM_TO_BLOCK = "toBlock";
static const QString PARAM_ADDRESSES = "address";
static const QString PARAM_TOPICS = "topics";
}
using namespace EventLog_NS;

QString createJsonString(std::string key, const std::vector<std::string> value)
{
    QJsonObject json;
    QJsonArray array;
    for(size_t i = 0; i < value.size(); i++)
    {
        array.append(QJsonValue(QString::fromStdString(value[i])));
    }
    json.insert(QString::fromStdString(key), array);

    QJsonDocument doc(json);
    QString retString(doc.toJson(QJsonDocument::Compact));
    retString.insert(0, "'");
    retString.append("'");

    return retString;
}

EventLog::EventLog():
    m_RPCCommand(0)
{
    // Create new searchlogs command line interface
    QStringList lstMandatory;
    lstMandatory.append(PARAM_FROM_BLOCK);
    lstMandatory.append(PARAM_TO_BLOCK);
    QStringList lstOptional;
    lstOptional.append(PARAM_ADDRESSES);
    lstOptional.append(PARAM_TOPICS);
    m_RPCCommand = new ExecRPCCommand(RPC_SERACH_LOGS, lstMandatory, lstOptional, QMap<QString, QString>());
}

EventLog::~EventLog()
{
    if(m_RPCCommand)
    {
        delete m_RPCCommand;
        m_RPCCommand = 0;
    }
}

bool EventLog::searchTokenTx(interfaces::Node& node, const WalletModel* wallet_model, int64_t fromBlock, int64_t toBlock, std::string strContractAddress, std::string strSenderAddress, QVariant &result)
{
    std::vector<std::string> addresses;
    addresses.push_back(strContractAddress);

    std::vector<std::string> topics;
    // Skip the event type check
    static std::string nullRecord = uint256().ToString();
    topics.push_back(nullRecord);
    // Match the log with sender address
    topics.push_back(strSenderAddress);
    // Match the log with receiver address
    topics.push_back(strSenderAddress);

    return search(node, wallet_model, fromBlock, toBlock, addresses, topics, result);
}

bool EventLog::search(interfaces::Node& node, const WalletModel* wallet_model, int64_t fromBlock, int64_t toBlock, const std::vector<std::string> addresses, const std::vector<std::string> topics, QVariant &result)
{
    setStartBlock(fromBlock);
    setEndBlock(toBlock);
    setAddresses(addresses);
    setTopics(topics);

    QString resultJson;
    QString errorMessage;
    if(!m_RPCCommand->exec(node, wallet_model, m_lstParams, result, resultJson, errorMessage))
        return false;
    return true;
}

void EventLog::setStartBlock(int64_t fromBlock)
{
    m_lstParams[PARAM_FROM_BLOCK] = QString::number(fromBlock);
}

void EventLog::setEndBlock(int64_t toBlock)
{
    m_lstParams[PARAM_TO_BLOCK] = QString::number(toBlock);
}

void EventLog::setAddresses(const std::vector<std::string> addresses)
{
    m_lstParams[PARAM_ADDRESSES] = createJsonString("addresses", addresses);
}

void EventLog::setTopics(const std::vector<std::string> topics)
{
    m_lstParams[PARAM_TOPICS] = createJsonString("topics", topics);
}
