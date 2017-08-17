#ifndef CREATECONTRACT_H
#define CREATECONTRACT_H

#include <QWidget>

class PlatformStyle;
class WalletModel;
class ExecRPCCommand;

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
    void setModel(WalletModel *model);

Q_SIGNALS:

public Q_SLOTS:
    void on_clearAll_clicked();
    void on_createContract_clicked();
    void on_updateGasValues();

private:
    Ui::CreateContract *ui;
    ExecRPCCommand* m_execRPCCommand;
    WalletModel *model;
};

#endif // CREATECONTRACT_H
