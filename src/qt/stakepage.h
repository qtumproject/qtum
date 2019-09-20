#ifndef QTUM_QT_STAKEPAGE_H
#define QTUM_QT_STAKEPAGE_H

#include <interfaces/wallet.h>

#include <QWidget>
#include <memory>

class ClientModel;
class TransactionFilterProxy;
class PlatformStyle;
class WalletModel;

namespace Ui {
    class StakePage;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Stake page widget */
class StakePage : public QWidget
{
    Q_OBJECT

public:
    explicit StakePage(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~StakePage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);

public Q_SLOTS:
    void setBalance(const interfaces::WalletBalances& balances);

Q_SIGNALS:
    void transactionClicked(const QModelIndex &index);


private:
    Ui::StakePage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;

private Q_SLOTS:
    void on_checkStake_clicked(bool checked);
};

#endif // QTUM_QT_STAKEPAGE_H
