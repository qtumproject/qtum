#ifndef DELEGATIONSTAKERITEMMODEL_H
#define DELEGATIONSTAKERITEMMODEL_H

#include <qt/bitcoinunits.h>
#include <QAbstractItemModel>
#include <QStringList>
#include <QThread>

#include <memory>

namespace interfaces {
class Handler;
}

class WalletModel;
class DelegationStakerItemPriv;
class DelegationStakerItemEntry;

class DelegationStakerItemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum ColumnIndex {
        Date = 0,
        Delegate = 1,
        Fee = 2,
        Weight = 3
    };

    enum DataRole{
        HashRole = Qt::UserRole + 1,
        DelegateRole = Qt::UserRole + 2,
        StakerRole = Qt::UserRole + 3,
        PoDRole = Qt::UserRole + 4,
        FeeRole = Qt::UserRole + 5,
        DateRole = Qt::UserRole + 6,
        BlockNumberRole = Qt::UserRole + 7,
        WeightRole = Qt::UserRole + 8,
        FormattedWeightRole = Qt::UserRole + 9,
        FormattedFeeRole = Qt::UserRole + 10,
    };

    DelegationStakerItemModel(WalletModel *parent = 0);
    ~DelegationStakerItemModel();

    /** @name Methods overridden from QAbstractItemModel
        @{*/
    QModelIndex index(int row, int column,
                              const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    /*@}*/

private Q_SLOTS:
    void updateDelegationStakerData(const QString &hash, int status, bool showDelegationStaker);

public Q_SLOTS:
    /** Updates the column title to "Amount (DisplayUnit)" and emits headerDataChanged() signal for table headers to react. */
    void updateAmountColumnTitle();
    void updateDisplayUnit();

private:
    /** Notify listeners that data changed. */
    void emitDataChanged(int index);
    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();
    QString formatWeight(const DelegationStakerItemEntry *rec, BitcoinUnits::SeparatorStyle separators=BitcoinUnits::separatorStandard) const;
    QString formatFee(const DelegationStakerItemEntry *rec) const;

    QStringList columns;
    WalletModel *walletModel;
    DelegationStakerItemPriv* priv;
    std::unique_ptr<interfaces::Handler> m_handler_delegationsstaker_changed;

    friend class DelegationStakerItemPriv;
};

#endif // DELEGATIONSTAKERITEMMODEL_H
