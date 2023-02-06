#ifndef CREATENFTDIALOG_H
#define CREATENFTDIALOG_H

#include <QDialog>
class Nft;
class WalletModel;
class ClientModel;

namespace Ui {
class CreateNftDialog;
}

class CreateNftDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateNftDialog(QWidget *parent = 0);
    ~CreateNftDialog();
    void clearAll();
    void setModel(WalletModel *_model);
    void setClientModel(ClientModel *clientModel);

Q_SIGNALS:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);

private Q_SLOTS:
    void on_clearButton_clicked();
    void on_gasInfoChanged(quint64 blockGasLimit, quint64 minGasPrice, quint64 nGasPrice);
    void on_confirmButton_clicked();
    void on_updateConfirmButton();
    void on_zeroBalanceAddressToken(bool enable);
    void updateDisplayUnit();

private:
    Ui::CreateNftDialog *ui;
    Nft *m_nftABI;
    WalletModel* m_model;
    ClientModel* m_clientModel;
    bool bCreateUnsigned = false;
};

#endif // CREATENFTDIALOG_H
