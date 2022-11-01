// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/bitcoingui.h>

#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/createwalletdialog.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/modaloverlay.h>
#include <qt/networkstyle.h>
#include <qt/notificator.h>
#include <qt/openuridialog.h>
#include <qt/optionsdialog.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/rpcconsole.h>
#include <qt/utilitydialog.h>
#include <rpc/server.h>
#include <qt/navigationbar.h>
#include <qt/titlebar.h>
#include <qt/qtumversionchecker.h>
#include <qt/styleSheet.h>
#ifdef ENABLE_WALLET
#include <qt/walletcontroller.h>
#include <qt/walletframe.h>
#include <qt/walletmodel.h>
#include <qt/walletview.h>
#include <wallet/wallet.h>
#endif // ENABLE_WALLET

#ifdef Q_OS_MAC
#include <qt/macdockiconhandler.h>
#endif

#include <functional>
#include <chain.h>
#include <chainparams.h>
#include <interfaces/handler.h>
#include <interfaces/node.h>
#include <node/ui_interface.h>
#include <util/system.h>
#include <util/translation.h>
#include <validation.h>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QCursor>
#include <QDateTime>
#include <QDragEnterEvent>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressDialog>
#include <QScreen>
#include <QSettings>
#include <QShortcut>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QToolBar>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QWindow>
#include <QDockWidget>
#include <QSizeGrip>


const std::string BitcoinGUI::DEFAULT_UIPLATFORM =
#if defined(Q_OS_MAC)
        "macosx"
#elif defined(Q_OS_WIN)
        "windows"
#else
        "other"
#endif
        ;

const int DISPLAY_UNIT_CONTROL_MARGIN = 25;
const int DISPLAY_UNIT_CONTROL_HEIGHT = 24;

BitcoinGUI::BitcoinGUI(interfaces::Node& node, const PlatformStyle *_platformStyle, const NetworkStyle *networkStyle, QWidget *parent) :
    QMainWindow(parent),
    m_node(node),
    trayIconMenu{new QMenu()},
    platformStyle(_platformStyle),
    m_network_style(networkStyle)
{
    QSettings settings;
    if (!restoreGeometry(settings.value("MainWindowGeometry").toByteArray())) {
        // Restore failed (perhaps missing setting), center the window
        move(QGuiApplication::primaryScreen()->availableGeometry().center() - frameGeometry().center());
    }

    setContextMenuPolicy(Qt::PreventContextMenu);

#ifdef ENABLE_WALLET
    enableWallet = WalletModel::isWalletEnabled();
#endif // ENABLE_WALLET
    QApplication::setWindowIcon(m_network_style->getTrayAndWindowIcon());
    setWindowIcon(m_network_style->getTrayAndWindowIcon());
    updateWindowTitle();

    rpcConsole = new RPCConsole(node, _platformStyle, nullptr);
    helpMessageDialog = new HelpMessageDialog(this, false);
#ifdef ENABLE_WALLET
    if(enableWallet)
    {
        /** Create wallet frame and make it the central widget */
        walletFrame = new WalletFrame(_platformStyle, this);
        connect(walletFrame, &WalletFrame::createWalletButtonClicked, [this] {
            auto activity = new CreateWalletActivity(getWalletController(), this);
            activity->create();
        });
        connect(walletFrame, &WalletFrame::message, [this](const QString& title, const QString& message, unsigned int style) {
            this->message(title, message, style);
        });
        connect(walletFrame, &WalletFrame::currentWalletSet, [this] { updateWalletStatus(); });
        setCentralWidget(walletFrame);
    } else
#endif // ENABLE_WALLET
    {
        /* When compiled without wallet or -disablewallet is provided,
         * the central widget is the rpc console.
         */
        setCentralWidget(rpcConsole);
        Q_EMIT consoleShown(rpcConsole);
    }

    // Accept D&D of URIs
    setAcceptDrops(true);

    // Create actions for the toolbar, menu bar and tray/dock icon
    // Needs walletFrame to be initialized
    createActions();

    // Create application menu bar
    createMenuBar();

    // Create the toolbars
    createToolBars();

    // Create the title bar
    createTitleBars();

    // Create system tray icon and notification
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        createTrayIcon();
    }
    notificator = new Notificator(QApplication::applicationName(), trayIcon, this);

    // Create status bar
    statusBar();

    // Enable the size grip (right size grip), add new size grip (left size grip) and set the status bar style
    statusBar()->setSizeGripEnabled(true);
    statusBar()->addWidget(new QSizeGrip(statusBar()));
    statusBar()->setStyleSheet("QSizeGrip { width: 3px; height: 25px; border: 0px solid black; } \n QStatusBar::item { border: 0px solid black; }");
    // Status bar notification icons
    QFrame *frameBlocks = new QFrame();
    frameBlocks->setObjectName("frameBlocks");
    frameBlocks->setContentsMargins(0,0,0,0);
    frameBlocks->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QVBoxLayout *frameBlocksLayout = new QVBoxLayout(frameBlocks);
    QHBoxLayout *hLayIcons = new QHBoxLayout();
    frameBlocksLayout->setContentsMargins(0,20,0,20);
    frameBlocksLayout->setSpacing(10);
    unitDisplayControl = new UnitDisplayStatusBarControl(platformStyle);
    unitDisplayControl->setObjectName("unitDisplayControl");
    labelLedgerIcon = new QLabel();
    labelWalletEncryptionIcon = new GUIUtil::ThemedLabel(platformStyle);
    labelWalletHDStatusIcon = new GUIUtil::ThemedLabel(platformStyle);
    labelProxyIcon = new GUIUtil::ClickableLabel(platformStyle);
    connectionsControl = new GUIUtil::ClickableLabel(platformStyle);
    labelBlocksIcon = new GUIUtil::ClickableLabel(platformStyle);
    labelStakingIcon = new QLabel();
    hLayIcons->addStretch();
    if(enableWallet)
    {
        QHBoxLayout *hLayUnit = new QHBoxLayout();
        hLayUnit->addStretch();
        hLayUnit->addWidget(unitDisplayControl);
        hLayUnit->addStretch();
        frameBlocksLayout->addLayout(hLayUnit);
        hLayIcons->addWidget(labelLedgerIcon);
        hLayIcons->addWidget(labelWalletEncryptionIcon);
        hLayIcons->addWidget(labelWalletHDStatusIcon);
        hLayIcons->addWidget(labelStakingIcon);
    }
    hLayIcons->addWidget(labelProxyIcon);
    hLayIcons->addWidget(connectionsControl);
    hLayIcons->addWidget(labelBlocksIcon);
    hLayIcons->addStretch();

    frameBlocksLayout->addLayout(hLayIcons);
    addDockWindows(Qt::LeftDockWidgetArea, frameBlocks);

#ifdef ENABLE_WALLET
    QTimer *timerLedgerIcon = new QTimer(labelLedgerIcon);
    connect(timerLedgerIcon, SIGNAL(timeout()), this, SLOT(updateLedgerIcon()));
    timerLedgerIcon->start(1000);

    updateLedgerIcon();

    if (gArgs.GetBoolArg("-staking", true))
    {
        timerStakingIcon = new QTimer(labelStakingIcon);
        connect(timerStakingIcon, SIGNAL(timeout()), this, SLOT(updateStakingIcon()));
        timerStakingIcon->start(1000);

        updateStakingIcon();
    }
    else
    {
        labelStakingIcon->setVisible(false);
    }
#endif // ENABLE_WALLET

    // Progress bar and label for blocks download
    progressBarLabel = new QLabel();
    progressBarLabel->setVisible(false);
    progressBar = new GUIUtil::ProgressBar();
    progressBar->setAlignment(Qt::AlignCenter);
    progressBar->setVisible(false);

    // Override style sheet for progress bar for styles that have a segmented progress bar,
    // as they make the text unreadable (workaround for issue #1071)
    // See https://doc.qt.io/qt-5/gallery.html
    QString curStyle = QApplication::style()->metaObject()->className();
    if(curStyle == "QWindowsStyle" || curStyle == "QWindowsXPStyle")
    {
        progressBar->setStyleSheet("QProgressBar { background-color: #e8e8e8; border: 1px solid grey; border-radius: 7px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #FF8000, stop: 1 orange); border-radius: 7px; margin: 0px; }");
    }

    statusBar()->addWidget(progressBarLabel);
    statusBar()->addWidget(progressBar);

    // Install event filter to be able to catch status tip events (QEvent::StatusTip)
    this->installEventFilter(this);

    // Initially wallet actions should be disabled
    setWalletActionsEnabled(false);

    // Subscribe to notifications from core
    subscribeToCoreSignals();

    connect(labelProxyIcon, &GUIUtil::ClickableLabel::clicked, [this] {
        openOptionsDialogWithTab(OptionsDialog::TAB_NETWORK);
    });

    modalOverlay = new ModalOverlay(enableWallet, this);
    qtumVersionChecker = new QtumVersionChecker(this);

    connect(labelBlocksIcon, &GUIUtil::ClickableLabel::clicked, this, &BitcoinGUI::showModalOverlay);
    connect(progressBar, &GUIUtil::ClickableProgressBar::clicked, this, &BitcoinGUI::showModalOverlay);

#ifdef ENABLE_WALLET
    if(enableWallet) {
        modalBackupOverlay = new ModalOverlay(enableWallet, this, ModalOverlay::Backup);
        connect(modalBackupOverlay, SIGNAL(backupWallet()), walletFrame, SLOT(backupWallet()));
        connect(m_wallet_selector, SIGNAL(currentIndexChanged(int)), rpcConsole, SLOT(activeWalletChanged(int)));
    }
#endif

    setStyleSheet("QMainWindow::separator { width: 1px; height: 1px; margin: 0px; padding: 0px; }");

#ifdef Q_OS_MAC
    m_app_nap_inhibitor = new CAppNapInhibitor;
#endif

    // Show the backup overlay screen the first time the application is started
    if(enableWallet && settings.value("ShowBackupOverlay", true).toBool())
    {
        showModalBackupOverlay();
        settings.setValue("ShowBackupOverlay", false);
    }

    GUIUtil::handleCloseWindowShortcut(this);
}

BitcoinGUI::~BitcoinGUI()
{
    // Unsubscribe from notifications from core
    unsubscribeFromCoreSignals();

    QSettings settings;
    settings.setValue("MainWindowGeometry", saveGeometry());
    if(trayIcon) // Hide tray icon, as deleting will let it linger until quit (on Ubuntu)
        trayIcon->hide();
#ifdef Q_OS_MAC
    delete m_app_nap_inhibitor;
    delete appMenuBar;
    MacDockIconHandler::cleanup();
#endif

    delete rpcConsole;
}

void BitcoinGUI::createActions()
{
    QActionGroup *tabGroup = new QActionGroup(this);
    connect(modalOverlay, &ModalOverlay::triggered, tabGroup, &QActionGroup::setEnabled);

    overviewAction = new QAction(platformStyle->MultiStatesIcon(":/icons/overview"), tr("My &wallet"), this);
    overviewAction->setStatusTip(tr("Show general overview of wallet"));
    overviewAction->setToolTip(overviewAction->statusTip());
    overviewAction->setCheckable(true);
    overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_1));
    tabGroup->addAction(overviewAction);

    sendCoinsAction = new QAction(platformStyle->MultiStatesIcon(":/icons/send_to"), tr("&Send"), this);
    sendCoinsAction->setStatusTip(tr("Send coins to a Qtum address"));
    sendCoinsAction->setToolTip(sendCoinsAction->statusTip());
    sendCoinsAction->setCheckable(true);
    tabGroup->addAction(sendCoinsAction);

    receiveCoinsAction = new QAction(platformStyle->MultiStatesIcon(":/icons/receive_from"), tr("&Receive"), this);
    receiveCoinsAction->setStatusTip(tr("Request payments (generates QR codes and qtum: URIs)"));
    receiveCoinsAction->setToolTip(receiveCoinsAction->statusTip());
    receiveCoinsAction->setCheckable(true);
    tabGroup->addAction(receiveCoinsAction);

    historyAction = new QAction(platformStyle->MultiStatesIcon(":/icons/history"), tr("&Transactions"), this);
    historyAction->setStatusTip(tr("Browse transaction history"));
    historyAction->setToolTip(historyAction->statusTip());
    historyAction->setCheckable(true);
    historyAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));
    tabGroup->addAction(historyAction);

    smartContractAction = new QAction(platformStyle->MultiStatesIcon(":/icons/smart_contract"), tr("Smart &Contracts"), this);
    smartContractAction->setStatusTip(tr("Smart contracts"));
    smartContractAction->setToolTip(smartContractAction->statusTip());
    smartContractAction->setCheckable(true);
    smartContractAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_3));
    tabGroup->addAction(smartContractAction);

    createContractAction = new QAction(tr("Create"), this);
    sendToContractAction = new QAction(tr("Send To"), this);
    callContractAction = new QAction(tr("Call"), this);

    walletStakeAction = new QAction(platformStyle->MultiStatesIcon(":/icons/tx_mined"), tr("&Stake"), this);
    walletStakeAction->setStatusTip(tr("Show stake of wallet"));
    walletStakeAction->setToolTip(walletStakeAction->statusTip());
    walletStakeAction->setCheckable(true);
    walletStakeAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_4));
    tabGroup->addAction(walletStakeAction);

    stakeAction = new QAction(tr("Staking"), this);
    delegationAction = new QAction(tr("Delegations"), this);
    superStakerAction = new QAction(tr("Super Staking"), this);

    QRCTokenAction = new QAction(platformStyle->MultiStatesIcon(":/icons/qrctoken"), tr("&QRC Tokens"), this);
    QRCTokenAction->setStatusTip(tr("QRC Tokens (send, receive or add Tokens in list)"));
    QRCTokenAction->setToolTip(QRCTokenAction->statusTip());
    QRCTokenAction->setCheckable(true);
    QRCTokenAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_5));
    tabGroup->addAction(QRCTokenAction);

#ifdef ENABLE_WALLET
    // These showNormalIfMinimized are needed because Send Coins and Receive Coins
    // can be triggered from the tray menu, and need to show the GUI to be useful.
    connect(overviewAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(overviewAction, &QAction::triggered, this, &BitcoinGUI::gotoOverviewPage);
    connect(sendCoinsAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(sendCoinsAction, &QAction::triggered, [this]{ gotoSendCoinsPage(); });
    connect(receiveCoinsAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(receiveCoinsAction, &QAction::triggered, this, &BitcoinGUI::gotoReceiveCoinsPage);
    connect(historyAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(historyAction, &QAction::triggered, this, &BitcoinGUI::gotoHistoryPage);

    connect(createContractAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(createContractAction, SIGNAL(triggered()), this, SLOT(gotoCreateContractPage()));
    connect(sendToContractAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendToContractAction, SIGNAL(triggered()), this, SLOT(gotoSendToContractPage()));
    connect(callContractAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(callContractAction, SIGNAL(triggered()), this, SLOT(gotoCallContractPage()));
    connect(QRCTokenAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(QRCTokenAction, SIGNAL(triggered()), this, SLOT(gotoTokenPage()));
    connect(stakeAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(stakeAction, &QAction::triggered, this, &BitcoinGUI::gotoStakePage);
    connect(delegationAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(delegationAction, SIGNAL(triggered()), this, SLOT(gotoDelegationPage()));
    connect(superStakerAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(superStakerAction, SIGNAL(triggered()), this, SLOT(gotoSuperStakerPage()));

#endif // ENABLE_WALLET

    quitAction = new QAction(tr("E&xit"), this);
    quitAction->setStatusTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);
    aboutAction = new QAction(tr("&About %1").arg(PACKAGE_NAME), this);
    aboutAction->setStatusTip(tr("Show information about %1").arg(PACKAGE_NAME));
    aboutAction->setMenuRole(QAction::AboutRole);
    aboutAction->setEnabled(false);
    aboutQtAction = new QAction(tr("About &Qt"), this);
    aboutQtAction->setStatusTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    optionsAction = new QAction(tr("&Options…"), this);
    optionsAction->setStatusTip(tr("Modify configuration options for %1").arg(PACKAGE_NAME));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    optionsAction->setEnabled(false);

    encryptWalletAction = new QAction(tr("&Encrypt Wallet…"), this);
    encryptWalletAction->setStatusTip(tr("Encrypt the private keys that belong to your wallet"));
    encryptWalletAction->setCheckable(true);
    backupWalletAction = new QAction(tr("&Backup Wallet…"), this);
    backupWalletAction->setStatusTip(tr("Backup wallet to another location"));

    restoreWalletAction = new QAction(tr("&Restore Wallet..."), this);
    restoreWalletAction->setStatusTip(tr("Restore wallet from another location"));

    changePassphraseAction = new QAction(tr("&Change Passphrase…"), this);
    changePassphraseAction->setStatusTip(tr("Change the passphrase used for wallet encryption"));

    unlockWalletAction = new QAction(tr("&Unlock Wallet..."), this);
    unlockWalletAction->setToolTip(tr("Unlock wallet"));
    unlockWalletAction->setObjectName("unlockWalletAction");
    lockWalletAction = new QAction(tr("&Lock Wallet"), this);
    lockWalletAction->setToolTip(tr("Lock wallet"));

    signMessageAction = new QAction(tr("Sign &message…"), this);
    signMessageAction->setStatusTip(tr("Sign messages with your Qtum addresses to prove you own them"));
    verifyMessageAction = new QAction(tr("&Verify message…"), this);
    verifyMessageAction->setStatusTip(tr("Verify messages to ensure they were signed with specified Qtum addresses"));
    m_load_psbt_action = new QAction(tr("&Load PSBT from file…"), this);
    m_load_psbt_action->setStatusTip(tr("Load Partially Signed Qtum Transaction"));
    m_load_psbt_clipboard_action = new QAction(tr("Load PSBT from &clipboard…"), this);
    m_load_psbt_clipboard_action->setStatusTip(tr("Load Partially Signed Qtum Transaction from clipboard"));
    signTxHardwareAction = new QAction(tr("Sign with &hardware..."), this);
    signTxHardwareAction->setStatusTip(tr("Sign transaction with hardware wallet"));

    openRPCConsoleAction = new QAction(tr("Node window"), this);
    openRPCConsoleAction->setStatusTip(tr("Open node debugging and diagnostic console"));
    // initially disable the debug window menu item
    openRPCConsoleAction->setEnabled(false);
    openRPCConsoleAction->setObjectName("openRPCConsoleAction");

    usedSendingAddressesAction = new QAction(tr("&Sending addresses"), this);
    usedSendingAddressesAction->setStatusTip(tr("Show the list of used sending addresses and labels"));
    usedReceivingAddressesAction = new QAction(tr("&Receiving addresses"), this);
    usedReceivingAddressesAction->setStatusTip(tr("Show the list of used receiving addresses and labels"));

    openAction = new QAction(tr("Open &URI…"), this);
    openAction->setStatusTip(tr("Open a qtum: URI"));

    m_open_wallet_action = new QAction(tr("Open Wallet"), this);
    m_open_wallet_action->setEnabled(false);
    m_open_wallet_action->setStatusTip(tr("Open a wallet"));
    m_open_wallet_menu = new QMenu(this);

    m_close_wallet_action = new QAction(tr("Close Wallet…"), this);
    m_close_wallet_action->setStatusTip(tr("Close wallet"));

    m_create_wallet_action = new QAction(tr("Create Wallet…"), this);
    m_create_wallet_action->setEnabled(false);
    m_create_wallet_action->setStatusTip(tr("Create a new wallet"));

    m_close_all_wallets_action = new QAction(tr("Close All Wallets…"), this);
    m_close_all_wallets_action->setStatusTip(tr("Close all wallets"));

    showHelpMessageAction = new QAction(tr("&Command-line options"), this);
    showHelpMessageAction->setMenuRole(QAction::NoRole);
    showHelpMessageAction->setStatusTip(tr("Show the %1 help message to get a list with possible Qtum command-line options").arg(PACKAGE_NAME));

    m_mask_values_action = new QAction(tr("&Mask values"), this);
    m_mask_values_action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M));
    m_mask_values_action->setStatusTip(tr("Mask the values in the Overview tab"));
    m_mask_values_action->setCheckable(true);

    connect(quitAction, &QAction::triggered, this, &BitcoinGUI::quitRequested);
    connect(aboutAction, &QAction::triggered, this, &BitcoinGUI::aboutClicked);
    connect(aboutQtAction, &QAction::triggered, qApp, QApplication::aboutQt);
    connect(optionsAction, &QAction::triggered, this, &BitcoinGUI::optionsClicked);
    connect(showHelpMessageAction, &QAction::triggered, this, &BitcoinGUI::showHelpMessageClicked);
    connect(openRPCConsoleAction, &QAction::triggered, this, &BitcoinGUI::showDebugWindow);
    // prevents an open debug window from becoming stuck/unusable on client shutdown
    connect(quitAction, &QAction::triggered, rpcConsole, &QWidget::hide);

#ifdef ENABLE_WALLET
    if(walletFrame)
    {
        connect(encryptWalletAction, &QAction::triggered, walletFrame, &WalletFrame::encryptWallet);
        connect(backupWalletAction, &QAction::triggered, walletFrame, &WalletFrame::backupWallet);
        connect(restoreWalletAction, SIGNAL(triggered()), walletFrame, SLOT(restoreWallet()));
        connect(changePassphraseAction, &QAction::triggered, walletFrame, &WalletFrame::changePassphrase);
        connect(unlockWalletAction, SIGNAL(triggered()), walletFrame, SLOT(unlockWallet()));
        connect(lockWalletAction, SIGNAL(triggered()), walletFrame, SLOT(lockWallet()));
        connect(signMessageAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
        connect(signMessageAction, &QAction::triggered, [this]{ gotoSignMessageTab(); });
        connect(m_load_psbt_action, &QAction::triggered, [this]{ gotoLoadPSBT(); });
        connect(m_load_psbt_clipboard_action, &QAction::triggered, [this]{ gotoLoadPSBT(true); });
        connect(verifyMessageAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
        connect(verifyMessageAction, &QAction::triggered, [this]{ gotoVerifyMessageTab(); });
        connect(usedSendingAddressesAction, &QAction::triggered, walletFrame, &WalletFrame::usedSendingAddresses);
        connect(usedReceivingAddressesAction, &QAction::triggered, walletFrame, &WalletFrame::usedReceivingAddresses);
        connect(signTxHardwareAction, &QAction::triggered, [this]{ signTxHardware(); });
        connect(openAction, &QAction::triggered, this, &BitcoinGUI::openClicked);
        connect(m_open_wallet_menu, &QMenu::aboutToShow, [this] {
            m_open_wallet_menu->clear();
            for (const std::pair<const std::string, bool>& i : m_wallet_controller->listWalletDir()) {
                const std::string& path = i.first;
                QString name = path.empty() ? QString("["+tr("default wallet")+"]") : QString::fromStdString(path);
                // Menu items remove single &. Single & are shown when && is in
                // the string, but only the first occurrence. So replace only
                // the first & with &&.
                name.replace(name.indexOf(QChar('&')), 1, QString("&&"));
                QAction* action = m_open_wallet_menu->addAction(name);

                if (i.second) {
                    // This wallet is already loaded
                    action->setEnabled(false);
                    continue;
                }

                connect(action, &QAction::triggered, [this, path] {
                    auto activity = new OpenWalletActivity(m_wallet_controller, this);
                    connect(activity, &OpenWalletActivity::opened, this, &BitcoinGUI::setCurrentWallet);
                    activity->open(path);
                });
            }
            if (m_open_wallet_menu->isEmpty()) {
                QAction* action = m_open_wallet_menu->addAction(tr("No wallets available"));
                action->setEnabled(false);
            }
        });
        connect(m_close_wallet_action, &QAction::triggered, [this] {
            m_wallet_controller->closeWallet(walletFrame->currentWalletModel(), this);
        });
        connect(m_create_wallet_action, &QAction::triggered, [this] {
            auto activity = new CreateWalletActivity(m_wallet_controller, this);
            connect(activity, &CreateWalletActivity::created, this, &BitcoinGUI::setCurrentWallet);
            activity->create();
        });
        connect(m_close_all_wallets_action, &QAction::triggered, [this] {
            m_wallet_controller->closeAllWallets(this);
        });
        connect(m_mask_values_action, &QAction::toggled, this, &BitcoinGUI::setPrivacy);
    }
#endif // ENABLE_WALLET

    connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C), this), &QShortcut::activated, this, &BitcoinGUI::showDebugWindowActivateConsole);
    connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_D), this), &QShortcut::activated, this, &BitcoinGUI::showDebugWindow);
}

void BitcoinGUI::createMenuBar()
{
#ifdef Q_OS_MAC
    // Create a decoupled menu bar on Mac which stays even if the window is closed
    appMenuBar = new QMenuBar();
#else
    // Get the main window's menu bar on other platforms
    appMenuBar = menuBar();
#endif

    // Configure the menus
    QMenu *file = appMenuBar->addMenu(tr("&File"));
    if(walletFrame)
    {
        file->addAction(m_create_wallet_action);
        file->addAction(m_open_wallet_action);
        file->addAction(m_close_wallet_action);
        file->addAction(m_close_all_wallets_action);
        file->addSeparator();
        file->addAction(openAction);
        file->addAction(backupWalletAction);
        file->addAction(restoreWalletAction);
        file->addAction(signMessageAction);
        file->addAction(verifyMessageAction);
        file->addAction(m_load_psbt_action);
        file->addAction(m_load_psbt_clipboard_action);

        if(::Params().HasHardwareWalletSupport())
        {
            file->addAction(signTxHardwareAction);
        }

        file->addSeparator();
    }
    file->addAction(quitAction);

    QMenu *settings = appMenuBar->addMenu(tr("&Settings"));
    if(walletFrame)
    {
        settings->addAction(encryptWalletAction);
        settings->addAction(changePassphraseAction);
        settings->addAction(unlockWalletAction);
        settings->addAction(lockWalletAction);
        settings->addSeparator();
        settings->addAction(m_mask_values_action);
        settings->addSeparator();
    }
    settings->addAction(optionsAction);

    QMenu* window_menu = appMenuBar->addMenu(tr("&Window"));

    QAction* minimize_action = window_menu->addAction(tr("&Minimize"));
    minimize_action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_M));
    connect(minimize_action, &QAction::triggered, [] {
        QApplication::activeWindow()->showMinimized();
    });
    connect(qApp, &QApplication::focusWindowChanged, [minimize_action] (QWindow* window) {
        minimize_action->setEnabled(window != nullptr && (window->flags() & Qt::Dialog) != Qt::Dialog && window->windowState() != Qt::WindowMinimized);
    });

#ifdef Q_OS_MAC
    QAction* zoom_action = window_menu->addAction(tr("Zoom"));
    connect(zoom_action, &QAction::triggered, [] {
        QWindow* window = qApp->focusWindow();
        if (window->windowState() != Qt::WindowMaximized) {
            window->showMaximized();
        } else {
            window->showNormal();
        }
    });

    connect(qApp, &QApplication::focusWindowChanged, [zoom_action] (QWindow* window) {
        zoom_action->setEnabled(window != nullptr);
    });
#endif

    if (walletFrame) {
#ifdef Q_OS_MAC
        window_menu->addSeparator();
        QAction* main_window_action = window_menu->addAction(tr("Main Window"));
        connect(main_window_action, &QAction::triggered, [this] {
            GUIUtil::bringToFront(this);
        });
#endif
        window_menu->addSeparator();
        window_menu->addAction(usedSendingAddressesAction);
        window_menu->addAction(usedReceivingAddressesAction);
    }

    window_menu->addSeparator();
    for (RPCConsole::TabTypes tab_type : rpcConsole->tabs()) {
        QAction* tab_action = window_menu->addAction(rpcConsole->tabTitle(tab_type));
        tab_action->setShortcut(rpcConsole->tabShortcut(tab_type));
        connect(tab_action, &QAction::triggered, [this, tab_type] {
            rpcConsole->setTabFocus(tab_type);
            showDebugWindow();
        });
    }

    QMenu *help = appMenuBar->addMenu(tr("&Help"));
    help->addAction(showHelpMessageAction);
    help->addSeparator();
    help->addAction(aboutAction);
    help->addAction(aboutQtAction);
}

void BitcoinGUI::createToolBars()
{
    if(walletFrame)
    {
        // Create custom tool bar component
        appNavigationBar = new NavigationBar();
        addDockWindows(Qt::LeftDockWidgetArea, appNavigationBar);

        // Fill the component with actions
        appNavigationBar->addAction(overviewAction);
        appNavigationBar->addAction(historyAction);
        QList<QAction*> contractActions;
        contractActions.append(createContractAction);
        contractActions.append(sendToContractAction);
        contractActions.append(callContractAction);
        appNavigationBar->mapGroup(smartContractAction, contractActions);
        QList<QAction*> walletStakeActions;
        walletStakeActions.append(stakeAction);
        walletStakeActions.append(delegationAction);
        walletStakeActions.append(superStakerAction);
        appNavigationBar->mapGroup(walletStakeAction, walletStakeActions);
        appNavigationBar->addAction(QRCTokenAction);
        appNavigationBar->buildUi();
        overviewAction->setChecked(true);
    }
}

void BitcoinGUI::createTitleBars()
{
#ifdef ENABLE_WALLET
    if(walletFrame)
    {
        // Create wallet selector
        m_wallet_selector = new QComboBox();
        m_wallet_selector->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        connect(m_wallet_selector, qOverload<int>(&QComboBox::currentIndexChanged), this, &BitcoinGUI::setCurrentWalletBySelectorIndex);

        m_wallet_selector_label = new QLabel();
        m_wallet_selector_label->setText(tr("Wallet:") + " ");
        m_wallet_selector_label->setBuddy(m_wallet_selector);

        m_wallet_selector_label->setVisible(false);
        m_wallet_selector->setVisible(false);

        // Create custom title bar component
        appTitleBar = new TitleBar(platformStyle);
        addDockWindows(Qt::TopDockWidgetArea, appTitleBar);
        appTitleBar->setWalletSelector( m_wallet_selector_label, m_wallet_selector);
        walletFrame->setTitleBar(appTitleBar);
        connect(appNavigationBar, SIGNAL(resized(QSize)), appTitleBar, SLOT(on_navigationResized(QSize)));
    }
#endif // ENABLE_WALLET
}

void BitcoinGUI::setClientModel(ClientModel *_clientModel, interfaces::BlockAndHeaderTipInfo* tip_info)
{
    this->clientModel = _clientModel;
    if(_clientModel)
    {

        // Check for updates
        if(_clientModel->getOptionsModel()->getCheckForUpdates() && qtumVersionChecker->newVersionAvailable())
        {
            QString link = QString("<a href=%1>%2</a>").arg(QTUM_RELEASES, QTUM_RELEASES);
            QString message(tr("New version of Qtum wallet is available on the Qtum source code repository: <br /> %1. <br />It is recommended to download it and update this application").arg(link));
            QMessageBox::information(this, tr("Check for updates"), message);
        }

        // Create system tray menu (or setup the dock menu) that late to prevent users from calling actions,
        // while the client has not yet fully loaded
        createTrayIconMenu();

        // Keep up to date with client
        setNetworkActive(m_node.getNetworkActive());
        connect(connectionsControl, &GUIUtil::ClickableLabel::clicked, [this] {
            GUIUtil::PopupMenu(m_network_context_menu, QCursor::pos());
        });
        connect(_clientModel, &ClientModel::numConnectionsChanged, this, &BitcoinGUI::setNumConnections);
        connect(_clientModel, &ClientModel::networkActiveChanged, this, &BitcoinGUI::setNetworkActive);

        modalOverlay->setKnownBestHeight(tip_info->header_height, QDateTime::fromSecsSinceEpoch(tip_info->header_time));
        setNumBlocks(tip_info->block_height, QDateTime::fromSecsSinceEpoch(tip_info->block_time), tip_info->verification_progress, false, SynchronizationState::INIT_DOWNLOAD);
        connect(_clientModel, &ClientModel::numBlocksChanged, this, &BitcoinGUI::setNumBlocks);

        // Receive and report messages from client model
        connect(_clientModel, &ClientModel::message, [this](const QString &title, const QString &message, unsigned int style){
            this->message(title, message, style);
        });

        // Show progress dialog
        connect(_clientModel, &ClientModel::showProgress, this, &BitcoinGUI::showProgress);

        rpcConsole->setClientModel(_clientModel, tip_info->block_height, tip_info->block_time, tip_info->verification_progress);

        updateProxyIcon();

#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            walletFrame->setClientModel(_clientModel);
        }
#endif // ENABLE_WALLET
        unitDisplayControl->setOptionsModel(_clientModel->getOptionsModel());

        OptionsModel* optionsModel = _clientModel->getOptionsModel();
        if (optionsModel && trayIcon) {
            // be aware of the tray icon disable state change reported by the OptionsModel object.
            connect(optionsModel, &OptionsModel::showTrayIconChanged, trayIcon, &QSystemTrayIcon::setVisible);

            // initialize the disable state of the tray icon with the current value in the model.
            trayIcon->setVisible(optionsModel->getShowTrayIcon());
        }

#ifdef ENABLE_WALLET
        if (optionsModel && appTitleBar) {
            connect(optionsModel, &OptionsModel::displayUnitChanged, appTitleBar, &TitleBar::updateDisplayUnit);
        }
#endif // ENABLE_WALLET

        modalOverlay->setKnownBestHeight(clientModel->getHeaderTipHeight(), QDateTime::fromTime_t(clientModel->getHeaderTipTime()));

    } else {
        if(trayIconMenu)
        {
            // Disable context menu on tray icon
            trayIconMenu->clear();
        }
        // Propagate cleared model to child objects
        rpcConsole->setClientModel(nullptr);
#ifdef ENABLE_WALLET
        if (walletFrame)
        {
            walletFrame->setClientModel(nullptr);
        }
#endif // ENABLE_WALLET
        unitDisplayControl->setOptionsModel(nullptr);
    }
}

#ifdef ENABLE_WALLET
void BitcoinGUI::setWalletController(WalletController* wallet_controller)
{
    assert(!m_wallet_controller);
    assert(wallet_controller);

    m_wallet_controller = wallet_controller;

    m_create_wallet_action->setEnabled(true);
    m_open_wallet_action->setEnabled(true);
    m_open_wallet_action->setMenu(m_open_wallet_menu);

    GUIUtil::ExceptionSafeConnect(wallet_controller, &WalletController::walletAdded, this, &BitcoinGUI::addWallet);
    connect(wallet_controller, &WalletController::walletRemoved, this, &BitcoinGUI::removeWallet);

    auto activity = new LoadWalletsActivity(m_wallet_controller, this);
    activity->load();
}

WalletController* BitcoinGUI::getWalletController()
{
    return m_wallet_controller;
}

void BitcoinGUI::addWallet(WalletModel* walletModel)
{
    if (!walletFrame) return;

    WalletView* wallet_view = new WalletView(walletModel, platformStyle, walletFrame);
    if (!walletFrame->addView(wallet_view)) return;

    rpcConsole->addWallet(walletModel);
    if (m_wallet_selector->count() == 0) {
        setWalletActionsEnabled(true);
    } else if (m_wallet_selector->count() == 1) {
        m_wallet_selector_label->setVisible(true);
        m_wallet_selector->setVisible(true);
    }

    connect(wallet_view, &WalletView::outOfSyncWarningClicked, this, &BitcoinGUI::showModalOverlay);
    connect(wallet_view, &WalletView::showMore, this, &BitcoinGUI::gotoHistoryPage);
    connect(wallet_view, &WalletView::sendCoins, this, &BitcoinGUI::gotoSendCoinsPage);
    connect(wallet_view, &WalletView::receiveCoins, this, &BitcoinGUI::gotoReceiveCoinsPage);
    connect(wallet_view, &WalletView::transactionClicked, this, &BitcoinGUI::gotoHistoryPage);
    connect(wallet_view, &WalletView::coinsSent, this, &BitcoinGUI::gotoHistoryPage);
    connect(wallet_view, &WalletView::message, [this](const QString& title, const QString& message, unsigned int style) {
        this->message(title, message, style);
    });
    connect(wallet_view, &WalletView::encryptionStatusChanged, this, &BitcoinGUI::updateWalletStatus);
    connect(wallet_view, &WalletView::incomingTransaction, this, &BitcoinGUI::incomingTransaction);
    connect(wallet_view, &WalletView::incomingTokenTransaction, this, &BitcoinGUI::incomingTokenTransaction);
    connect(wallet_view, &WalletView::currentChanged, walletFrame, &WalletFrame::pageChanged);
    connect(this, &BitcoinGUI::setPrivacy, wallet_view, &WalletView::setPrivacy);
    wallet_view->setPrivacy(isPrivacyModeActivated());
    const QString display_name = walletModel->getDisplayName();
    m_wallet_selector->addItem(display_name, QVariant::fromValue(walletModel));

    appTitleBar->addWallet(walletModel);
    if(!(clientModel->fBatchProcessingMode))
        QTimer::singleShot(MODEL_UPDATE_DELAY, clientModel, SLOT(updateTip()));

    m_wallet_selector->setCurrentIndex(m_wallet_selector->count()-1);

}

void BitcoinGUI::removeWallet(WalletModel* walletModel)
{
    if (!walletFrame) return;

    labelWalletHDStatusIcon->hide();
    labelWalletEncryptionIcon->hide();

    int index = m_wallet_selector->findData(QVariant::fromValue(walletModel));
    m_wallet_selector->removeItem(index);
    if (m_wallet_selector->count() == 0) {
        setWalletActionsEnabled(false);
        overviewAction->setChecked(true);
    } else if (m_wallet_selector->count() == 1) {
        m_wallet_selector_label->setVisible(false);
        m_wallet_selector->setVisible(false);
    }
    rpcConsole->removeWallet(walletModel);
    appTitleBar->removeWallet(walletModel);
    walletFrame->removeWallet(walletModel);
    updateWindowTitle();
}

void BitcoinGUI::setCurrentWallet(WalletModel* wallet_model)
{
    if (!walletFrame) return;
    walletFrame->setCurrentWallet(wallet_model);
    appTitleBar->setModel(wallet_model);
    walletFrame->updateTabBar();
    appTitleBar->setModel(wallet_model);
    for (int index = 0; index < m_wallet_selector->count(); ++index) {
        if (m_wallet_selector->itemData(index).value<WalletModel*>() == wallet_model) {
            m_wallet_selector->setCurrentIndex(index);
            break;
        }
    }
    updateWindowTitle();
}

void BitcoinGUI::setCurrentWalletBySelectorIndex(int index)
{
    WalletModel* wallet_model = m_wallet_selector->itemData(index).value<WalletModel*>();
    if (wallet_model) setCurrentWallet(wallet_model);
}

void BitcoinGUI::removeAllWallets()
{
    if(!walletFrame)
        return;
    setWalletActionsEnabled(false);
    walletFrame->removeAllWallets();
}
#endif // ENABLE_WALLET

void BitcoinGUI::setWalletActionsEnabled(bool enabled)
{
    overviewAction->setEnabled(enabled);
    sendCoinsAction->setEnabled(enabled);
    receiveCoinsAction->setEnabled(enabled);
    historyAction->setEnabled(enabled);
    smartContractAction->setEnabled(enabled);
    QRCTokenAction->setEnabled(enabled);
    encryptWalletAction->setEnabled(enabled);
    backupWalletAction->setEnabled(enabled);
    restoreWalletAction->setEnabled(enabled);
    changePassphraseAction->setEnabled(enabled);
    unlockWalletAction->setEnabled(enabled);
    lockWalletAction->setEnabled(enabled);
    signMessageAction->setEnabled(enabled);
    verifyMessageAction->setEnabled(enabled);
    usedSendingAddressesAction->setEnabled(enabled);
    usedReceivingAddressesAction->setEnabled(enabled);
    openAction->setEnabled(enabled);
    stakeAction->setEnabled(enabled);
    delegationAction->setEnabled(enabled);
    superStakerAction->setEnabled(enabled);
    walletStakeAction->setEnabled(enabled);
    m_load_psbt_action->setEnabled(enabled);
    m_load_psbt_clipboard_action->setEnabled(enabled);
    signTxHardwareAction->setEnabled(enabled);
    m_close_wallet_action->setEnabled(enabled);
    m_close_all_wallets_action->setEnabled(enabled);
}

void BitcoinGUI::createTrayIcon()
{
    assert(QSystemTrayIcon::isSystemTrayAvailable());

#ifndef Q_OS_MAC
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        trayIcon = new QSystemTrayIcon(m_network_style->getTrayAndWindowIcon(), this);
        QString toolTip = tr("%1 client").arg(PACKAGE_NAME) + " " + m_network_style->getTitleAddText();
        trayIcon->setToolTip(toolTip);
    }
#endif
}

void BitcoinGUI::createTrayIconMenu()
{
#ifndef Q_OS_MAC
    if (!trayIcon) return;
#endif // Q_OS_MAC

    // Configuration of the tray icon (or Dock icon) menu.
    QAction* show_hide_action{nullptr};
#ifndef Q_OS_MAC
    // Note: On macOS, the Dock icon's menu already has Show / Hide action.
    show_hide_action = trayIconMenu->addAction(QString(), this, &BitcoinGUI::toggleHidden);
    trayIconMenu->addSeparator();
#endif // Q_OS_MAC

    QAction* send_action{nullptr};
    QAction* receive_action{nullptr};
    QAction* sign_action{nullptr};
    QAction* verify_action{nullptr};
    if (enableWallet) {
        send_action = trayIconMenu->addAction(sendCoinsAction->text(), sendCoinsAction, &QAction::trigger);
        receive_action = trayIconMenu->addAction(receiveCoinsAction->text(), receiveCoinsAction, &QAction::trigger);
        trayIconMenu->addSeparator();
        sign_action = trayIconMenu->addAction(signMessageAction->text(), signMessageAction, &QAction::trigger);
        verify_action = trayIconMenu->addAction(verifyMessageAction->text(), verifyMessageAction, &QAction::trigger);
        trayIconMenu->addSeparator();
    }
    QAction* options_action = trayIconMenu->addAction(optionsAction->text(), optionsAction, &QAction::trigger);
    options_action->setMenuRole(QAction::PreferencesRole);
    QAction* node_window_action = trayIconMenu->addAction(openRPCConsoleAction->text(), openRPCConsoleAction, &QAction::trigger);
    QAction* quit_action{nullptr};
#ifndef Q_OS_MAC
    // Note: On macOS, the Dock icon's menu already has Quit action.
    trayIconMenu->addSeparator();
    quit_action = trayIconMenu->addAction(quitAction->text(), quitAction, &QAction::trigger);

    trayIcon->setContextMenu(trayIconMenu.get());
    connect(trayIcon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            // Click on system tray icon triggers show/hide of the main window
            toggleHidden();
        }
    });
#else
    // Note: On macOS, the Dock icon is used to provide the tray's functionality.
    MacDockIconHandler* dockIconHandler = MacDockIconHandler::instance();
    connect(dockIconHandler, &MacDockIconHandler::dockIconClicked, [this] {
        show();
        activateWindow();
    });
    trayIconMenu->setAsDockMenu();
#endif // Q_OS_MAC

    connect(
        // Using QSystemTrayIcon::Context is not reliable.
        // See https://bugreports.qt.io/browse/QTBUG-91697
        trayIconMenu.get(), &QMenu::aboutToShow,
        [this, show_hide_action, send_action, receive_action, sign_action, verify_action, options_action, node_window_action, quit_action] {
            if (show_hide_action) show_hide_action->setText(
                (!isHidden() && !isMinimized() && !GUIUtil::isObscured(this)) ?
                    tr("&Hide") :
                    tr("S&how"));
            if (QApplication::activeModalWidget()) {
                for (QAction* a : trayIconMenu.get()->actions()) {
                    a->setEnabled(false);
                }
            } else {
                if (show_hide_action) show_hide_action->setEnabled(true);
                if (enableWallet) {
                    send_action->setEnabled(sendCoinsAction->isEnabled());
                    receive_action->setEnabled(receiveCoinsAction->isEnabled());
                    sign_action->setEnabled(signMessageAction->isEnabled());
                    verify_action->setEnabled(verifyMessageAction->isEnabled());
                }
                options_action->setEnabled(optionsAction->isEnabled());
                node_window_action->setEnabled(openRPCConsoleAction->isEnabled());
                if (quit_action) quit_action->setEnabled(true);
            }
        });
}

void BitcoinGUI::optionsClicked()
{
    openOptionsDialogWithTab(OptionsDialog::TAB_MAIN);
}

void BitcoinGUI::aboutClicked()
{
    if(!clientModel)
        return;

    auto dlg = new HelpMessageDialog(this, /* about */ true);
    GUIUtil::ShowModalDialogAsynchronously(dlg);
}

void BitcoinGUI::showDebugWindow()
{
    GUIUtil::bringToFront(rpcConsole);
    Q_EMIT consoleShown(rpcConsole);
}

void BitcoinGUI::showDebugWindowActivateConsole()
{
    rpcConsole->setTabFocus(RPCConsole::TabTypes::CONSOLE);
    showDebugWindow();
}

void BitcoinGUI::showHelpMessageClicked()
{
    GUIUtil::bringToFront(helpMessageDialog);
}

#ifdef ENABLE_WALLET
void BitcoinGUI::openClicked()
{
    OpenURIDialog dlg(platformStyle, this);
    if(dlg.exec())
    {
        Q_EMIT receivedURI(dlg.getURI());
    }
}

void BitcoinGUI::gotoOverviewPage()
{
    overviewAction->setChecked(true);
    if (walletFrame) walletFrame->gotoOverviewPage();
}

void BitcoinGUI::gotoHistoryPage()
{
    historyAction->setChecked(true);
    if (walletFrame) walletFrame->gotoHistoryPage();
}

void BitcoinGUI::gotoTokenPage()
{
    QRCTokenAction->setChecked(true);
    if (walletFrame) walletFrame->gotoTokenPage();
}

void BitcoinGUI::gotoDelegationPage()
{
    delegationAction->setChecked(true);
    if (walletFrame) walletFrame->gotoDelegationPage();
}

void BitcoinGUI::gotoSuperStakerPage()
{
    superStakerAction->setChecked(true);
    if (walletFrame) walletFrame->gotoSuperStakerPage();
}

void BitcoinGUI::gotoReceiveCoinsPage()
{
    receiveCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoReceiveCoinsPage();
}

void BitcoinGUI::gotoSendCoinsPage(QString addr)
{
    sendCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoSendCoinsPage(addr);
}

void BitcoinGUI::gotoCreateContractPage()
{
    if (walletFrame) walletFrame->gotoCreateContractPage();
}

void BitcoinGUI::gotoSendToContractPage()
{
    if (walletFrame) walletFrame->gotoSendToContractPage();
}

void BitcoinGUI::gotoCallContractPage()
{
    if (walletFrame) walletFrame->gotoCallContractPage();
}

void BitcoinGUI::gotoStakePage()
{
    stakeAction->setChecked(true);
    if (walletFrame) walletFrame->gotoStakePage();
}

void BitcoinGUI::gotoSignMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoSignMessageTab(addr);
}

void BitcoinGUI::gotoVerifyMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoVerifyMessageTab(addr);
}
void BitcoinGUI::gotoLoadPSBT(bool from_clipboard)
{
    if (walletFrame) walletFrame->gotoLoadPSBT(from_clipboard);
}

void BitcoinGUI::signTxHardware(const QString& tx)
{
    if (walletFrame) walletFrame->signTxHardware(tx);
}

#endif // ENABLE_WALLET

void BitcoinGUI::updateNetworkState()
{
    int count = clientModel->getNumConnections();
    QString icon;
    switch(count)
    {
    case 0: icon = ":/icons/connect_0"; break;
    case 1: case 2: case 3: icon = ":/icons/connect_1"; break;
    case 4: case 5: case 6: icon = ":/icons/connect_2"; break;
    case 7: case 8: case 9: icon = ":/icons/connect_3"; break;
    default: icon = ":/icons/connect_4"; break;
    }

    QString tooltip;

    if (m_node.getNetworkActive()) {
        //: A substring of the tooltip.
        tooltip = tr("%n active connection(s) to Qtum network.", "", count);
    } else {
        //: A substring of the tooltip.
        tooltip = tr("Network activity disabled.");
        icon = ":/icons/network_disabled";
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QLatin1String("<nobr>") + tooltip + QLatin1String("<br>") +
              //: A substring of the tooltip. "More actions" are available via the context menu.
              tr("Click for more actions.") + QLatin1String("</nobr>");
    connectionsControl->setToolTip(tooltip);

    connectionsControl->setPixmap(platformStyle->MultiStatesIcon(icon).pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
}

void BitcoinGUI::setNumConnections(int count)
{
    updateNetworkState();
}

void BitcoinGUI::setNetworkActive(bool network_active)
{
    updateNetworkState();
    m_network_context_menu->clear();
    m_network_context_menu->addAction(
        //: A context menu item. The "Peers tab" is an element of the "Node window".
        tr("Show Peers tab"),
        [this] {
            rpcConsole->setTabFocus(RPCConsole::TabTypes::PEERS);
            showDebugWindow();
        });
    m_network_context_menu->addAction(
        network_active ?
            //: A context menu item.
            tr("Disable network activity") :
            //: A context menu item. The network activity was disabled previously.
            tr("Enable network activity"),
        [this, new_state = !network_active] { m_node.setNetworkActive(new_state); });
}

void BitcoinGUI::updateHeadersSyncProgressLabel()
{
    int64_t headersTipTime = clientModel->getHeaderTipTime();
    int headersTipHeight = clientModel->getHeaderTipHeight();
    int estHeadersLeft = GUIUtil::estimateNumberHeadersLeft(GetTime() - headersTipTime ,headersTipHeight);
    if (estHeadersLeft > HEADER_HEIGHT_DELTA_SYNC)
        progressBarLabel->setText(tr("Syncing Headers (%1%)…").arg(QString::number(100.0 / (headersTipHeight+estHeadersLeft)*headersTipHeight, 'f', 1)));
}

void BitcoinGUI::openOptionsDialogWithTab(OptionsDialog::Tab tab)
{
    if (!clientModel || !clientModel->getOptionsModel())
        return;

    auto dlg = new OptionsDialog(this, enableWallet);
    connect(dlg, &OptionsDialog::quitOnReset, this, &BitcoinGUI::quitRequested);
    dlg->setCurrentTab(tab);
    dlg->setModel(clientModel->getOptionsModel());
    GUIUtil::ShowModalDialogAsynchronously(dlg);
}

void BitcoinGUI::setNumBlocks(int count, const QDateTime& blockDate, double nVerificationProgress, bool header, SynchronizationState sync_state)
{
// Disabling macOS App Nap on initial sync, disk and reindex operations.
#ifdef Q_OS_MAC
    if (sync_state == SynchronizationState::POST_INIT) {
        m_app_nap_inhibitor->enableAppNap();
    } else {
        m_app_nap_inhibitor->disableAppNap();
    }
#endif

    if (modalOverlay)
    {
        if (header)
            modalOverlay->setKnownBestHeight(count, blockDate);
        else
            modalOverlay->tipUpdate(count, blockDate, nVerificationProgress);
    }
    if (!clientModel)
        return;

    // Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbled text)
    statusBar()->clearMessage();

    // Acquire current block source
    enum BlockSource blockSource = clientModel->getBlockSource();
    switch (blockSource) {
        case BlockSource::NETWORK:
            if (header) {
                updateHeadersSyncProgressLabel();
                return;
            }
            progressBarLabel->setText(tr("Synchronizing with network…"));
            updateHeadersSyncProgressLabel();
            break;
        case BlockSource::DISK:
            if (header) {
                progressBarLabel->setText(tr("Indexing blocks on disk…"));
            } else {
                progressBarLabel->setText(tr("Processing blocks on disk…"));
            }
            break;
        case BlockSource::REINDEX:
            progressBarLabel->setText(tr("Reindexing blocks on disk…"));
            break;
        case BlockSource::NONE:
            if (header) {
                return;
            }
            progressBarLabel->setText(tr("Connecting to peers…"));
            break;
    }

    QString tooltip;

    QDateTime currentDate = QDateTime::currentDateTime();
    qint64 secs = blockDate.secsTo(currentDate);

    tooltip = tr("Processed %n block(s) of transaction history.", "", count);

    // Set icon state: spinning if catching up, tick otherwise
    if (secs < MAX_BLOCK_TIME_GAP) {
        tooltip = tr("Up to date") + QString(".<br>") + tooltip;
        labelBlocksIcon->setPixmap(platformStyle->MultiStatesIcon(":/icons/synced").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));

#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            walletFrame->showOutOfSyncWarning(false);
            modalOverlay->showHide(true, true);
        }
#endif // ENABLE_WALLET

        progressBarLabel->setVisible(false);
        progressBar->setVisible(false);

        // notify tip changed when the sync is finished
        if(clientModel->fBatchProcessingMode)
        {
            clientModel->fBatchProcessingMode = false;
            QMetaObject::invokeMethod(clientModel, "tipChanged", Qt::QueuedConnection);
        }

    }
    else
    {
        QString timeBehindText = GUIUtil::formatNiceTimeOffset(secs);

        progressBarLabel->setVisible(true);
        progressBar->setFormat(tr("%1 behind").arg(timeBehindText));
        progressBar->setMaximum(1000000000);
        progressBar->setValue(nVerificationProgress * 1000000000.0 + 0.5);
        progressBar->setVisible(true);

        tooltip = tr("Catching up…") + QString("<br>") + tooltip;
        if(count != prevBlocks)
        {
            labelBlocksIcon->setPixmap(platformStyle->MultiStatesIcon(
                QString(":/animation/spinner-%1").arg(spinnerFrame, 3, 10, QChar('0')))
                .pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
            spinnerFrame = (spinnerFrame + 1) % SPINNER_FRAMES;
        }
        prevBlocks = count;

#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            walletFrame->showOutOfSyncWarning(true);
            modalOverlay->showHide();
        }
#endif // ENABLE_WALLET

        tooltip += QString("<br>");
        tooltip += tr("Last received block was generated %1 ago.").arg(timeBehindText);
        tooltip += QString("<br>");
        tooltip += tr("Transactions after this will not yet be visible.");
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}

void BitcoinGUI::message(const QString& title, QString message, unsigned int style, bool* ret, const QString& detailed_message)
{
    // Default title. On macOS, the window title is ignored (as required by the macOS Guidelines).
    QString strTitle{PACKAGE_NAME};
    // Default to information icon
    int nMBoxIcon = QMessageBox::Information;
    int nNotifyIcon = Notificator::Information;

    QString msgType;
    if (!title.isEmpty()) {
        msgType = title;
    } else {
        switch (style) {
        case CClientUIInterface::MSG_ERROR:
            msgType = tr("Error");
            message = tr("Error: %1").arg(message);
            break;
        case CClientUIInterface::MSG_WARNING:
            msgType = tr("Warning");
            message = tr("Warning: %1").arg(message);
            break;
        case CClientUIInterface::MSG_INFORMATION:
            msgType = tr("Information");
            // No need to prepend the prefix here.
            break;
        default:
            break;
        }
    }

    if (!msgType.isEmpty()) {
        strTitle += " - " + msgType;
    }

    if (style & CClientUIInterface::ICON_ERROR) {
        nMBoxIcon = QMessageBox::Critical;
        nNotifyIcon = Notificator::Critical;
    } else if (style & CClientUIInterface::ICON_WARNING) {
        nMBoxIcon = QMessageBox::Warning;
        nNotifyIcon = Notificator::Warning;
    }

    if (style & CClientUIInterface::MODAL) {
        // Check for buttons, use OK as default, if none was supplied
        QMessageBox::StandardButton buttons;
        if (!(buttons = (QMessageBox::StandardButton)(style & CClientUIInterface::BTN_MASK)))
            buttons = QMessageBox::Ok;

        showNormalIfMinimized();
        QMessageBox mBox(static_cast<QMessageBox::Icon>(nMBoxIcon), strTitle, message, buttons, this);
        mBox.setTextFormat(Qt::PlainText);
        mBox.setDetailedText(detailed_message);
        int r = mBox.exec();
        if (ret != nullptr)
            *ret = r == QMessageBox::Ok;
    } else {
        notificator->notify(static_cast<Notificator::Class>(nNotifyIcon), strTitle, message);
    }
}

void BitcoinGUI::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);

#ifndef Q_OS_MAC // Ignored on Mac
    if(e->type() == QEvent::WindowStateChange)
    {
        if(clientModel && clientModel->getOptionsModel() && clientModel->getOptionsModel()->getMinimizeToTray())
        {
            QWindowStateChangeEvent *wsevt = static_cast<QWindowStateChangeEvent*>(e);
            if(!(wsevt->oldState() & Qt::WindowMinimized) && isMinimized())
            {
                QTimer::singleShot(0, this, &BitcoinGUI::hide);
                e->ignore();
            }
            else if((wsevt->oldState() & Qt::WindowMinimized) && !isMinimized())
            {
                QTimer::singleShot(0, this, &BitcoinGUI::show);
                e->ignore();
            }
        }
    }
#endif
}

void BitcoinGUI::closeEvent(QCloseEvent *event)
{
#ifndef Q_OS_MAC // Ignored on Mac
    if(clientModel && clientModel->getOptionsModel())
    {
        if(!clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            // close rpcConsole in case it was open to make some space for the shutdown window
            rpcConsole->close();

            Q_EMIT quitRequested();
        }
        else
        {
            QMainWindow::showMinimized();
            event->ignore();
        }
    }
#else
    QMainWindow::closeEvent(event);
#endif
}

void BitcoinGUI::showEvent(QShowEvent *event)
{
    // enable the debug window when the main window shows up
    openRPCConsoleAction->setEnabled(true);
    aboutAction->setEnabled(true);
    optionsAction->setEnabled(true);
}

#ifdef ENABLE_WALLET
void BitcoinGUI::incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address, const QString& label, const QString& walletName)
{
    // On new transaction, make an info balloon
    QString msg = tr("Date: %1\n").arg(date) +
                  tr("Amount: %1\n").arg(BitcoinUnits::formatWithUnit(unit, amount, true));
    if (m_node.walletLoader().getWallets().size() > 1 && !walletName.isEmpty()) {
        msg += tr("Wallet: %1\n").arg(walletName);
    }
    msg += tr("Type: %1\n").arg(type);
    if (!label.isEmpty())
        msg += tr("Label: %1\n").arg(label);
    else if (!address.isEmpty())
        msg += tr("Address: %1\n").arg(address);
    message((amount)<0 ? tr("Sent transaction") : tr("Incoming transaction"),
             msg, CClientUIInterface::MSG_INFORMATION);
}

void BitcoinGUI::incomingTokenTransaction(const QString& date, const QString& amount, const QString& type, const QString& address, const QString& label, const QString& walletName, const QString& title)
{
    // On new transaction, make an info balloon
    QString msg = tr("Date: %1\n").arg(date) +
                  tr("Amount: %1\n").arg(amount);
    if (m_node.walletLoader().getWallets().size() > 1 && !walletName.isEmpty()) {
        msg += tr("Wallet: %1\n").arg(walletName);
    }
    msg += tr("Type: %1\n").arg(type);
    if (!label.isEmpty())
        msg += tr("Label: %1\n").arg(label);
    else if (!address.isEmpty())
        msg += tr("Address: %1\n").arg(address);
    message(title, msg, CClientUIInterface::MSG_INFORMATION);
}

#endif // ENABLE_WALLET

void BitcoinGUI::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept only URIs
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void BitcoinGUI::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        for (const QUrl &uri : event->mimeData()->urls())
        {
            Q_EMIT receivedURI(uri.toString());
        }
    }
    event->acceptProposedAction();
}

bool BitcoinGUI::eventFilter(QObject *object, QEvent *event)
{
    // Catch status tip events
    if (event->type() == QEvent::StatusTip)
    {
        // Prevent adding text from setStatusTip(), if we currently use the status bar for displaying other stuff
        if (progressBarLabel->isVisible() || progressBar->isVisible())
            return true;
    }
    return QMainWindow::eventFilter(object, event);
}

#ifdef ENABLE_WALLET
bool BitcoinGUI::handlePaymentRequest(const SendCoinsRecipient& recipient)
{
    // URI has to be valid
    if (walletFrame && walletFrame->handlePaymentRequest(recipient))
    {
        showNormalIfMinimized();
        gotoSendCoinsPage();
        return true;
    }
    return false;
}

void BitcoinGUI::setHDStatus(bool privkeyDisabled, int hdEnabled)
{
    labelWalletHDStatusIcon->setPixmap(platformStyle->MultiStatesIcon(privkeyDisabled ? ":/icons/eye" : hdEnabled ? ":/icons/hd_enabled" : ":/icons/hd_disabled").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
    labelWalletHDStatusIcon->setToolTip(privkeyDisabled ? tr("Private key <b>disabled</b>") : hdEnabled ? tr("HD key generation is <b>enabled</b>") : tr("HD key generation is <b>disabled</b>"));
    labelWalletHDStatusIcon->show();
}

void BitcoinGUI::setEncryptionStatus(WalletModel *walletModel)
{
    int status = walletModel->getEncryptionStatus();
    switch(status)
    {
    case WalletModel::NoKeys:
        labelWalletEncryptionIcon->hide();
        encryptWalletAction->setChecked(false);
        changePassphraseAction->setEnabled(false);
        encryptWalletAction->setEnabled(false);
        break;
    case WalletModel::Unencrypted:
        labelWalletEncryptionIcon->hide();
        encryptWalletAction->setChecked(false);
        changePassphraseAction->setEnabled(false);
        unlockWalletAction->setVisible(false);
        lockWalletAction->setVisible(false);
        encryptWalletAction->setEnabled(true);
        break;
    case WalletModel::Unlocked:
        labelWalletEncryptionIcon->show();
        if(walletModel->getWalletUnlockStakingOnly())
        {
            labelWalletEncryptionIcon->setPixmap(platformStyle->MultiStatesIcon(":/icons/lock_staking").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
            labelWalletEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked for staking only</b>"));
        }
        else
        {
            labelWalletEncryptionIcon->setPixmap(platformStyle->MultiStatesIcon(":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
            labelWalletEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        }
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(false);
        lockWalletAction->setVisible(true);
        encryptWalletAction->setEnabled(false);
        break;
    case WalletModel::Locked:
        labelWalletEncryptionIcon->show();
        labelWalletEncryptionIcon->setPixmap(platformStyle->MultiStatesIcon(":/icons/lock_closed").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelWalletEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>locked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(true);
        lockWalletAction->setVisible(false);
        encryptWalletAction->setEnabled(false);
        break;
    }
}

void BitcoinGUI::updateWalletStatus()
{
    assert(walletFrame);

    WalletView * const walletView = walletFrame->currentWalletView();
    if (!walletView) {
        return;
    }
    WalletModel * const walletModel = walletView->getWalletModel();
    setEncryptionStatus(walletModel);
    setHDStatus(walletModel->wallet().privateKeysDisabled(), walletModel->wallet().hdEnabled());
}
#endif // ENABLE_WALLET

void BitcoinGUI::updateProxyIcon()
{
    std::string ip_port;
    bool proxy_enabled = clientModel->getProxyInfo(ip_port);

    if (proxy_enabled) {
        if (!GUIUtil::HasPixmap(labelProxyIcon)) {
            QString ip_port_q = QString::fromStdString(ip_port);
            labelProxyIcon->setThemedPixmap((":/icons/proxy"), STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE);
            labelProxyIcon->setToolTip(tr("Proxy is <b>enabled</b>: %1").arg(ip_port_q));
        } else {
            labelProxyIcon->show();
        }
    } else {
        labelProxyIcon->hide();
    }
}

void BitcoinGUI::updateWindowTitle()
{
    QString window_title = PACKAGE_NAME;
#ifdef ENABLE_WALLET
    if (walletFrame) {
        WalletModel* const wallet_model = walletFrame->currentWalletModel();
        if (wallet_model && !wallet_model->getWalletName().isEmpty()) {
            window_title += " - " + wallet_model->getDisplayName();
        }
    }
#endif
    if (!m_network_style->getTitleAddText().isEmpty()) {
        window_title += " - " + m_network_style->getTitleAddText();
    }
    setWindowTitle(window_title);
}

void BitcoinGUI::showNormalIfMinimized(bool fToggleHidden)
{
    if(!clientModel)
        return;

    if (!isHidden() && !isMinimized() && !GUIUtil::isObscured(this) && fToggleHidden) {
        hide();
    } else {
        GUIUtil::bringToFront(this);
    }
}

void BitcoinGUI::toggleHidden()
{
    showNormalIfMinimized(true);
}

#ifdef ENABLE_WALLET
void BitcoinGUI::updateLedgerIcon()
{
    if(m_node.shutdownRequested() || !clientModel || clientModel->fBatchProcessingMode)
        return;

    WalletView * const walletView = walletFrame ? walletFrame->currentWalletView() : 0;

    if (!walletView) {
        labelLedgerIcon->setVisible(false);
        return;
    }

    WalletModel * const walletModel = walletView->getWalletModel();
    if(!walletModel) {
        labelLedgerIcon->setVisible(false);
        return;
    }

    if(walletModel->wallet().privateKeysDisabled())
    {
        labelLedgerIcon->setVisible(true);
        QList<HWDevice> devices = walletModel->getDevices();
        if(devices.count() > 0){
            labelLedgerIcon->setPixmap(platformStyle->MultiStatesIcon(":/icons/ledger_on").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
            QString toolTipStr;
            for(int i = 0; i < devices.count(); i++)
            {
                HWDevice device = devices[i];
                if(i > 0) toolTipStr += "<br><br>";
                toolTipStr += tr("Device connected.<br>Type is %1<br>Model is %2<br>Fingerprint is %3<br>App name is %4").arg(device.type, device.model, device.fingerprint, device.app_name);
            }
            labelLedgerIcon->setToolTip(toolTipStr);
        }
        else{
            labelLedgerIcon->setPixmap(platformStyle->MultiStatesIcon(":/icons/ledger_off").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
            labelLedgerIcon->setToolTip(tr("Device not connected"));
        }
    }
    else
    {
        labelLedgerIcon->setVisible(false);
    }
}

void BitcoinGUI::updateStakingIcon()
{
    if(m_node.shutdownRequested() || !clientModel || clientModel->fBatchProcessingMode)
        return;

    WalletView * const walletView = walletFrame ? walletFrame->currentWalletView() : 0;
    if (!walletView) {
        // Not staking because wallet is closed
        labelStakingIcon->setPixmap(platformStyle->MultiStatesIcon(":/icons/staking_off").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        labelStakingIcon->setToolTip(tr("Not staking"));
        return;
    }
    WalletModel * const walletModel = walletView->getWalletModel();

    uint64_t nWeight= walletModel->getStakeWeight();
    if (walletModel->wallet().getLastCoinStakeSearchInterval() &&
            walletModel->wallet().getEnabledStaking() && nWeight)
    {
        uint64_t nNetworkWeight = GetPoSKernelPS();
        const Consensus::Params& consensusParams = Params().GetConsensus();
        int headersTipHeight = clientModel->getHeaderTipHeight();
        int64_t nTargetSpacing = consensusParams.TargetSpacing(headersTipHeight);

        unsigned nEstimateTime = nTargetSpacing * nNetworkWeight / nWeight;

        QString text;
        if (nEstimateTime < 60)
        {
            text = tr("%n second(s)", "", nEstimateTime);
        }
        else if (nEstimateTime < 60*60)
        {
            text = tr("%n minute(s)", "", nEstimateTime/60);
        }
        else if (nEstimateTime < 24*60*60)
        {
            text = tr("%n hour(s)", "", nEstimateTime/(60*60));
        }
        else
        {
            text = tr("%n day(s)", "", nEstimateTime/(60*60*24));
        }

        nWeight /= COIN;
        nNetworkWeight /= COIN;

        labelStakingIcon->setPixmap(platformStyle->MultiStatesIcon(":/icons/staking_on").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        labelStakingIcon->setToolTip(tr("Staking.<br>Your weight is %1<br>Network weight is %2<br>Expected time to earn reward is %3").arg(nWeight).arg(nNetworkWeight).arg(text));
    }
    else
    {
        labelStakingIcon->setPixmap(platformStyle->MultiStatesIcon(":/icons/staking_off").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));

        if (m_node.getNodeCount(ConnectionDirection::Both) == 0)
            labelStakingIcon->setToolTip(tr("Not staking because wallet is offline"));
        else if (m_node.isInitialBlockDownload())
            labelStakingIcon->setToolTip(tr("Not staking because wallet is syncing"));
        else if (!nWeight)
            labelStakingIcon->setToolTip(tr("Not staking because you don't have mature coins"));
        else if (walletModel->wallet().isLocked())
            labelStakingIcon->setToolTip(tr("Not staking because wallet is locked"));
        else if(walletModel->hasLedgerProblem())
            labelStakingIcon->setToolTip(tr("Not staking because the ledger device failed to connect"));
        else
            labelStakingIcon->setToolTip(tr("Not staking"));
    }
}
#endif // ENABLE_WALLET

void BitcoinGUI::detectShutdown()
{
    if (m_node.shutdownRequested())
    {
        if(rpcConsole)
            rpcConsole->hide();
        Q_EMIT quitRequested();
    }
}

void BitcoinGUI::showProgress(const QString &title, int nProgress)
{
    if (nProgress == 0) {
        progressDialog = new QProgressDialog(title, QString(), 0, 100);
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
        progressDialog->setValue(nProgress);
    }
}

void BitcoinGUI::showModalOverlay()
{
    if (modalOverlay && (progressBar->isVisible() || modalOverlay->isLayerVisible()))
        modalOverlay->toggleVisibility();
}

void BitcoinGUI::showModalBackupOverlay()
{
    if (modalBackupOverlay)
        modalBackupOverlay->toggleVisibility();
}

static bool ThreadSafeMessageBox(BitcoinGUI* gui, const bilingual_str& message, const std::string& caption, unsigned int style)
{
    bool modal = (style & CClientUIInterface::MODAL);
    // The SECURE flag has no effect in the Qt GUI.
    // bool secure = (style & CClientUIInterface::SECURE);
    style &= ~CClientUIInterface::SECURE;
    bool ret = false;

    QString detailed_message; // This is original message, in English, for googling and referencing.
    if (message.original != message.translated) {
        detailed_message = BitcoinGUI::tr("Original message:") + "\n" + QString::fromStdString(message.original);
    }

    // In case of modal message, use blocking connection to wait for user to click a button
    bool invoked = QMetaObject::invokeMethod(gui, "message",
                               modal ? GUIUtil::blockingGUIThreadConnection() : Qt::QueuedConnection,
                               Q_ARG(QString, QString::fromStdString(caption)),
                               Q_ARG(QString, QString::fromStdString(message.translated)),
                               Q_ARG(unsigned int, style),
                               Q_ARG(bool*, &ret),
                               Q_ARG(QString, detailed_message));
    assert(invoked);
    return ret;
}

void BitcoinGUI::subscribeToCoreSignals()
{
    // Connect signals to client
    m_handler_message_box = m_node.handleMessageBox(std::bind(ThreadSafeMessageBox, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    m_handler_question = m_node.handleQuestion(std::bind(ThreadSafeMessageBox, this, std::placeholders::_1, std::placeholders::_3, std::placeholders::_4));
}

void BitcoinGUI::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    m_handler_message_box->disconnect();
    m_handler_question->disconnect();
}

void BitcoinGUI::addDockWindows(Qt::DockWidgetArea area, QWidget* widget)
{
    QDockWidget *dock = new QDockWidget(this);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setAllowedAreas(area);
    QWidget* titleBar = new QWidget();
    titleBar->setMaximumSize(0, 0);
    dock->setTitleBarWidget(titleBar);
    dock->setWidget(widget);
    addDockWidget(area, dock);
}

bool BitcoinGUI::isPrivacyModeActivated() const
{
    assert(m_mask_values_action);
    return m_mask_values_action->isChecked();
}

UnitDisplayStatusBarControl::UnitDisplayStatusBarControl(const PlatformStyle *platformStyle)
    : optionsModel(nullptr),
      menu(nullptr),
      m_platform_style{platformStyle}
{

    // Get params
    menuMargin = GetIntStyleValue("unitdisplaystatusbarcontrol/menu-margin", 5);
    iconHeight = GetIntStyleValue("unitdisplaystatusbarcontrol/icon-height", 0);
    iconWidth = GetIntStyleValue("unitdisplaystatusbarcontrol/icon-width", 0);
    iconPath = GetStringStyleValue("unitdisplaystatusbarcontrol/icon-path", "");

    createContextMenu();
    setToolTip(tr("Unit to show amounts in. Click to select another unit."));
    QList<BitcoinUnits::Unit> units = BitcoinUnits::availableUnits();
    int max_width = 0;
    const QFontMetrics fm(font());
    for (const BitcoinUnits::Unit unit : units)
    {
        max_width = qMax(max_width, GUIUtil::TextWidth(fm, BitcoinUnits::longName(unit)));
    }

    setMinimumSize(DISPLAY_UNIT_CONTROL_MARGIN + max_width + DISPLAY_UNIT_CONTROL_MARGIN, DISPLAY_UNIT_CONTROL_HEIGHT);
    setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    setTextFormat(Qt::RichText);
}

/** So that it responds to button clicks */
void UnitDisplayStatusBarControl::mousePressEvent(QMouseEvent *)
{

    menu->adjustSize();
    onDisplayUnitsClicked(QPoint((width() -menu->width())/2, -menu->height() -menuMargin));
}

void UnitDisplayStatusBarControl::changeEvent(QEvent* e)
{
    QLabel::changeEvent(e);
}

/** Creates context menu, its actions, and wires up all the relevant signals for mouse events. */
void UnitDisplayStatusBarControl::createContextMenu()
{
    menu = new QMenu(this);
    menu->setWindowFlag(Qt::Window);
    menu->setWindowFlag(Qt::FramelessWindowHint);
    menu->setAttribute(Qt::WA_TranslucentBackground);

    for (const BitcoinUnits::Unit u : BitcoinUnits::availableUnits()) {
        menu->addAction(BitcoinUnits::longName(u))->setData(QVariant(u));
    }
    connect(menu, &QMenu::triggered, this, &UnitDisplayStatusBarControl::onMenuSelection);
}

/** Lets the control know about the Options Model (and its signals) */
void UnitDisplayStatusBarControl::setOptionsModel(OptionsModel *_optionsModel)
{
    if (_optionsModel)
    {
        this->optionsModel = _optionsModel;

        // be aware of a display unit change reported by the OptionsModel object.
        connect(_optionsModel, &OptionsModel::displayUnitChanged, this, &UnitDisplayStatusBarControl::updateDisplayUnit);

        // initialize the display units label with the current value in the model.
        updateDisplayUnit(_optionsModel->getDisplayUnit());
    }
}

/** When Display Units are changed on OptionsModel it will refresh the display text of the control on the status bar */
void UnitDisplayStatusBarControl::updateDisplayUnit(int newUnits)
{
    static const QString INIT_DISPLAY_FORMAT = "<html>%1<img src='%2' width='%3' height='%4'></html>";
    setText(INIT_DISPLAY_FORMAT.arg(BitcoinUnits::longName(newUnits), iconPath, QString::number(iconWidth), QString::number(iconHeight)));
}

/** Shows context menu with Display Unit options by the mouse coordinates */
void UnitDisplayStatusBarControl::onDisplayUnitsClicked(const QPoint& point)
{
    QPoint globalPos = mapToGlobal(point);
    menu->exec(globalPos);
}

/** Tells underlying optionsModel to update its current display unit. */
void UnitDisplayStatusBarControl::onMenuSelection(QAction* action)
{
    if (action)
    {
        optionsModel->setDisplayUnit(action->data());
    }
}

WalletFrame *BitcoinGUI::getWalletFrame() const
{
    return walletFrame;
}

void BitcoinGUI::join()
{
#ifdef ENABLE_WALLET
    if (timerStakingIcon) timerStakingIcon->stop();
    if(m_wallet_controller)
    {
        for (WalletModel* wallet_model : m_wallet_controller->getOpenWallets()) {
            wallet_model->join();
        }
    }
#endif // ENABLE_WALLET
}
