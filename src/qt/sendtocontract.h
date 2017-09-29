#ifndef SENDTOCONTRACT_H
#define SENDTOCONTRACT_H

#include <QWidget>

class PlatformStyle;
class WalletModel;
class ClientModel;
class ExecRPCCommand;
class ABIFunctionField;
class ContractABI;
class TabBarInfo;

namespace Ui {
class SendToContract;
}

class SendToContract : public QWidget
{
    Q_OBJECT

public:
    explicit SendToContract(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~SendToContract();

    void setClientModel(ClientModel *clientModel);
    void setModel(WalletModel *model);
    bool isValidContractAddress();
    bool isValidInterfaceABI();
    bool isDataValid();

Q_SIGNALS:

public Q_SLOTS:
    void on_clearAll_clicked();
    void on_sendToContract_clicked();
    void on_numBlocksChanged();
    void on_updateSendToContractButton();
    void on_newContractABI();
    void on_functionChanged();

private:
    QString toDataHex(int func, QString& errorMessage);
    bool isFunctionPayable();

private:
    Ui::SendToContract *ui;
    WalletModel* m_model;
    ClientModel* m_clientModel;
    ExecRPCCommand* m_execRPCCommand;
    ABIFunctionField* m_ABIFunctionField;
    ContractABI* m_contractABI;
    TabBarInfo* m_tabInfo;
};

#endif // SENDTOCONTRACT_H
