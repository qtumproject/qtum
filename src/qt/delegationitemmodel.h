#ifndef DELEGATIONITEMMODEL_H
#define DELEGATIONITEMMODEL_H

#include <QAbstractItemModel>
#include <QStringList>
#include <QThread>
#include <QtGlobal>

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
        StakerName = 1,
        StakerAddress = 2,
        Fee = 3,
        Height = 4,
        Time = 5
    };

    enum DataRole{
        HashRole = Qt::UserRole + 1,
        AddressRole = Qt::UserRole + 2,
        StakerNameRole = Qt::UserRole + 3,
        StakerAddressRole = Qt::UserRole + 4,
        FeeRole = Qt::UserRole + 5,
        BlockHeightRole = Qt::UserRole + 6,
        CreateTxHashRole = Qt::UserRole + 7,
        RemoveTxHashRole = Qt::UserRole + 8,
        FormattedFeeRole = Qt::UserRole + 9,
        BalanceRole = Qt::UserRole + 10,
        StakeRole = Qt::UserRole + 11,
        WeightRole = Qt::UserRole + 12,
        FormattedWeightRole = Qt::UserRole + 13,
        TxStatusRole = Qt::UserRole + 14,
    };

    enum TxStatus
    {
        NoTx = 0,
        CreateTxConfirmed = 1,
        CreateTxNotConfirmed = 2,
        CreateTxError = 3,
        RemoveTxNotConfirmed = 4,
        RemoveTxError = 5,
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
    void itemChanged(QString hash, qint64 balance, qint64 stake, qint64 weight, qint32 status);

private Q_SLOTS:
    void updateDelegationData(const QString &hash, int status, bool showDelegation);

private:
    /** Notify listeners that data changed. */
    void emitDataChanged(int index);
    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();
    QString formatFee(const DelegationItemEntry *rec) const;

    QStringList columns;
    WalletModel *walletModel;
    DelegationItemPriv* priv;
    DelegationWorker* worker;
    QThread t;
    std::unique_ptr<interfaces::Handler> m_handler_delegation_changed;

    friend class DelegationItemPriv;
};

#endif // DELEGATIONITEMMODEL_H
