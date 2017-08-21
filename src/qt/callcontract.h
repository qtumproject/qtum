#ifndef CALLCONTRACT_H
#define CALLCONTRACT_H

#include <QWidget>

class PlatformStyle;
class ClientModel;
class ExecRPCCommand;

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

Q_SIGNALS:

public Q_SLOTS:
    void on_clearAll_clicked();
    void on_callContract_clicked();
    void on_numBlocksChanged();
    void on_updateCallContractButton();

private:
    Ui::CallContract *ui;
    ClientModel* m_clientModel;
    ExecRPCCommand* m_execRPCCommand;
};

#endif // CALLCONTRACT_H
