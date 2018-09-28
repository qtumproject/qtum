#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QWidget>
#include <QSize>
#include <QTabBar>
#include <QIcon>
#include <QLabel>
#include <QComboBox>
#include <qt/walletmodel.h>

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
    /**
     * @brief setBalance Slot for changing the balance
     */
    void setBalance(const interfaces::WalletBalances& balances);

    /**
     * @brief on_navigationResized Slot for changing the size of the navigation bar
     * @param _size Size of the navigation bar
     */
    void on_navigationResized(const QSize& _size);

private:
    /**
     * @brief setBalanceLabel Changing the displayed balance
     */
    void setBalanceLabel(const interfaces::WalletBalances& balances);

private:
    Ui::TitleBar *ui;
    WalletModel *m_model;
    TabBarInfo* m_tab;
    QIcon m_iconCloseTab;
    std::map<QObject*, interfaces::WalletBalances> m_models;
};

#endif // TITLEBAR_H
