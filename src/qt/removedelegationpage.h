#ifndef REMOVEDELEGATIONPAGE_H
#define REMOVEDELEGATIONPAGE_H

#include <QDialog>
class WalletModel;
class ClientModel;
class ExecRPCCommand;

namespace Ui {
class RemoveDelegationPage;
}

class RemoveDelegationPage : public QDialog
{
    Q_OBJECT

public:
    explicit RemoveDelegationPage(QWidget *parent = nullptr);
    ~RemoveDelegationPage();
    void setModel(WalletModel *_model);
    void setClientModel(ClientModel *_clientModel);

public Q_SLOTS:
    void on_gasInfoChanged(quint64 blockGasLimit, quint64 minGasPrice, quint64 nGasPrice);

private Q_SLOTS:
    void on_removeDelegationClicked();
    void on_updateRemoveDelegationButton();
    void updateDisplayUnit();

private:
    Ui::RemoveDelegationPage *ui;
    WalletModel* m_model;
    ClientModel* m_clientModel;
    ExecRPCCommand *m_execRPCCommand;
};

#endif // REMOVEDELEGATIONPAGE_H
