// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/splashscreen.h>

#include <qt/networkstyle.h>

#include <clientversion.h>
#include <interfaces/handler.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <ui_interface.h>
#include <util/system.h>
#include <version.h>
#include "styleSheet.h"
#include <qt/platformstyle.h>

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QPainter>
#include <QRadialGradient>


SplashScreen::SplashScreen(interfaces::Node& node, Qt::WindowFlags f, const NetworkStyle *networkStyle) :
    QWidget(nullptr, f), curAlignment(0), m_node(node)
{
    // set sizes
    int logoSize                = 50;
    int logoImageSize           = logoSize - 13;
    int packageTextHeight       = 30;
    int versionTextHeight       = 20;
    int statusHeight            = 30;
    int titleAddTextHeight      = 12;
    int welcomeTextHeight       = 35;
    float fontFactor            = 1.0;
    float devicePixelRatio      = 1.0;
    devicePixelRatio = static_cast<QGuiApplication*>(QCoreApplication::instance())->devicePixelRatio();

    // define text to place
    QString titleText       = tr(PACKAGE_NAME);
    QString versionText     = QString("%1").arg(QString::fromStdString(FormatFullVersion()));
    QString copyrightText   = QString::fromUtf8(CopyrightHolders(strprintf("\xc2\xA9 %u ", COPYRIGHT_YEAR)).c_str());
    QString titleAddText    = networkStyle->getTitleAddText();

    QString font            = QApplication::font().toString();

    // create a bitmap according to device pixelratio
    QSize splashSize(480,320);
    pixmap = QPixmap(480*devicePixelRatio,320*devicePixelRatio);

    // change to HiDPI if it makes sense
    pixmap.setDevicePixelRatio(devicePixelRatio);

    QPainter pixPaint(&pixmap);

    QColor foreground_color = GetStringStyleValue("splashscreen/foreground-color", "#ffffff");
    QColor foreground_color_statusbar = GetStringStyleValue("splashscreen/foreground-color-statusbar", "#ffffff");
    QColor logo_frame_color = GetStringStyleValue("splashscreen/logo-frame-color", "#ffffff");

    QRect mainRect(QPoint(0,0), splashSize);
    QColor background_color     = GetStringStyleValue("splashscreen/background-color", "#030509");
    pixPaint.fillRect(mainRect, background_color);

    // draw background
    QRect rectBg(QPoint(-50, -50), QSize(splashSize.width() + 50, splashSize.height() + 50));
    QPixmap bg(GetStringStyleValue("splashscreen/background-image", ":/styles/theme1/app-icons/splash_bg"));
    pixPaint.drawPixmap(rectBg, bg);

    QRect logoRect(splashSize.width() - logoSize - 20, 20, logoSize, logoSize);
    QPainterPath logoPath;
    logoPath.addRoundedRect(logoRect, logoSize / 2, logoSize / 2);
    pixPaint.setRenderHint(QPainter::Antialiasing);
    pixPaint.setPen(logo_frame_color);
    pixPaint.drawPath(logoPath);

    QPixmap logo = PlatformStyle::SingleColorIcon(":/icons/bitcoin", foreground_color).pixmap(QSize(logoImageSize, logoImageSize));
    pixPaint.drawPixmap(logoRect.x() + 6, logoRect.y() + 6, logo);

    pixPaint.setPen(foreground_color);

    pixPaint.setFont(QFont(font, 22 * fontFactor, QFont::Bold));
    QRect rectTitle(QPoint(0, logoRect.bottom() + 10), QSize(splashSize.width() - 20, packageTextHeight));
    pixPaint.drawText(rectTitle, Qt::AlignRight | Qt::AlignBottom, titleText);

    QPoint versionPoint(rectTitle.bottomLeft());

    // draw additional text if special network
    if(!titleAddText.isEmpty())
    {
        QRect titleAddRect(rectTitle.bottomLeft(), QSize(rectTitle.width(), titleAddTextHeight));
        versionPoint = titleAddRect.bottomLeft();
        pixPaint.setFont(QFont("HiraginoSansGB", 8 * fontFactor, QFont::Bold));
        pixPaint.drawText(titleAddRect, Qt::AlignRight | Qt::AlignBottom, titleAddText);
    }

    pixPaint.setFont(QFont("HiraginoSansGB", 8 * fontFactor));
    QRect versionRect(versionPoint, QSize(rectTitle.width(), versionTextHeight));
    pixPaint.drawText(versionRect, Qt::AlignRight | Qt::AlignVCenter, versionText);

    QRect welcomeRect(0, splashSize.height() - statusHeight - welcomeTextHeight - 40, splashSize.width() -20, welcomeTextHeight);
    pixPaint.setFont(QFont(font, 10 * fontFactor, QFont::Bold));
    pixPaint.drawText(welcomeRect, Qt::AlignRight | Qt::AlignTop, "Qtum-Qt Wallet");

    // draw copyright stuff
    QFont statusFont = QApplication::font();
    statusFont.setPointSizeF(statusFont.pointSizeF() * 0.9);
    pixPaint.setFont(statusFont);
    pixPaint.setPen(foreground_color_statusbar);
    QRect statusRect(mainRect.left(), mainRect.height() - statusHeight, mainRect.width(), statusHeight);
    QColor statusColor(255, 255, 255);
    statusColor.setAlphaF(0.1);
    pixPaint.fillRect(statusRect, statusColor);
    pixPaint.drawText(statusRect.adjusted(10, 0, -10, 0), Qt::AlignRight | Qt::AlignVCenter, copyrightText);

    pixPaint.end();

    // Set window title
    setWindowTitle(titleText + " " + titleAddText);

    // Resize window and move to center of desktop, disallow resizing
    QRect r(QPoint(), QSize(pixmap.size().width()/devicePixelRatio,pixmap.size().height()/devicePixelRatio));
    resize(r.size());
    setFixedSize(r.size());
    move(QApplication::desktop()->screenGeometry().center() - r.center());

    subscribeToCoreSignals();
    installEventFilter(this);
}

SplashScreen::~SplashScreen()
{
    unsubscribeFromCoreSignals();
}

bool SplashScreen::eventFilter(QObject * obj, QEvent * ev) {
    if (ev->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
        if(keyEvent->text()[0] == 'q') {
            m_node.startShutdown();
        }
    }
    return QObject::eventFilter(obj, ev);
}

void SplashScreen::finish()
{
    /* If the window is minimized, hide() will be ignored. */
    /* Make sure we de-minimize the splashscreen window before hiding */
    if (isMinimized())
        showNormal();
    hide();
    deleteLater(); // No more need for this
}

static void InitMessage(SplashScreen *splash, const std::string &message)
{
    QColor foreground_color = GetStringStyleValue("splashscreen/foreground-color_statusbar", "#ffffff");
    QMetaObject::invokeMethod(splash, "showMessage",
        Qt::QueuedConnection,
        Q_ARG(QString, QString::fromStdString(message)),
        Q_ARG(int, Qt::AlignBottom|Qt::AlignLeft),
        Q_ARG(QColor, foreground_color));
}

static void ShowProgress(SplashScreen *splash, const std::string &title, int nProgress, bool resume_possible)
{
    InitMessage(splash, title + std::string("\n") +
            (resume_possible ? _("(press q to shutdown and continue later)")
                                : _("press q to shutdown")) +
            strprintf("\n%d", nProgress) + "%");
}
#ifdef ENABLE_WALLET
void SplashScreen::ConnectWallet(std::unique_ptr<interfaces::Wallet> wallet)
{
    m_connected_wallet_handlers.emplace_back(wallet->handleShowProgress(std::bind(ShowProgress, this, std::placeholders::_1, std::placeholders::_2, false)));
    m_connected_wallets.emplace_back(std::move(wallet));
}
#endif

void SplashScreen::subscribeToCoreSignals()
{
    // Connect signals to client
    m_handler_init_message = m_node.handleInitMessage(std::bind(InitMessage, this, std::placeholders::_1));
    m_handler_show_progress = m_node.handleShowProgress(std::bind(ShowProgress, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
#ifdef ENABLE_WALLET
    m_handler_load_wallet = m_node.handleLoadWallet([this](std::unique_ptr<interfaces::Wallet> wallet) { ConnectWallet(std::move(wallet)); });
#endif
}

void SplashScreen::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    m_handler_init_message->disconnect();
    m_handler_show_progress->disconnect();
    for (const auto& handler : m_connected_wallet_handlers) {
        handler->disconnect();
    }
    m_connected_wallet_handlers.clear();
    m_connected_wallets.clear();
}

void SplashScreen::showMessage(const QString &message, int alignment, const QColor &color)
{
    curMessage = message;
    curAlignment = alignment;
    curColor = color;
    update();
}

void SplashScreen::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, pixmap);
    QRect r = rect().adjusted(10, 10, -10, -10);
    painter.setPen(curColor);
    QFont font = QApplication::font();
    font.setPointSizeF(font.pointSizeF() * 0.9);
    painter.setFont(font);
    painter.drawText(r, curAlignment, curMessage);
}

void SplashScreen::closeEvent(QCloseEvent *event)
{
    m_node.startShutdown(); // allows an "emergency" shutdown during startup
    event->ignore();
}
