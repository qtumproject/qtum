// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "splashscreen.h"

#include "networkstyle.h"

#include "clientversion.h"
#include "init.h"
#include "util.h"
#include "ui_interface.h"
#include "version.h"

#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QPainter>
#include <QRadialGradient>

SplashScreen::SplashScreen(Qt::WindowFlags f, const NetworkStyle *networkStyle) :
    QWidget(0, f), curAlignment(0)
{
    // set sizes
    int versionTextHeight       = 30;
    int statusHeight            = 30;
    int titleAddTextHeight      = 20;

    float fontFactor            = 1.0;
    float devicePixelRatio      = 1.0;
#if QT_VERSION > 0x050100
    devicePixelRatio = ((QGuiApplication*)QCoreApplication::instance())->devicePixelRatio();
#endif

    // define text to place
    QString titleText       = tr(PACKAGE_NAME);
    QString versionText     = QString("Version %1").arg(QString::fromStdString(FormatFullVersion()));
    QString copyrightText   = QString::fromUtf8(CopyrightHolders(strprintf("\xc2\xA9 %u ", COPYRIGHT_YEAR)).c_str());
    QString titleAddText    = networkStyle->getTitleAddText();

    QString font            = QApplication::font().toString();

    // create a bitmap according to device pixelratio
    QSize splashSize(480,320);
    pixmap = QPixmap(480*devicePixelRatio,320*devicePixelRatio);

#if QT_VERSION > 0x050100
    // change to HiDPI if it makes sense
    pixmap.setDevicePixelRatio(devicePixelRatio);
#endif

    QPainter pixPaint(&pixmap);
    pixPaint.setPen(QColor("#ffffff"));

    QRect mainRect(QPoint(0,0), splashSize);
    pixPaint.fillRect(mainRect, QColor("#030509"));

    // draw background
    QRect rectBg(QPoint(-50, -50), QSize(splashSize.width() + 50, splashSize.height() + 50));
    QPixmap bg(":/styles/app-icons/splash_bg");
    pixPaint.drawPixmap(rectBg, bg);

    pixPaint.setFont(QFont(font, 32*fontFactor, QFont::Bold));
    QRect rectTitle(QPoint(0,0), QSize(splashSize.width(), (splashSize.height() / 2)));
    pixPaint.drawText(rectTitle, Qt::AlignHCenter | Qt::AlignBottom, titleText);

    QPoint versionPoint(rectTitle.bottomLeft());

    // draw additional text if special network
    if(!titleAddText.isEmpty())
    {
        QRect titleAddRect(rectTitle.bottomLeft(), QSize(rectTitle.width(), titleAddTextHeight));
        versionPoint = titleAddRect.bottomLeft();
        pixPaint.setFont(QFont(font, 8*fontFactor, QFont::Bold));
        pixPaint.drawText(titleAddRect, Qt::AlignHCenter | Qt::AlignVCenter, titleAddText);
    }

    pixPaint.setFont(QFont(font, 11*fontFactor));
    QRect versionRect(versionPoint, QSize(rectTitle.width(), versionTextHeight));
    pixPaint.drawText(versionRect, Qt::AlignHCenter | Qt::AlignTop, versionText);

    // draw copyright stuff
    QFont statusFont = QApplication::font();
    statusFont.setPointSizeF(statusFont.pointSizeF() * 0.9);
    pixPaint.setFont(statusFont);
    QRect statusRect(mainRect.left(), mainRect.height() - statusHeight, mainRect.width(), statusHeight);
    QColor statusColor(255, 255, 255);
    statusColor.setAlphaF(0.1);
    pixPaint.fillRect(statusRect, statusColor);
    pixPaint.drawText(statusRect.adjusted(10, 0, -10, 0), Qt::AlignLeft | Qt::AlignVCenter, copyrightText);

    pixPaint.end();

    // Set window title
    setWindowTitle(titleText + " " + titleAddText);

    // Resize window and move to center of desktop, disallow resizing
    QRect r(QPoint(), QSize(pixmap.size().width()/devicePixelRatio,pixmap.size().height()/devicePixelRatio));
    resize(r.size());
    setFixedSize(r.size());
    move(QApplication::desktop()->screenGeometry().center() - r.center());

    subscribeToCoreSignals();
}

SplashScreen::~SplashScreen()
{
    unsubscribeFromCoreSignals();
}

void SplashScreen::slotFinish(QWidget *mainWin)
{
    Q_UNUSED(mainWin);

    /* If the window is minimized, hide() will be ignored. */
    /* Make sure we de-minimize the splashscreen window before hiding */
    if (isMinimized())
        showNormal();
    hide();
    deleteLater(); // No more need for this
}

static void InitMessage(SplashScreen *splash, const std::string &message)
{
    QMetaObject::invokeMethod(splash, "showMessage",
        Qt::QueuedConnection,
        Q_ARG(QString, QString::fromStdString(message)),
        Q_ARG(int, Qt::AlignBottom|Qt::AlignRight),
        Q_ARG(QColor, QColor("#ffffff")));
}

static void ShowProgress(SplashScreen *splash, const std::string &title, int nProgress)
{
    InitMessage(splash, title + strprintf("%d", nProgress) + "%");
}

#ifdef ENABLE_WALLET
void SplashScreen::ConnectWallet(CWallet* wallet)
{
    wallet->ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));
    connectedWallets.push_back(wallet);
}
#endif

void SplashScreen::subscribeToCoreSignals()
{
    // Connect signals to client
    uiInterface.InitMessage.connect(boost::bind(InitMessage, this, _1));
    uiInterface.ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));
#ifdef ENABLE_WALLET
    uiInterface.LoadWallet.connect(boost::bind(&SplashScreen::ConnectWallet, this, _1));
#endif
}

void SplashScreen::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    uiInterface.InitMessage.disconnect(boost::bind(InitMessage, this, _1));
    uiInterface.ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
#ifdef ENABLE_WALLET
    Q_FOREACH(CWallet* const & pwallet, connectedWallets) {
        pwallet->ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
    }
#endif
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
    StartShutdown(); // allows an "emergency" shutdown during startup
    event->ignore();
}
