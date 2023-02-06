#ifndef CONTRACTRESULT_H
#define CONTRACTRESULT_H

#include <QStackedWidget>

class FunctionABI;
class WalletModel;
class ClientModel;
class ExecRPCCommand;

namespace Ui {
class ContractResult;
}

class ContractResult : public QStackedWidget
{
    Q_OBJECT

public:
    enum ContractTxType{
        CreateResult,
        SendToResult,
        CallResult
    };
    explicit ContractResult(QWidget *parent = 0);
    ~ContractResult();
    void setResultData(QVariant result, FunctionABI function, QList<QStringList> paramValues, ContractTxType type);
    void setClientModel(ClientModel *clientModel);
    void setModel(WalletModel *model);

public Q_SLOTS:
    void tipChanged();

private:
    Ui::ContractResult *ui;
    void setParamsData(FunctionABI function, QList<QStringList> paramValues);
    void updateCreateResult(QVariant result);
    void updateSendToResult(QVariant result);
    void updateCallResult(QVariant result, FunctionABI function, QList<QStringList> paramValues);
    void updateCreateSendToResult(const QVariant& result, bool create);
private:
    WalletModel* m_model;
    ClientModel* m_clientModel;
    ExecRPCCommand* m_execRPCCommand;
    QString m_txid;
    bool m_showReceipt;
};

#endif // CONTRACTRESULT_H
