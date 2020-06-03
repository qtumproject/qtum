#ifndef ADDDELEGATIONPAGE_H
#define ADDDELEGATIONPAGE_H

#include <QDialog>
class WalletModel;
class ClientModel;
class ExecRPCCommand;

namespace Ui {
class AddDelegationPage;
}

class AddDelegationPage : public QDialog
{
    Q_OBJECT

public:
    explicit AddDelegationPage(QWidget *parent = nullptr);
    ~AddDelegationPage();
    void setModel(WalletModel *_model);
    void setClientModel(ClientModel *_clientModel);
    void clearAll();
    bool isValidStakerAddress();
    bool isDataValid();

public Q_SLOTS:
    void on_gasInfoChanged(quint64 blockGasLimit, quint64 minGasPrice, quint64 nGasPrice);
    void accept();
    void reject();
    void show();

private Q_SLOTS:
    void on_clearButton_clicked();
    void on_addDelegationClicked();
    void on_updateAddDelegationButton();
    void updateDisplayUnit();

private:
    Ui::AddDelegationPage *ui;
    WalletModel* m_model;
    ClientModel* m_clientModel;
    ExecRPCCommand *m_execRPCCommand;
};

#endif // ADDDELEGATIONPAGE_H
