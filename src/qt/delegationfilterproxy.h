#ifndef DELEGATIONFILTERPROXY_H
#define DELEGATIONFILTERPROXY_H

#include <qt/delegationstakeritemmodel.h>

#include <amount.h>
#include <QDateTime>
#include <QSortFilterProxyModel>

class DelegationFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit DelegationFilterProxy(QObject *parent = 0);

    /** Earliest date that can be represented (far in the past) */
    static const QDateTime MIN_DATE;
    /** Last date that can be represented (far in the future) */
    static const QDateTime MAX_DATE;

    void setStaker(const QString &addrStaker);
    void setDateRange(const QDateTime &from, const QDateTime &to);
    void setAddrPrefix(const QString &addrPrefix);
    void setMinFee(const int& minimum);
    void setMinAmount(const CAmount& minimum);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
    QString addrStaker;
    QDateTime dateFrom;
    QDateTime dateTo;
    QString addrPrefix;
    int minFee;
    CAmount minAmount;
};

#endif // DELEGATIONFILTERPROXY_H
