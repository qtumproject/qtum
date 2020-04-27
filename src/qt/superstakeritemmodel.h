#ifndef SUPERSTAKERITEMMODEL_H
#define SUPERSTAKERITEMMODEL_H

#include <QAbstractItemModel>
#include <QStringList>
#include <QThread>

#include <memory>

namespace interfaces {
class Handler;
}

class WalletModel;
class SuperStakerItemPriv;
class SuperStakerWorker;
class SuperStakerItemEntry;

class SuperStakerItemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum ColumnIndex {
        StakerName = 0,
        StakerAddress = 1,
        MinFee = 2,
        Staking = 3,
    };

    enum DataRole{
        HashRole = Qt::UserRole + 1,
        StakerNameRole = Qt::UserRole + 2,
        StakerAddressRole = Qt::UserRole + 3,
        MinFeeRole = Qt::UserRole + 4,
        StakingRole = Qt::UserRole + 5,
        FormattedMinFeeRole = Qt::UserRole + 6,
    };

    SuperStakerItemModel(WalletModel *parent = 0);
    ~SuperStakerItemModel();

    /** @name Methods overridden from QAbstractItemModel
        @{*/
    QModelIndex index(int row, int column,
                              const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    /*@}*/
    
    void updateSuperStakerData(const SuperStakerItemEntry& entry);

public Q_SLOTS:
    void checkSuperStakerChanged();

private Q_SLOTS:
    void updateSuperStakerData(const QString &hash, int status, bool showSuperStaker);

private:
    /** Notify listeners that data changed. */
    void emitDataChanged(int index);
    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();
    QString formatMinFee(const SuperStakerItemEntry *rec) const;

    QStringList columns;
    WalletModel *walletModel;
    SuperStakerItemPriv* priv;
    SuperStakerWorker* worker;
    QThread t;
    std::unique_ptr<interfaces::Handler> m_handler_superstaker_changed;

    friend class SuperStakerItemPriv;
};

#endif // SUPERSTAKERITEMMODEL_H
