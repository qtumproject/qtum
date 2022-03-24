#ifndef SENDNFTDIALOG_H
#define SENDNFTDIALOG_H

#include <QDialog>

class WalletModel;
class ClientModel;
class Nft;
struct SelectedNft;

namespace Ui {
class SendNftDialog;
}

class SendNftDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendNftDialog(QWidget *parent = 0);
    ~SendNftDialog();

    void setModel(WalletModel *_model);
    void setClientModel(ClientModel *clientModel);
    void clearAll();
    bool isValidAddress();
    bool isDataValid();

    void setNftData(std::string sender, std::string id, std::string balance);

Q_SIGNALS:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);

public Q_SLOTS:
    void reject();
    void accept();

private Q_SLOTS:
    void on_clearButton_clicked();
    void on_gasInfoChanged(quint64 blockGasLimit, quint64 minGasPrice, quint64 nGasPrice);
    void on_updateConfirmButton();
    void on_confirmClicked();
    void updateDisplayUnit();

private:
    Ui::SendNftDialog *ui;
    WalletModel* m_model;
    ClientModel* m_clientModel;
    Nft *m_nftABI;
    SelectedNft *m_selectedNft;
    bool bCreateUnsigned = false;
};

#endif // SENDNFTDIALOG_H
