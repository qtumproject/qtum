#include "delegationfilterproxy.h"

// Earliest date that can be represented (far in the past)
const QDateTime DelegationFilterProxy::MIN_DATE = QDateTime::fromTime_t(0);
// Last date that can be represented (far in the future)
const QDateTime DelegationFilterProxy::MAX_DATE = QDateTime::fromTime_t(0xFFFFFFFF);

DelegationFilterProxy::DelegationFilterProxy(QObject *parent) :
    QSortFilterProxyModel(parent),
    dateFrom(MIN_DATE),
    dateTo(MAX_DATE),
    addrPrefix(),
    minFee(0),
    podPrefix()
{

}

void DelegationFilterProxy::setDateRange(const QDateTime &from, const QDateTime &to)
{
    this->dateFrom = from;
    this->dateTo = to;
    invalidateFilter();
}

void DelegationFilterProxy::setAddrPrefix(const QString &_addrPrefix)
{
    this->addrPrefix = _addrPrefix;
    invalidateFilter();
}

void DelegationFilterProxy::setMinFee(const int &_minimum)
{
    this->minFee = _minimum;
    invalidateFilter();
}

void DelegationFilterProxy::setPODPrefix(const QString &_podPrefix)
{
    this->podPrefix = _podPrefix;
    invalidateFilter();
}

bool DelegationFilterProxy::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);

    QDateTime datetime;
    QString address = index.data(DelegationItemModel::AddressRole).toString();
    int fee = (index.data(DelegationItemModel::FeeRole).toInt());
    QString PoD;

    if(datetime < dateFrom || datetime > dateTo)
        return false;
    if (!address.contains(addrPrefix, Qt::CaseInsensitive))
        return false;
    if (!PoD.contains(podPrefix, Qt::CaseInsensitive))
        return false;
    if(fee < minFee)
        return false;

    return true;
}
