#ifndef CREATECONTRACT_H
#define CREATECONTRACT_H

#include <QWidget>

class PlatformStyle;
class WalletModel;
class ClientModel;
class ExecRPCCommand;
class ABIFunctionField;
class ContractABI;
class TabBarInfo;

namespace Ui {
class CreateContract;
}

class CreateContract : public QWidget
{
    Q_OBJECT

public:
    explicit CreateContract(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~CreateContract();

    void setLinkLabels();
    void setClientModel(ClientModel *clientModel);
    void setModel(WalletModel *model);
    bool isValidBytecode();
    bool isValidInterfaceABI();
    bool isDataValid();

Q_SIGNALS:

public Q_SLOTS:
    void on_clearAllClicked();
    void on_createContractClicked();
    void on_gasInfoChanged(quint64 blockGasLimit, quint64 minGasPrice, quint64 nGasPrice);
    void on_updateCreateButton();
    void on_newContractABI();

private Q_SLOTS:
    void updateDisplayUnit();

private:
    QString toDataHex(int func, QString& errorMessage);

private:

    Ui::CreateContract *ui;
    WalletModel* m_model;
    ClientModel* m_clientModel;
    ExecRPCCommand* m_execRPCCommand;
    ABIFunctionField* m_ABIFunctionField;
    ContractABI* m_contractABI;
    TabBarInfo* m_tabInfo;
    int m_results;
};

#endif // CREATECONTRACT_H
