#ifndef TITLEBAR_H
#define TITLEBAR_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <QWidget>
#include <QSize>
#include <QTabBar>
#include <QIcon>
#include <QLabel>
#include <QComboBox>
#include <QPointer>
#ifdef ENABLE_WALLET
#include <qt/walletmodel.h>
#endif

namespace Ui {
class TitleBar;
}
class WalletModel;
class TabBarInfo;
class PlatformStyle;

/**
 * @brief The TitleBar class Title bar widget
 */
class TitleBar : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief TitleBar Constructor
     * @param parent Parent widget
     */
    explicit TitleBar(const PlatformStyle *platformStyle, QWidget *parent = 0);
    
    /**
     * @brief TitleBar Destrustor
     */
    ~TitleBar();

#ifdef ENABLE_WALLET
    /**
     * @brief setModel Set wallet model
     * @param _model Wallet model
     */
    void setModel(WalletModel *_model);

    /**
     * @brief addWallet Add wallet model
     * @param _model Wallet model
     */
    void addWallet(WalletModel *_model);

    /**
     * @brief removeWallet Remove wallet model
     * @param _model Wallet model
     */
    void removeWallet(WalletModel *_model);
#endif

    /**
     * @brief setTabBarInfo Set the tab bar info
     * @param info Information about tabs
     */
    void setTabBarInfo(QObject* info);

    /**
     * @brief setWalletSelector Set wallet selector
     * @param walletSelectorLabel Wallet selector label
     * @param walletSelector Wallet selector
     */
    void setWalletSelector(QLabel *walletSelectorLabel, QComboBox* walletSelector);


Q_SIGNALS:

public Q_SLOTS:
#ifdef ENABLE_WALLET
    /**
     * @brief setBalance Slot for changing the balance
     */
    void setBalance(const interfaces::WalletBalances& balances);
#endif

    /**
     * @brief on_navigationResized Slot for changing the size of the navigation bar
     * @param _size Size of the navigation bar
     */
    void on_navigationResized(const QSize& _size);

#ifdef ENABLE_WALLET
    void updateDisplayUnit();
#endif

private:
#ifdef ENABLE_WALLET
    /**
     * @brief setBalanceLabel Changing the displayed balance
     */
    void setBalanceLabel(const interfaces::WalletBalances& balances);
#endif

private:
    Ui::TitleBar *ui;
#ifdef ENABLE_WALLET
    QPointer<WalletModel> m_model;
#endif
    QPointer<TabBarInfo> m_tab;
    QIcon m_iconCloseTab;
#ifdef ENABLE_WALLET
    std::map<QObject*, interfaces::WalletBalances> m_models;
#endif
};

#endif // TITLEBAR_H
