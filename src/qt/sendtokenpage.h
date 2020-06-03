#ifndef SENDTOKENPAGE_H
#define SENDTOKENPAGE_H

#include <QDialog>

class WalletModel;
class ClientModel;
class Token;
struct SelectedToken;

namespace Ui {
class SendTokenPage;
}

class SendTokenPage : public QDialog
{
    Q_OBJECT

public:
    explicit SendTokenPage(QWidget *parent = 0);
    ~SendTokenPage();

    void setModel(WalletModel *_model);
    void setClientModel(ClientModel *clientModel);
    void clearAll();
    bool isValidAddress();
    bool isDataValid();

    void setTokenData(std::string address, std::string sender, std::string symbol, int8_t decimals, std::string balance);

private Q_SLOTS:
    void on_clearButton_clicked();
    void on_gasInfoChanged(quint64 blockGasLimit, quint64 minGasPrice, quint64 nGasPrice);
    void on_updateConfirmButton();
    void on_confirmClicked();
    void updateDisplayUnit();

private:
    Ui::SendTokenPage *ui;
    WalletModel* m_model;
    ClientModel* m_clientModel;
    Token *m_tokenABI;
    SelectedToken *m_selectedToken;
};

#endif // SENDTOKENPAGE_H
