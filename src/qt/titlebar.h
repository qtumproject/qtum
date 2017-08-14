#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QWidget>
#include "walletmodel.h"

namespace Ui {
class TitleBar;
}
class WalletModel;

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
    explicit TitleBar(QWidget *parent = 0);
    
    /**
     * @brief TitleBar Destrustor
     */
    ~TitleBar();

    /**
     * @brief setModel Set wallet model
     * @param _model Wallet model
     */
    void setModel(WalletModel *_model);

Q_SIGNALS:

public Q_SLOTS:
    /**
     * @brief setBalance Slot for changing the balance
     */
    void setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& stake,
                    const CAmount& watchBalance, const CAmount& watchUnconfirmedBalance, const CAmount& watchImmatureBalance, const CAmount& watchStake);

private:
    Ui::TitleBar *ui;
    WalletModel *model;
};

#endif // TITLEBAR_H
