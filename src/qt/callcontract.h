#ifndef CALLCONTRACT_H
#define CALLCONTRACT_H

#include <QWidget>

class PlatformStyle;
class ClientModel;
class ExecRPCCommand;
class ABIFunctionField;
class ContractABI;
class TabBarInfo;

namespace Ui {
class CallContract;
}

class CallContract : public QWidget
{
    Q_OBJECT

public:
    explicit CallContract(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~CallContract();

    void setClientModel(ClientModel *clientModel);
    bool isValidContractAddress();
    bool isValidInterfaceABI();
    bool isDataValid();

Q_SIGNALS:

public Q_SLOTS:
    void on_clearAll_clicked();
    void on_callContract_clicked();
    void on_numBlocksChanged();
    void on_updateCallContractButton();
    void on_newContractABI();

private:
    QString toDataHex(int func, QString& errorMessage);

private:
    Ui::CallContract *ui;
    ClientModel* m_clientModel;
    ExecRPCCommand* m_execRPCCommand;
    ABIFunctionField* m_ABIFunctionField;
    ContractABI* m_contractABI;
    TabBarInfo* m_tabInfo;
};

#endif // CALLCONTRACT_H
