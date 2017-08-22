#ifndef SENDTOCONTRACT_H
#define SENDTOCONTRACT_H

#include <QWidget>

class PlatformStyle;
class WalletModel;
class ClientModel;
class ExecRPCCommand;

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

Q_SIGNALS:

public Q_SLOTS:
    void on_clearAll_clicked();
    void on_sendToContract_clicked();
    void on_numBlocksChanged();
    void on_updateSendToContractButton();

private:
    Ui::SendToContract *ui;
    WalletModel* m_model;
    ClientModel* m_clientModel;
    ExecRPCCommand* m_execRPCCommand;
};

#endif // SENDTOCONTRACT_H
