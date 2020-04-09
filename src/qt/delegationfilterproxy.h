#ifndef DELEGATIONFILTERPROXY_H
#define DELEGATIONFILTERPROXY_H

#include <qt/delegationstakeritemmodel.h>

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

    void setDateRange(const QDateTime &from, const QDateTime &to);
    void setAddrPrefix(const QString &addrPrefix);
    void setMinFee(const int& minimum);
    void setPODPrefix(const QString &podPrefix);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
    QDateTime dateFrom;
    QDateTime dateTo;
    QString addrPrefix;
    int minFee;
    QString podPrefix;
};

#endif // DELEGATIONFILTERPROXY_H
