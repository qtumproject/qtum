// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_WALLETVIEW_H
#define BITCOIN_QT_WALLETVIEW_H

#include <consensus/amount.h>
#include <qt/bitcoinunits.h>

#include <QStackedWidget>

class ClientModel;
class OverviewPage;
class PlatformStyle;
class ReceiveRequestDialog;
class SendCoinsDialog;
class SendCoinsRecipient;
class TransactionView;
class WalletModel;
class AddressBookPage;
class CreateContract;
class SendToContract;
class CallContract;
class QRCToken;
class StakePage;
class DelegationPage;
class SuperStakerPage;
class WalletFrame;
QT_BEGIN_NAMESPACE
class QModelIndex;
class QProgressDialog;
QT_END_NAMESPACE

/*
  WalletView class. This class represents the view to a single wallet.
  It was added to support multiple wallet functionality. Each wallet gets its own WalletView instance.
  It communicates with both the client and the wallet models to give the user an up-to-date view of the
  current core state.
*/
class WalletView : public QStackedWidget
{
    Q_OBJECT

public:
    explicit WalletView(WalletModel* wallet_model, const PlatformStyle* platformStyle, QWidget* parent);
    ~WalletView();

    /** Set the client model.
        The client model represents the part of the core that communicates with the P2P network, and is wallet-agnostic.
    */
    void setClientModel(ClientModel *clientModel);
    WalletModel* getWalletModel() const noexcept { return walletModel; }

    bool handlePaymentRequest(const SendCoinsRecipient& recipient);

    void showOutOfSyncWarning(bool fShow);

private:
    ClientModel* clientModel{nullptr};

    //!
    //! The wallet model represents a bitcoin wallet, and offers access to
    //! the list of transactions, address book and sending functionality.
    //!
    WalletModel* const walletModel;

    OverviewPage *overviewPage;
    QWidget *transactionsPage;
    ReceiveRequestDialog *receiveCoinsPage;
    SendCoinsDialog *sendCoinsPage;
    AddressBookPage *usedSendingAddressesPage;
    AddressBookPage *usedReceivingAddressesPage;
    CreateContract* createContractPage;
    SendToContract* sendToContractPage;
    CallContract* callContractPage;
    QRCToken* QRCTokenPage;
    StakePage *stakePage;
    DelegationPage* delegationPage;
    SuperStakerPage* superStakerPage;
    TransactionView *transactionView;

    QProgressDialog* progressDialog{nullptr};
    const PlatformStyle *platformStyle;
    WalletFrame *walletFrame;

public Q_SLOTS:
    /** Switch to overview (home) page */
    void gotoOverviewPage();
    /** Switch to history (transactions) page */
    void gotoHistoryPage();
    /** Switch to receive coins page */
    void gotoReceiveCoinsPage();
    /** Switch to send coins page */
    void gotoSendCoinsPage(QString addr = "");
    /** Switch to create contract page */
    void gotoCreateContractPage();
    /** Switch to send contract page */
    void gotoSendToContractPage();
    /** Switch to call contract page */
    void gotoCallContractPage();
    /** Switch to token page */
    void gotoTokenPage();
    /** Switch to stake page */
    void gotoStakePage();
    /** Switch to delegation page */
    void gotoDelegationPage();
    /** Switch to super staker page */
    void gotoSuperStakerPage();
    /** Show Sign/Verify Message dialog and switch to sign message tab */
    void gotoSignMessageTab(QString addr = "");
    /** Show Sign/Verify Message dialog and switch to verify message tab */
    void gotoVerifyMessageTab(QString addr = "");

    /** Show incoming transaction notification for new transactions.

        The new items are those between start and end inclusive, under the given parent item.
    */
    void processNewTransaction(const QModelIndex& parent, int start, int /*end*/);

    /** Show incoming token transaction notification for new token transactions.

        The new items are those between start and end inclusive, under the given parent item.
    */
    void processNewTokenTransaction(const QModelIndex& parent, int start, int /*end*/);
    /** Encrypt the wallet */
    void encryptWallet();
    /** Backup the wallet */
    void backupWallet();
    /** Restore the wallet */
    void restoreWallet();
    /** Change encrypted wallet passphrase */
    void changePassphrase();
    /** Ask for passphrase to unlock wallet temporarily */
    void unlockWallet(bool fromMenu = false);
    /** Lock the wallet */
    void lockWallet();
    /** Sign transaction with hardware wallet*/
    void signTxHardware(const QString& tx = "");
    /** Show used sending addresses */
    void usedSendingAddresses();
    /** Show used receiving addresses */
    void usedReceivingAddresses();

    /** Show progress dialog e.g. for rescan */
    void showProgress(const QString &title, int nProgress);

private Q_SLOTS:
    void disableTransactionView(bool disable);

Q_SIGNALS:
    void setPrivacy(bool privacy);
    void transactionClicked();
    void coinsSent();
    /**  Fired when a message should be reported to the user */
    void message(const QString &title, const QString &message, unsigned int style);
    /** Encryption status of wallet changed */
    void encryptionStatusChanged();
    /** Notify that a new transaction appeared */
    void incomingTransaction(const QString& date, BitcoinUnit unit, const CAmount& amount, const QString& type, const QString& address, const QString& label, const QString& walletName);
    /** Notify that a new token transaction appeared */
    void incomingTokenTransaction(const QString& date, const QString& amount, const QString& type, const QString& address, const QString& label, const QString& walletName, const QString& title);
    /** Notify that the out of sync warning icon has been pressed */
    void outOfSyncWarningClicked();
    void showMore();
    void sendCoins(QString addr = "");
    void receiveCoins();
};

#endif // BITCOIN_QT_WALLETVIEW_H
