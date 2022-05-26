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

Q_SIGNALS:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);

public Q_SLOTS:
    void on_gasInfoChanged(quint64 blockGasLimit, quint64 minGasPrice, quint64 nGasPrice);
    void accept() override;
    void reject() override;
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
    bool bCreateUnsigned = false;
};

#endif // ADDDELEGATIONPAGE_H
