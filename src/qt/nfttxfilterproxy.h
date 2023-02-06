#ifndef NFTTXFILTERPROXY_H
#define NFTTXFILTERPROXY_H

#include <amount.h>
#include <qt/nfttransactiontablemodel.h>
#include <QDateTime>
#include <QSortFilterProxyModel>

class NftTxFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit NftTxFilterProxy(QObject *parent = 0);

    /** Earliest date that can be represented (far in the past) */
    static const QDateTime MIN_DATE;
    /** Last date that can be represented (far in the future) */
    static const QDateTime MAX_DATE;
    /** Type filter bit field (all types) */
    static const quint32 ALL_TYPES = 0xFFFFFFFF;

    static quint32 TYPE(int type) { return 1<<type; }

    void setDateRange(const QDateTime &from, const QDateTime &to);
    void setAddressPrefix(const QString &addrPrefix);
    /**
      @note Type filter takes a bit field created with TYPE() or ALL_TYPES
     */
    void setTypeFilter(quint32 modes);
    void setMinAmount(const qint32& minimum);
    void setName(const QString _name);

    /** Set maximum number of rows returned, -1 if unlimited. */
    void setLimit(int limit);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;
    bool lessThan(const QModelIndex & left, const QModelIndex & right) const override;

private:
    QDateTime dateFrom;
    QDateTime dateTo;
    QString addrPrefix;
    QString name;
    quint32 typeFilter;
    qint32 minAmount;
    int limitRows;
    bool showInactive;
};

#endif // NFTTXFILTERPROXY_H
