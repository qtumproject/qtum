#ifndef SUPERSTAKERITEMMODEL_H
#define SUPERSTAKERITEMMODEL_H

#include <QAbstractItemModel>
class SuperStakerItemModel : public QAbstractItemModel
{
public:
    enum DataRole{
        HashRole = Qt::UserRole + 1,
        StakerRole = Qt::UserRole + 2,
        FeeRole = Qt::UserRole + 3,
    };
};

#endif // SUPERSTAKERITEMMODEL_H
