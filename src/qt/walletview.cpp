// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/walletview.h>

#include <qt/addressbookpage.h>
#include <qt/askpassphrasedialog.h>
#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/overviewpage.h>
#include <qt/platformstyle.h>
#include <qt/receiverequestdialog.h>
#include <qt/sendcoinsdialog.h>
#include <qt/signverifymessagedialog.h>
#include <qt/transactiontablemodel.h>
#include <qt/tokentransactiontablemodel.h>
#include <qt/tokentransactionrecord.h>
#include <qt/transactionview.h>
#include <qt/walletmodel.h>
#include <qt/createcontract.h>
#include <qt/sendtocontract.h>
#include <qt/callcontract.h>
#include <qt/qrctoken.h>
#include <qt/restoredialog.h>
#include <qt/stakepage.h>
#include <qt/delegationpage.h>
#include <qt/superstakerpage.h>
#include <qt/hardwaresigntxdialog.h>
#include <qt/walletframe.h>
#include <interfaces/node.h>
#include <node/ui_interface.h>
#include <util/strencodings.h>

#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QProgressDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <qt/qtumpushbutton.h>

WalletView::WalletView(WalletModel* wallet_model, const PlatformStyle* _platformStyle, QWidget* parent)
    : QStackedWidget(parent),
      clientModel(nullptr),
      walletModel(wallet_model),
    platformStyle(_platformStyle),
    walletFrame(qobject_cast<WalletFrame*>(parent))
{
    assert(walletModel);

    // Create tabs
    overviewPage = new OverviewPage(platformStyle);
    overviewPage->setWalletModel(walletModel);

    transactionsPage = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout();
    QHBoxLayout *hbox_buttons = new QHBoxLayout();
    transactionView = new TransactionView(platformStyle, this);
    transactionView->setModel(walletModel);

    vbox->addWidget(transactionView);
    QPushButton *exportButton = new QtumPushButton(tr("&Export"), this);
    exportButton->setToolTip(tr("Export the data in the current tab to a file"));
    if (platformStyle->getImagesOnButtons()) {
        exportButton->setIcon(platformStyle->MultiStatesIcon(":/icons/export", PlatformStyle::PushButton));
    }
    hbox_buttons->addStretch();
    hbox_buttons->addWidget(exportButton);
    vbox->addLayout(hbox_buttons);
    transactionsPage->setLayout(vbox);

    receiveCoinsPage = new ReceiveRequestDialog(platformStyle, overviewPage);
    receiveCoinsPage->setModel(walletModel);

    sendCoinsPage = new SendCoinsDialog(platformStyle, overviewPage);
    sendCoinsPage->setModel(walletModel);

    usedSendingAddressesPage = new AddressBookPage(platformStyle, AddressBookPage::ForEditing, AddressBookPage::SendingTab, this);
    usedSendingAddressesPage->setModel(walletModel->getAddressTableModel());

    usedReceivingAddressesPage = new AddressBookPage(platformStyle, AddressBookPage::ForEditing, AddressBookPage::ReceivingTab, this);
    usedReceivingAddressesPage->setModel(walletModel->getAddressTableModel());

    createContractPage = new CreateContract(platformStyle);
    createContractPage->setModel(walletModel);
    sendToContractPage = new SendToContract(platformStyle);
    sendToContractPage->setModel(walletModel);
    callContractPage = new CallContract(platformStyle);
    callContractPage->setModel(walletModel);

    QRCTokenPage = new QRCToken(platformStyle);
    QRCTokenPage->setModel(walletModel);

    stakePage = new StakePage(platformStyle);
    stakePage->setWalletModel(walletModel);
    delegationPage = new DelegationPage(platformStyle);
    delegationPage->setModel(walletModel);
    superStakerPage = new SuperStakerPage(platformStyle);
    superStakerPage->setModel(walletModel);

    addWidget(overviewPage);
    addWidget(transactionsPage);
    addWidget(createContractPage);
    addWidget(sendToContractPage);
    addWidget(callContractPage);
    addWidget(QRCTokenPage);
    addWidget(stakePage);
    addWidget(delegationPage);
    addWidget(superStakerPage);

    connect(overviewPage, &OverviewPage::transactionClicked, this, &WalletView::transactionClicked);
    // Clicking on a transaction on the overview pre-selects the transaction on the transaction history page
    connect(overviewPage, &OverviewPage::transactionClicked, transactionView, qOverload<const QModelIndex&>(&TransactionView::focusTransaction));

    connect(overviewPage, &OverviewPage::outOfSyncWarningClicked, this, &WalletView::outOfSyncWarningClicked);

    // Clicking on a transaction on the overview page simply sends you to transaction history page
    connect(overviewPage, &OverviewPage::showMoreClicked, this, &WalletView::showMore);

    // Clicking send coins button show send coins dialog
    connect(overviewPage, &OverviewPage::sendCoinsClicked, this, &WalletView::sendCoins);

    // Clicking receive coins button show receive coins dialog
    connect(overviewPage, &OverviewPage::receiveCoinsClicked, this, &WalletView::receiveCoins);

    connect(sendCoinsPage, &SendCoinsDialog::coinsSent, this, &WalletView::coinsSent);
    // Highlight transaction after send
    connect(sendCoinsPage, &SendCoinsDialog::coinsSent, transactionView, qOverload<const uint256&>(&TransactionView::focusTransaction));

    // Clicking on "Export" allows to export the transaction list
    connect(exportButton, &QPushButton::clicked, transactionView, &TransactionView::exportClicked);

    // Pass through messages from sendCoinsPage
    connect(sendCoinsPage, &SendCoinsDialog::message, this, &WalletView::message);
    // Pass through messages from transactionView
    connect(transactionView, &TransactionView::message, this, &WalletView::message);
    // Pass through messages from createContractPage
    connect(createContractPage, &CreateContract::message, this, &WalletView::message);
    // Pass through messages from sendToContractPage
    connect(sendToContractPage, &SendToContract::message, this, &WalletView::message);
    // Pass through messages from QRCTokenPage
    connect(QRCTokenPage, &QRCToken::message, this, &WalletView::message);
    // Pass through messages from delegationPage
    connect(delegationPage, &DelegationPage::message, this, &WalletView::message);
    // Pass through messages from superStakerPage
    connect(superStakerPage, &SuperStakerPage::message, this, &WalletView::message);

    connect(this, &WalletView::setPrivacy, overviewPage, &OverviewPage::setPrivacy);

    // Receive and pass through messages from wallet model
    connect(walletModel, &WalletModel::message, this, &WalletView::message);

    // Handle changes in encryption status
    connect(walletModel, &WalletModel::encryptionStatusChanged, this, &WalletView::encryptionStatusChanged);

    // Balloon pop-up for new transaction
    connect(walletModel->getTransactionTableModel(), &TransactionTableModel::rowsInserted, this, &WalletView::processNewTransaction);

    // Balloon pop-up for new token transaction
    connect(walletModel->getTokenTransactionTableModel(), &TokenTransactionTableModel::rowsInserted, this, &WalletView::processNewTokenTransaction);

    // Ask for passphrase if needed
    connect(walletModel, SIGNAL(requireUnlock()), this, SLOT(unlockWallet()));
    connect(stakePage, SIGNAL(requireUnlock(bool)), this, SLOT(unlockWallet(bool)));
    connect(walletModel, &WalletModel::encryptionStatusChanged, stakePage, &StakePage::updateEncryptionStatus);

    // Show progress dialog
    connect(walletModel, &WalletModel::showProgress, this, &WalletView::showProgress);
}

WalletView::~WalletView()
{
}

void WalletView::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;

    overviewPage->setClientModel(_clientModel);
    sendCoinsPage->setClientModel(_clientModel);
    createContractPage->setClientModel(_clientModel);
    sendToContractPage->setClientModel(_clientModel);
    callContractPage->setClientModel(_clientModel);
    QRCTokenPage->setClientModel(_clientModel);
    stakePage->setClientModel(_clientModel);
    delegationPage->setClientModel(_clientModel);
    superStakerPage->setClientModel(_clientModel);
    if (walletModel) walletModel->setClientModel(_clientModel);
}

void WalletView::processNewTransaction(const QModelIndex& parent, int start, int /*end*/)
{
    // Prevent balloon-spam when initial block download is in progress
    if (!clientModel || clientModel->node().isInitialBlockDownload()) {
        return;
    }

    TransactionTableModel *ttm = walletModel->getTransactionTableModel();
    if (!ttm || ttm->processingQueuedTransactions())
        return;

    QString date = ttm->index(start, TransactionTableModel::Date, parent).data().toString();
    qint64 amount = ttm->index(start, TransactionTableModel::Amount, parent).data(Qt::EditRole).toULongLong();
    QString type = ttm->index(start, TransactionTableModel::Type, parent).data().toString();
    QModelIndex index = ttm->index(start, 0, parent);
    QString address = ttm->data(index, TransactionTableModel::AddressRole).toString();
    QString label = GUIUtil::HtmlEscape(ttm->data(index, TransactionTableModel::LabelRole).toString());

    Q_EMIT incomingTransaction(date, walletModel->getOptionsModel()->getDisplayUnit(), amount, type, address, label, GUIUtil::HtmlEscape(walletModel->getWalletName()));
}

void WalletView::processNewTokenTransaction(const QModelIndex &parent, int start, int /*end*/)
{
    // Prevent balloon-spam when initial block download is in progress
    if (!walletModel || !clientModel || clientModel->node().isInitialBlockDownload())
        return;

    TokenTransactionTableModel *tttm = walletModel->getTokenTransactionTableModel();
    if (!tttm || tttm->processingQueuedTransactions())
        return;

    QString date = tttm->index(start, TokenTransactionTableModel::Date, parent).data().toString();
    QString amount(tttm->index(start, TokenTransactionTableModel::Amount, parent).data(TokenTransactionTableModel::FormattedAmountWithUnitRole).toString());
    QString type = tttm->index(start, TokenTransactionTableModel::Type, parent).data().toString();
    QModelIndex index = tttm->index(start, 0, parent);
    QString address = tttm->data(index, TokenTransactionTableModel::AddressRole).toString();
    QString label = tttm->data(index, TokenTransactionTableModel::LabelRole).toString();
    QString title;
    int txType = tttm->data(index, TokenTransactionTableModel::TypeRole).toInt();
    switch (txType)
    {
    case TokenTransactionRecord::RecvWithAddress:
    case TokenTransactionRecord::RecvFromOther:
        title = tr("Incoming transaction");
        break;
    default:
        title = tr("Sent transaction");
        break;
    }
    Q_EMIT incomingTokenTransaction(date, amount, type, address, label, walletModel->getWalletName(), title);
}

void WalletView::gotoOverviewPage()
{
    setCurrentWidget(overviewPage);
}

void WalletView::gotoHistoryPage()
{
    setCurrentWidget(transactionsPage);
}

void WalletView::gotoReceiveCoinsPage()
{
    setCurrentWidget(overviewPage);

    if(walletFrame && walletFrame->currentWalletView() == this)
    {
        receiveCoinsPage->show();
    }

}

void WalletView::gotoSendCoinsPage(QString addr)
{
    setCurrentWidget(overviewPage);
    if(walletFrame && walletFrame->currentWalletView() == this)
    {
        if (!addr.isEmpty())
            sendCoinsPage->setAddress(addr);
        sendCoinsPage->show();
    }
}

void WalletView::gotoCreateContractPage()
{
    setCurrentWidget(createContractPage);
}

void WalletView::gotoSendToContractPage()
{
    setCurrentWidget(sendToContractPage);
}

void WalletView::gotoCallContractPage()
{
    setCurrentWidget(callContractPage);
}

void WalletView::gotoTokenPage()
{
    setCurrentWidget(QRCTokenPage);
}

void WalletView::gotoStakePage()
{
    setCurrentWidget(stakePage);
}

void WalletView::gotoDelegationPage()
{
    setCurrentWidget(delegationPage);
}

void WalletView::gotoSuperStakerPage()
{
    setCurrentWidget(superStakerPage);
}

void WalletView::gotoSignMessageTab(QString addr)
{
    // calls show() in showTab_SM()
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(platformStyle, this);
    signVerifyMessageDialog->setAttribute(Qt::WA_DeleteOnClose);
    signVerifyMessageDialog->setModel(walletModel);
    signVerifyMessageDialog->showTab_SM(true);

    if (!addr.isEmpty())
        signVerifyMessageDialog->setAddress_SM(addr);
}

void WalletView::gotoVerifyMessageTab(QString addr)
{
    // calls show() in showTab_VM()
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(platformStyle, this);
    signVerifyMessageDialog->setAttribute(Qt::WA_DeleteOnClose);
    signVerifyMessageDialog->setModel(walletModel);
    signVerifyMessageDialog->showTab_VM(true);

    if (!addr.isEmpty())
        signVerifyMessageDialog->setAddress_VM(addr);
}

bool WalletView::handlePaymentRequest(const SendCoinsRecipient& recipient)
{
    return sendCoinsPage->handlePaymentRequest(recipient);
}

void WalletView::showOutOfSyncWarning(bool fShow)
{
    overviewPage->showOutOfSyncWarning(fShow);
}

void WalletView::encryptWallet()
{
    auto dlg = new AskPassphraseDialog(AskPassphraseDialog::Encrypt, this);
    dlg->setModel(walletModel);
    connect(dlg, &QDialog::finished, this, &WalletView::encryptionStatusChanged);
    GUIUtil::ShowModalDialogAsynchronously(dlg);
}

void WalletView::backupWallet()
{
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Backup Wallet"), QString(),
        //: Name of the wallet data file format.
        tr("Wallet Data") + QLatin1String(" (*.dat)"), nullptr);

    if (filename.isEmpty())
        return;

#ifndef WIN32
    // Use local encoding for non Windows OS
    std::string strFilename = filename.toLocal8Bit().data();
#else
    // Use utf8 encoding for Windows OS, the path will be converted into utf16 when the file is opened
    std::string strFilename = filename.toUtf8().data();
#endif

    if (!walletModel->wallet().backupWallet(strFilename)) {
        Q_EMIT message(tr("Backup Failed"), tr("There was an error trying to save the wallet data to %1.").arg(filename),
            CClientUIInterface::MSG_ERROR);
        }
    else {
        Q_EMIT message(tr("Backup Successful"), tr("The wallet data was successfully saved to %1.").arg(filename),
            CClientUIInterface::MSG_INFORMATION);
    }
}

void WalletView::restoreWallet()
{
    RestoreDialog dlg(this);
    dlg.setModel(walletModel);
    dlg.exec();
}

void WalletView::changePassphrase()
{
    auto dlg = new AskPassphraseDialog(AskPassphraseDialog::ChangePass, this);
    dlg->setModel(walletModel);
    GUIUtil::ShowModalDialogAsynchronously(dlg);
}

void WalletView::unlockWallet(bool fromMenu)
{
    if(!walletModel)
        return;
    // Unlock wallet when requested by wallet model
    if (walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        AskPassphraseDialog::Mode mode = fromMenu ?
            AskPassphraseDialog::UnlockStaking : AskPassphraseDialog::Unlock;
        AskPassphraseDialog dlg(mode, this);
        dlg.setModel(walletModel);
        // A modal dialog must be synchronous here as expected
        // in the WalletModel::requestUnlock() function.
        dlg.exec();

        if(sender() == stakePage)
            stakePage->updateEncryptionStatus();
    }
}

void WalletView::lockWallet()
{
    if(!walletModel)
        return;

    walletModel->setWalletLocked(true);
}

void WalletView::usedSendingAddresses()
{
    GUIUtil::bringToFront(usedSendingAddressesPage);
}

void WalletView::usedReceivingAddresses()
{
    GUIUtil::bringToFront(usedReceivingAddressesPage);
}

void WalletView::showProgress(const QString &title, int nProgress)
{
    if (nProgress == 0) {
        progressDialog = new QProgressDialog(title, tr("Cancel"), 0, 100);
        GUIUtil::PolishProgressDialog(progressDialog);
        progressDialog->setWindowModality(Qt::ApplicationModal);
        progressDialog->setAutoClose(false);
        progressDialog->setValue(0);
    } else if (nProgress == 100) {
        if (progressDialog) {
            progressDialog->close();
            progressDialog->deleteLater();
            progressDialog = nullptr;
        }
    } else if (progressDialog) {
        if (progressDialog->wasCanceled()) {
            getWalletModel()->wallet().abortRescan();
        } else {
            progressDialog->setValue(nProgress);
        }
    }
}

void WalletView::signTxHardware(const QString &tx)
{
    if(!walletModel)
        return;
    HardwareSignTxDialog dlg(tx, this);
    dlg.setModel(walletModel);
    dlg.exec();
}
