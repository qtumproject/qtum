#include <qt/superstakeritemmodel.h>
#include <qt/walletmodel.h>
#include <interfaces/wallet.h>
#include <validation.h>
#include <qt/bitcoinunits.h>
#include <interfaces/node.h>
#include <interfaces/handler.h>
#include <algorithm>

#include <QDateTime>
#include <QFont>
#include <QDebug>
#include <QThread>

class SuperStakerItemEntry
{
public:
    SuperStakerItemEntry()
    {}

    SuperStakerItemEntry(const interfaces::SuperStakerInfo &superStakerInfo)
    {
        hash = superStakerInfo.hash;
        stakerAddress = QString::fromStdString(superStakerInfo.staker_address);
        fee = superStakerInfo.fee;
        staking = false;
    }

    SuperStakerItemEntry( const SuperStakerItemEntry &obj)
    {
        hash = obj.hash;
        stakerAddress = obj.stakerAddress;
        fee = obj.fee;
        staking = obj.staking;
    }

    ~SuperStakerItemEntry()
    {}

    uint256 hash;
    QString stakerAddress;
    quint8 fee;
    bool staking;
};

class SuperStakerWorker : public QObject
{
    Q_OBJECT
public:
    WalletModel *walletModel;
    bool first;
    SuperStakerWorker(WalletModel *_walletModel):
        walletModel(_walletModel), first(true) {}

private Q_SLOTS:
    void updateSuperStakerData(QString hash, QString stakerAddress, quint8 fee)
    {
    }

Q_SIGNALS:
};

#include <qt/superstakeritemmodel.moc>

struct SuperStakerItemEntryLessThan
{
    bool operator()(const SuperStakerItemEntry &a, const SuperStakerItemEntry &b) const
    {
        return a.hash < b.hash;
    }
    bool operator()(const SuperStakerItemEntry &a, const uint256 &b) const
    {
        return a.hash < b;
    }
    bool operator()(const uint256 &a, const SuperStakerItemEntry &b) const
    {
        return a < b.hash;
    }
};

class SuperStakerItemPriv
{
public:
    QList<SuperStakerItemEntry> cachedSuperStakerItem;
    SuperStakerItemModel *parent;

    SuperStakerItemPriv(SuperStakerItemModel *_parent):
        parent(_parent) {}

    void refreshSuperStakerItem(interfaces::Wallet& wallet)
    {
        cachedSuperStakerItem.clear();
        {
            for(interfaces::SuperStakerInfo superStaker : wallet.getSuperStakers())
            {
                SuperStakerItemEntry superStakerItem(superStaker);
                if(parent)
                {
                    parent->updateSuperStakerData(superStakerItem);
                }
                cachedSuperStakerItem.append(superStakerItem);
            }
        }
        std::sort(cachedSuperStakerItem.begin(), cachedSuperStakerItem.end(), SuperStakerItemEntryLessThan());
    }

    void updateEntry(const SuperStakerItemEntry &item, int status)
    {
        // Find super staker in model
        QList<SuperStakerItemEntry>::iterator lower = qLowerBound(
            cachedSuperStakerItem.begin(), cachedSuperStakerItem.end(), item, SuperStakerItemEntryLessThan());
        QList<SuperStakerItemEntry>::iterator upper = qUpperBound(
            cachedSuperStakerItem.begin(), cachedSuperStakerItem.end(), item, SuperStakerItemEntryLessThan());
        int lowerIndex = (lower - cachedSuperStakerItem.begin());
        int upperIndex = (upper - cachedSuperStakerItem.begin());
        bool inModel = (lower != upper);

        switch(status)
        {
        case CT_NEW:
            if(inModel)
            {
                qWarning() << "SuperStakerItemPriv::updateEntry: Warning: Got CT_NEW, but entry is already in model";
                break;
            }
            parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
            cachedSuperStakerItem.insert(lowerIndex, item);
            parent->endInsertRows();
            break;
        case CT_UPDATED:
            if(!inModel)
            {
                qWarning() << "SuperStakerItemPriv::updateEntry: Warning: Got CT_UPDATED, but entry is not in model";
                break;
            }
            cachedSuperStakerItem[lowerIndex] = item;
            parent->emitDataChanged(lowerIndex);
            break;
        case CT_DELETED:
            if(!inModel)
            {
                qWarning() << "SuperStakerItemPriv::updateEntry: Warning: Got CT_DELETED, but entry is not in model";
                break;
            }
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedSuperStakerItem.erase(lower, upper);
            parent->endRemoveRows();
            break;
        }
    }

    int size()
    {
        return cachedSuperStakerItem.size();
    }

    SuperStakerItemEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedSuperStakerItem.size())
        {
            return &cachedSuperStakerItem[idx];
        }
        else
        {
            return 0;
        }
    }
};

SuperStakerItemModel::SuperStakerItemModel(WalletModel *parent):
    QAbstractItemModel(parent),
    walletModel(parent),
    priv(0),
    worker(0)
{
    columns << tr("Staker") << tr("Fee") << tr("Staking");

    priv = new SuperStakerItemPriv(this);
    priv->refreshSuperStakerItem(walletModel->wallet());

    worker = new SuperStakerWorker(walletModel);
    worker->moveToThread(&(t));

    t.start();

    subscribeToCoreSignals();
}

SuperStakerItemModel::~SuperStakerItemModel()
{
    unsubscribeFromCoreSignals();

    t.quit();
    t.wait();

    if(priv)
    {
        delete priv;
        priv = 0;
    }
}

QModelIndex SuperStakerItemModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    SuperStakerItemEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    return QModelIndex();
}

QModelIndex SuperStakerItemModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);
    return QModelIndex();
}

int SuperStakerItemModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int SuperStakerItemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant SuperStakerItemModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    SuperStakerItemEntry *rec = static_cast<SuperStakerItemEntry*>(index.internalPointer());

    switch (role) {
    case Qt::DisplayRole:
        switch(index.column())
        {
        case Staker:
            return rec->stakerAddress;
        case Fee:
            return rec->fee;
        case Staking:
            return rec->staking;
        default:
            break;
        }
        break;
    case SuperStakerItemModel::HashRole:
        return QString::fromStdString(rec->hash.ToString());
        break;
    case SuperStakerItemModel::StakerRole:
        return rec->stakerAddress;
        break;
    case SuperStakerItemModel::FeeRole:
        return rec->fee;
        break;
    case SuperStakerItemModel::StakingRole:
        return rec->staking;
        break;
    case SuperStakerItemModel::FormattedFeeRole:
        return formatFee(rec);
        break;
    default:
        break;
    }

    return QVariant();
}

void SuperStakerItemModel::updateSuperStakerData(const QString &hash, int status, bool showSuperStaker)
{
    // Find superStaker in wallet
    uint256 updated;
    updated.SetHex(hash.toStdString());
    interfaces::SuperStakerInfo superStaker =walletModel->wallet().getSuperStaker(updated);
    showSuperStaker &= superStaker.hash == updated;

    SuperStakerItemEntry superStakerEntry;
    if(showSuperStaker)
    {
        superStakerEntry = SuperStakerItemEntry(superStaker);
        updateSuperStakerData(superStakerEntry);
    }
    else
    {
        superStakerEntry.hash = updated;
    }
    priv->updateEntry(superStakerEntry, status);
}

void SuperStakerItemModel::checkSuperStakerChanged()
{
    if(!priv)
        return;

    // Update superStaker from contract
    for(int i = 0; i < priv->cachedSuperStakerItem.size(); i++)
    {
        SuperStakerItemEntry superStakerEntry = priv->cachedSuperStakerItem[i];
        updateSuperStakerData(superStakerEntry);
    }
}

void SuperStakerItemModel::emitDataChanged(int idx)
{
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}

struct SuperStakerNotification
{
public:
    SuperStakerNotification() {}
    SuperStakerNotification(uint256 _hash, ChangeType _status, bool _showSuperStaker):
        hash(_hash), status(_status), showSuperStaker(_showSuperStaker) {}

    void invoke(QObject *tim)
    {
        QString strHash = QString::fromStdString(hash.GetHex());
        qDebug() << "NotifySuperStakerChanged: " + strHash + " status= " + QString::number(status);

        QMetaObject::invokeMethod(tim, "updateSuperStakerData", Qt::QueuedConnection,
                                  Q_ARG(QString, strHash),
                                  Q_ARG(int, status),
                                  Q_ARG(bool, showSuperStaker));
    }
private:
    uint256 hash;
    ChangeType status;
    bool showSuperStaker;
};

static void NotifySuperStakerChanged(SuperStakerItemModel *tim, const uint256 &hash, ChangeType status)
{
    SuperStakerNotification notification(hash, status, true);
    notification.invoke(tim);
}

void SuperStakerItemModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    m_handler_superstaker_changed = walletModel->wallet().handleSuperStakerChanged(boost::bind(NotifySuperStakerChanged, this, _1, _2));
}

void SuperStakerItemModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    m_handler_superstaker_changed->disconnect();
}

void SuperStakerItemModel::updateSuperStakerData(const SuperStakerItemEntry &entry)
{
    QString hash = QString::fromStdString(entry.hash.ToString());
    QMetaObject::invokeMethod(worker, "updateSuperStakerData", Qt::QueuedConnection,
                              Q_ARG(QString, hash),
                              Q_ARG(QString, entry.stakerAddress),
                              Q_ARG(quint8, entry.fee));
}

QString SuperStakerItemModel::formatFee(const SuperStakerItemEntry *rec) const
{
    return QString("%1%").arg(rec->fee);
}
