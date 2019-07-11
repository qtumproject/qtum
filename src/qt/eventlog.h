#ifndef EVENTLOG_H
#define EVENTLOG_H
#include <string>
#include <vector>
#include <QMap>
#include <QVariant>
#include <interfaces/node.h>
#include <qt/walletmodel.h>

class ExecRPCCommand;

class EventLog
{
public:
    /**
     * @brief EventLog Constructor
     */
    EventLog();

    /**
     * @brief ~EventLog Destructor
     */
    ~EventLog();

    /**
     * @brief searchTokenTx Search the event log for token transactions
     * @param node Select node to search
     * @param wallet Select wallet to search
     * @param fromBlock Begin from block
     * @param toBlock End to block
     * @param strContractAddress Token contract address
     * @param strSenderAddress Token sender address
     * @param result Result of the performed call
     * @return success of the operation
     */
    bool searchTokenTx(interfaces::Node& node, const WalletModel* wallet_model, int64_t fromBlock, int64_t toBlock, std::string strContractAddress, std::string strSenderAddress, QVariant& result);

    /**
     * @brief search Search for log events
     * @param node Select node to search
     * @param wallet Select wallet to search
     * @param fromBlock Begin from block
     * @param toBlock End to block
     * @param addresses Contract address
     * @param topics Event topics
     * @param result Result of the performed call
     * @return success of the operation
     */
    bool search(interfaces::Node& node,  const WalletModel* wallet_model, int64_t fromBlock, int64_t toBlock, const std::vector<std::string> addresses, const std::vector<std::string> topics, QVariant& result);

private:
    // Set command data
    void setStartBlock(int64_t fromBlock);
    void setEndBlock(int64_t toBlock);
    void setAddresses(const std::vector<std::string> addresses);
    void setTopics(const std::vector<std::string> topics);

    ExecRPCCommand* m_RPCCommand;
    QMap<QString, QString> m_lstParams;
};

#endif // EVENTLOG_H
