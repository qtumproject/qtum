#ifndef CALLCONTRACT_H
#define CALLCONTRACT_H

#include <QWidget>

class PlatformStyle;
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

Q_SIGNALS:

public Q_SLOTS:
    void on_clearAll_clicked();
    void on_callContract_clicked();
    void on_updateCallContractButton();

private:
    Ui::CallContract *ui;
    ExecRPCCommand* m_execRPCCommand;
};

#endif // CALLCONTRACT_H
