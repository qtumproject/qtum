// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/walletframe.h>
#include <qt/walletmodel.h>

#include <qt/bitcoingui.h>
#include <qt/walletview.h>
#include <qt/tabbarinfo.h>
#include <wallet/wallet.h>

#include <cassert>

#include <QHBoxLayout>
#include <QLabel>

WalletFrame::WalletFrame(const PlatformStyle *_platformStyle, BitcoinGUI *_gui) :
    QFrame(_gui),
    gui(_gui),
    platformStyle(_platformStyle)
{
    // Leave HBox hook for adding a list view later
    QHBoxLayout *walletFrameLayout = new QHBoxLayout(this);
    setContentsMargins(0,0,0,0);
    walletStack = new QStackedWidget(this);
    walletFrameLayout->setContentsMargins(0,0,0,0);
    walletFrameLayout->addWidget(walletStack);

    QLabel *noWallet = new QLabel(tr("No wallet has been loaded."));
    noWallet->setAlignment(Qt::AlignCenter);
    walletStack->addWidget(noWallet);
}

WalletFrame::~WalletFrame()
{
}

void WalletFrame::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;

    for (auto i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i) {
        i.value()->setClientModel(_clientModel);
    }
}

bool WalletFrame::addWallet(WalletModel *walletModel)
{
    if (!gui || !clientModel || !walletModel) return false;

    if (mapWalletViews.count(walletModel) > 0) return false;

    WalletView *walletView = new WalletView(platformStyle, this);
    walletView->setClientModel(clientModel);
    walletView->setWalletModel(walletModel);
    walletView->showOutOfSyncWarning(bOutOfSync);

    WalletView* current_wallet_view = currentWalletView();
    if (current_wallet_view) {
        walletView->setCurrentIndex(current_wallet_view->currentIndex());
    } else {
        walletView->gotoOverviewPage();
    }

    walletStack->addWidget(walletView);
    mapWalletViews[walletModel] = walletView;

    connect(walletView, &WalletView::outOfSyncWarningClicked, this, &WalletFrame::outOfSyncWarningClicked);
    connect(walletView, &WalletView::showMore, gui, &BitcoinGUI::gotoHistoryPage);
    connect(walletView, &WalletView::sendCoins, gui, &BitcoinGUI::gotoSendCoinsPage);
    connect(walletView, &WalletView::receiveCoins, gui, &BitcoinGUI::gotoReceiveCoinsPage);
    connect(walletView, &WalletView::transactionClicked, gui, &BitcoinGUI::gotoHistoryPage);
    connect(walletView, &WalletView::coinsSent, gui, &BitcoinGUI::gotoHistoryPage);
    connect(walletView, &WalletView::message, [this](const QString& title, const QString& message, unsigned int style) {
        gui->message(title, message, style);
    });
    connect(walletView, &WalletView::encryptionStatusChanged, gui, &BitcoinGUI::updateWalletStatus);
    connect(walletView, &WalletView::incomingTransaction, gui, &BitcoinGUI::incomingTransaction);
    connect(walletView, &WalletView::incomingTokenTransaction, gui, &BitcoinGUI::incomingTokenTransaction);
    connect(walletView, &WalletView::hdEnabledStatusChanged, gui, &BitcoinGUI::updateWalletStatus);
    connect(walletView, &WalletView::currentChanged, this, &WalletFrame::pageChanged);

    return true;
}

void WalletFrame::setCurrentWallet(WalletModel* wallet_model)
{
    if (mapWalletViews.count(wallet_model) == 0) return;

    WalletView *walletView = mapWalletViews.value(wallet_model);
    walletStack->setCurrentWidget(walletView);
    assert(walletView);
    walletView->updateEncryptionStatus();
}

void WalletFrame::removeWallet(WalletModel* wallet_model)
{
    if (mapWalletViews.count(wallet_model) == 0) return;

    WalletView *walletView = mapWalletViews.take(wallet_model);
    walletStack->removeWidget(walletView);
    delete walletView;
}

void WalletFrame::removeAllWallets()
{
    QMap<WalletModel*, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        walletStack->removeWidget(i.value());
    mapWalletViews.clear();
}

bool WalletFrame::handlePaymentRequest(const SendCoinsRecipient &recipient)
{
    WalletView *walletView = currentWalletView();
    if (!walletView)
        return false;

    return walletView->handlePaymentRequest(recipient);
}

void WalletFrame::showOutOfSyncWarning(bool fShow)
{
    bOutOfSync = fShow;
    QMap<WalletModel*, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->showOutOfSyncWarning(fShow);
}

void WalletFrame::gotoOverviewPage()
{
    QMap<WalletModel*, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoOverviewPage();
}

void WalletFrame::gotoHistoryPage()
{
    QMap<WalletModel*, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoHistoryPage();
}

void WalletFrame::gotoTokenPage()
{
    QMap<WalletModel*, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoTokenPage();
}

void WalletFrame::gotoDelegationPage()
{
    QMap<WalletModel*, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoDelegationPage();
}

void WalletFrame::gotoSuperStakerPage()
{
    QMap<WalletModel*, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoSuperStakerPage();
}

void WalletFrame::gotoReceiveCoinsPage()
{
    QMap<WalletModel*, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoReceiveCoinsPage();
}

void WalletFrame::gotoSendCoinsPage(QString addr)
{
    QMap<WalletModel*, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoSendCoinsPage(addr);
}

void WalletFrame::gotoCreateContractPage()
{
    QMap<WalletModel*, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoCreateContractPage();
}

void WalletFrame::gotoSendToContractPage()
{
    QMap<WalletModel*, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoSendToContractPage();
}

void WalletFrame::gotoCallContractPage()
{
    QMap<WalletModel*, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoCallContractPage();
}

void WalletFrame::gotoStakePage()
{
    QMap<WalletModel*, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoStakePage();
}

void WalletFrame::gotoSignMessageTab(QString addr)
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->gotoSignMessageTab(addr);
}

void WalletFrame::gotoVerifyMessageTab(QString addr)
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->gotoVerifyMessageTab(addr);
}

void WalletFrame::encryptWallet(bool status)
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->encryptWallet(status);
}

void WalletFrame::backupWallet()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->backupWallet();
}

void WalletFrame::restoreWallet()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->restoreWallet();
}

void WalletFrame::changePassphrase()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->changePassphrase();
}

void WalletFrame::unlockWallet()
{
    QObject* object = sender();
    QString objectName = object ? object->objectName() : "";
    bool fromMenu = objectName == "unlockWalletAction";
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->unlockWallet(fromMenu);
}

void WalletFrame::lockWallet()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
    {
        walletView->lockWallet();
        walletView->getWalletModel()->setWalletUnlockStakingOnly(false);
    }
}

void WalletFrame::usedSendingAddresses()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->usedSendingAddresses();
}

void WalletFrame::usedReceivingAddresses()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->usedReceivingAddresses();
}

WalletView* WalletFrame::currentWalletView() const
{
    return qobject_cast<WalletView*>(walletStack->currentWidget());
}

WalletModel* WalletFrame::currentWalletModel() const
{
    WalletView* wallet_view = currentWalletView();
    return wallet_view ? wallet_view->getWalletModel() : nullptr;
}

void WalletFrame::outOfSyncWarningClicked()
{
    Q_EMIT requestedSyncWarningInfo();
}

void WalletFrame::pageChanged(int index)
{
    updateTabBar(0, index);
}

void WalletFrame::updateTabBar(WalletView *walletView, int index)
{
    // update default parameters
    if(walletView == 0)
    {
        walletView = currentWalletView();
    }
    if(walletView && index == -1)
    {
        index = walletView->currentIndex();
    }

    // update the tab bar into the title bar
    bool found = false;
    if(walletView && walletView->count() > index)
    {
        QWidget* currentPage = walletView->widget(index);
        QObject* info = currentPage->findChild<TabBarInfo *>("");
        gui->setTabBarInfo(info);
        found = true;
    }
    if(!found)
    {
        gui->setTabBarInfo(0);
    }
}
