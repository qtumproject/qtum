#ifndef NFTFILTERPROXY_H
#define NFTFILTERPROXY_H

#include <amount.h>
#include <qt/nftitemmodel.h>
#include <QDateTime>
#include <QSortFilterProxyModel>

class NftFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit NftFilterProxy(QObject *parent = 0);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;

private:
};

#endif // NFTFILTERPROXY_H
