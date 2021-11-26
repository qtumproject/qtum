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
    void clearAll();
    void setDelegationData(const QString& address, const QString& hash);
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
    void on_removeDelegationClicked();
    void on_updateRemoveDelegationButton();
    void updateDisplayUnit();

private:
    Ui::RemoveDelegationPage *ui;
    WalletModel* m_model;
    ClientModel* m_clientModel;
    ExecRPCCommand *m_execRPCCommand;
    QString address;
    QString hash;
    bool bCreateUnsigned = false;
};

#endif // REMOVEDELEGATIONPAGE_H
