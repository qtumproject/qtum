#include <qt/nftfilterproxy.h>

#include <cstdlib>

#include <QDateTime>

NftFilterProxy::NftFilterProxy(QObject *parent) :
    QSortFilterProxyModel(parent)
{}

bool NftFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    int balance = index.data(NftItemModel::BalanceRole).toInt();
    return balance > 0;
}
