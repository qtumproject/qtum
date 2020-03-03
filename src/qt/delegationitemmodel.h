#ifndef DELEGATIONITEMMODEL_H
#define DELEGATIONITEMMODEL_H

#include <QAbstractItemModel>
#include <QStringList>
#include <QThread>

#include <memory>

namespace interfaces {
class Handler;
}

class WalletModel;
class DelegationItemPriv;
class DelegationWorker;
class DelegationItemEntry;

class DelegationItemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum ColumnIndex {
        Address = 0,
        Staker = 1,
        Fee = 2,
        Height = 3,
        Time = 4
    };

    enum DataRole{
        HashRole = Qt::UserRole + 1,
        AddressRole = Qt::UserRole + 2,
        StakerRole = Qt::UserRole + 3,
        FeeRole = Qt::UserRole + 4,
        BlockHeightRole = Qt::UserRole + 5,
        CreateTxHashRole = Qt::UserRole + 6,
        RemoveTxHashRole = Qt::UserRole + 7,
    };

    DelegationItemModel(WalletModel *parent = 0);
    ~DelegationItemModel();

    /** @name Methods overridden from QAbstractItemModel
        @{*/
    QModelIndex index(int row, int column,
                              const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    /*@}*/
    
    void updateDelegationData(const DelegationItemEntry& entry);

public Q_SLOTS:
    void checkDelegationChanged();

private Q_SLOTS:
    void updateDelegationData(const QString &hash, int status, bool showDelegation);

private:
    /** Notify listeners that data changed. */
    void emitDataChanged(int index);
    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();

    QStringList columns;
    WalletModel *walletModel;
    DelegationItemPriv* priv;
    DelegationWorker* worker;
    QThread t;
    std::unique_ptr<interfaces::Handler> m_handler_delegation_changed;

    friend class DelegationItemPriv;
};

#endif // DELEGATIONITEMMODEL_H
