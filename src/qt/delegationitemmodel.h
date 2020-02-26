#ifndef DELEGATIONNITEMMODEL_H
#define DELEGATIONNITEMMODEL_H

#include <QAbstractItemModel>

class DelegationItemModel : public QAbstractItemModel
{
public:
    enum DataRole{
        HashRole = Qt::UserRole + 1,
        AddressRole = Qt::UserRole + 2,
        StakerRole = Qt::UserRole + 3,
        FeeRole = Qt::UserRole + 4,
        BlockHeightRole = Qt::UserRole + 5,
        ProofOfDelegationRole = Qt::UserRole + 6,
    };
};

#endif // DELEGATIONNITEMMODEL_H
