// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_RECEIVEREQUESTDIALOG_H
#define BITCOIN_QT_RECEIVEREQUESTDIALOG_H

#include <qt/sendcoinsrecipient.h>

#include <QDialog>

class PlatformStyle;
class ReceiveCoinsDialog;
class WalletModel;

namespace Ui {
    class ReceiveRequestDialog;
}

class ReceiveRequestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReceiveRequestDialog(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~ReceiveRequestDialog();

    void setModel(WalletModel *model);
    void setInfo(const SendCoinsRecipient &info);

public Q_SLOTS:
    void clear();
    void reject();
    void accept();

private Q_SLOTS:
    void on_btnCopyURI_clicked();
    void on_btnCopyAddress_clicked();
    void on_btnRefreshAddress_clicked();
    void on_btnRequestPayment_clicked();
    void on_btnClear_clicked();

    void update();

private:
    bool refreshAddress();
    bool getDefaultAddress();

private:
    Ui::ReceiveRequestDialog *ui;
    WalletModel *model;
    SendCoinsRecipient info;
    const PlatformStyle *platformStyle;
    ReceiveCoinsDialog* requestPaymentDialog;
};

#endif // BITCOIN_QT_RECEIVEREQUESTDIALOG_H
