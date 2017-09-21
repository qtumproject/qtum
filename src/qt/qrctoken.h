#ifndef QRCTOKEN_H
#define QRCTOKEN_H

#include "sendtokenpage.h"
#include "receivetokenpage.h"
#include "addtokenpage.h"

#include <QWidget>
#include <QModelIndex>

class TokenViewDelegate;
class WalletModel;
class QStandardItemModel;

namespace Ui {
class QRCToken;
}

class QRCToken : public QWidget
{
    Q_OBJECT

public:
    explicit QRCToken(QWidget *parent = 0);
    ~QRCToken();

    void setModel(WalletModel *_model);

Q_SIGNALS:

public Q_SLOTS:
    void on_sendButton_clicked();
    void on_receiveButton_clicked();
    void on_addTokenButton_clicked();
    void on_addToken(QString address, QString name, QString symbol, int decimals, double balance);

private:
    Ui::QRCToken *ui;
    SendTokenPage* m_sendTokenPage;
    ReceiveTokenPage* m_receiveTokenPage;
    AddTokenPage* m_addTokenPage;
    WalletModel* m_model;
    TokenViewDelegate* m_tokenDelegate;
    QStandardItemModel *m_tokenModel;
};

#endif // QRCTOKEN_H
