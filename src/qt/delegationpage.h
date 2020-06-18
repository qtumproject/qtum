#ifndef DELEGATIONPAGE_H
#define DELEGATIONPAGE_H

#include <qt/removedelegationpage.h>
#include <qt/adddelegationpage.h>
#include <qt/splitutxopage.h>

#include <QWidget>
#include <QModelIndex>
#include <QAbstractItemModel>

class WalletModel;
class ClientModel;
class DelegationListWidget;
class PlatformStyle;
class QMenu;

namespace Ui {
class DelegationPage;
}

class DelegationPage : public QWidget
{
    Q_OBJECT

public:
    explicit DelegationPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~DelegationPage();

    void setModel(WalletModel *_model);
    void setClientModel(ClientModel *clientModel);

Q_SIGNALS:

public Q_SLOTS:
    void on_goToSplitCoinsPage();
    void on_goToRemoveDelegationPage();
    void on_goToAddDelegationPage();
    void on_currentDelegationChanged(QModelIndex index);
    void on_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());
    void on_currentChanged(QModelIndex current, QModelIndex previous);
    void on_rowsInserted(QModelIndex index, int first, int last);
    void contextualMenu(const QPoint &);
    void copyDelegateAddress();
    void copyDelegateWeight();
    void copyStekerFee();
    void copyStakerName();
    void copyStakerAddress();
    void editStakerName();
    void removeDelegation();
    void on_removeDelegation(const QModelIndex& index);
    void on_addDelegation();
    void on_splitCoins(const QModelIndex& index);
    void on_restoreDelegations();

private:
    Ui::DelegationPage *ui;
    RemoveDelegationPage* m_removeDelegationPage;
    AddDelegationPage* m_addDelegationPage;
    SplitUTXOPage* m_splitUtxoPage;
    WalletModel* m_model;
    ClientModel* m_clientModel;
    QAction *m_sendAction;
    QAction *m_removeAction;
    QAction *m_addDelegationAction;
    QString m_selectedDelegationHash;
    const PlatformStyle *m_platformStyle;
    QMenu *contextMenu;
    QModelIndex indexMenu;
    DelegationListWidget* m_delegationList;
};

#endif // DELEGATIONPAGE_H
