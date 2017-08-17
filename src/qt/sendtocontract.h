#ifndef SENDTOCONTRACT_H
#define SENDTOCONTRACT_H

#include <QWidget>

class PlatformStyle;
class WalletModel;
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

    void setModel(WalletModel *model);

Q_SIGNALS:

public Q_SLOTS:
    void on_clearAll_clicked();
    void on_sendToContract_clicked();
    void on_updateGasValues();

private:
    Ui::SendToContract *ui;
    ExecRPCCommand* m_execRPCCommand;
    WalletModel* m_model;
};

#endif // SENDTOCONTRACT_H
