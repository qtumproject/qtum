#include <qt/nfttxfilterproxy.h>

#include <cstdlib>

#include <QDateTime>

// Earliest date that can be represented (far in the past)
const QDateTime NftTxFilterProxy::MIN_DATE = QDateTime::fromTime_t(0);
// Last date that can be represented (far in the future)
const QDateTime NftTxFilterProxy::MAX_DATE = QDateTime::fromTime_t(0xFFFFFFFF);

NftTxFilterProxy::NftTxFilterProxy(QObject *parent) :
    QSortFilterProxyModel(parent),
    dateFrom(MIN_DATE),
    dateTo(MAX_DATE),
    addrPrefix(),
    name(),
    typeFilter(ALL_TYPES),
    minAmount(0),
    limitRows(-1)
{

}

void NftTxFilterProxy::setDateRange(const QDateTime &from, const QDateTime &to)
{
    this->dateFrom = from;
    this->dateTo = to;
    invalidateFilter();
}

void NftTxFilterProxy::setAddressPrefix(const QString &_addrPrefix)
{
    this->addrPrefix = _addrPrefix;
    invalidateFilter();
}

void NftTxFilterProxy::setTypeFilter(quint32 modes)
{
    this->typeFilter = modes;
    invalidateFilter();
}

void NftTxFilterProxy::setMinAmount(const qint32 &minimum)
{
    this->minAmount = minimum;
    invalidateFilter();
}

void NftTxFilterProxy::setName(const QString _name)
{
    this->name = _name;
    invalidateFilter();
}

void NftTxFilterProxy::setLimit(int limit)
{
    this->limitRows = limit;
}

int NftTxFilterProxy::rowCount(const QModelIndex &parent) const
{
    if(limitRows != -1)
    {
        return std::min(QSortFilterProxyModel::rowCount(parent), limitRows);
    }
    else
    {
        return QSortFilterProxyModel::rowCount(parent);
    }
}

bool NftTxFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    int type = index.data(NftTransactionTableModel::TypeRole).toInt();
    QDateTime datetime = index.data(NftTransactionTableModel::DateRole).toDateTime();
    QString address = index.data(NftTransactionTableModel::AddressRole).toString();
    qint32 amount = index.data(NftTransactionTableModel::AmountRole).toInt();
    amount = abs(amount);
    QString nftName = index.data(NftTransactionTableModel::NameRole).toString();

    if(!(TYPE(type) & typeFilter))
        return false;
    if(datetime < dateFrom || datetime > dateTo)
        return false;
    if (!address.contains(addrPrefix, Qt::CaseInsensitive))
        return false;
    if(amount < minAmount)
        return false;
    if(!name.isEmpty() && name != nftName)
        return false;

    return true;
}

bool NftTxFilterProxy::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if(left.column() == NftTransactionTableModel::Amount &&
            right.column() == NftTransactionTableModel::Amount)
    {
        qint32 amountLeft = left.data(NftTransactionTableModel::AmountRole).toInt();
        amountLeft = abs(amountLeft);

        qint32 amountRight = right.data(NftTransactionTableModel::AmountRole).toInt();
        amountRight = abs(amountRight);

        return amountLeft < amountRight;
    }
    return QSortFilterProxyModel::lessThan(left, right);
}
