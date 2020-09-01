#include <qt/superstakeritemmodel.h>
#include <qt/walletmodel.h>
#include <interfaces/wallet.h>
#include <validation.h>
#include <qt/bitcoinunits.h>
#include <interfaces/node.h>
#include <interfaces/handler.h>
#include <wallet/wallet.h>
#include <algorithm>

#include <QDateTime>
#include <QFont>
#include <QDebug>
#include <QThread>

class SuperStakerItemEntry
{
public:
    SuperStakerItemEntry():
        staking(false),
        balance(0),
        stake(0),
        weight(0),
        delegationsWeight(0)
    {}

    SuperStakerItemEntry(const interfaces::SuperStakerInfo &superStakerInfo)
    {
        hash = superStakerInfo.hash;
        stakerName = QString::fromStdString(superStakerInfo.staker_name);
        stakerAddress = QString::fromStdString(superStakerInfo.staker_address);
        minFee = superStakerInfo.custom_config ? superStakerInfo.min_fee : DEFAULT_STAKING_MIN_FEE;
        staking = false;
        balance = 0;
        stake = 0;
        weight = 0;
        delegationsWeight = 0;
        createTime.setTime_t(superStakerInfo.time);
    }

    SuperStakerItemEntry( const SuperStakerItemEntry &obj)
    {
        hash = obj.hash;
        stakerName = obj.stakerName;
        stakerAddress = obj.stakerAddress;
        minFee = obj.minFee;
        staking = obj.staking;
        balance = obj.balance;
        stake = obj.stake;
        weight = obj.weight;
        delegationsWeight = obj.delegationsWeight;
        createTime = obj.createTime;
    }

    ~SuperStakerItemEntry()
    {}

    uint256 hash;
    QString stakerName;
    QString stakerAddress;
    quint8 minFee;
    bool staking;
    qint64 balance;
    qint64 stake;
    qint64 weight;
    qint64 delegationsWeight;
    QDateTime createTime;
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
    void updateSuperStakerData(QString hash, QString stakerAddress)
    {
        // Get address balance
        bool staking = false;
        CAmount balance = 0;
        CAmount stake = 0;
        CAmount weight = 0;
        CAmount delegationsWeight = 0;
        std::string sAddress = stakerAddress.toStdString();
        uint256 id;
        id.SetHex(hash.toStdString());
        staking = walletModel->wallet().isSuperStakerStaking(id, delegationsWeight);
        walletModel->wallet().getStakerAddressBalance(sAddress, balance, stake, weight);
        Q_EMIT itemChanged(hash, balance, stake, weight, delegationsWeight, staking);
    }

Q_SIGNALS:
    // Signal that item in changed
    void itemChanged(QString hash, qint64 balance, qint64 stake, qint64 weight, qint64 delegationsWeight, bool staking);
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
        SuperStakerItemEntry _item = item;

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
            _item.balance = cachedSuperStakerItem[lowerIndex].balance;
            _item.stake = cachedSuperStakerItem[lowerIndex].stake;
            _item.weight = cachedSuperStakerItem[lowerIndex].weight;
            _item.delegationsWeight = cachedSuperStakerItem[lowerIndex].delegationsWeight;
            _item.staking = cachedSuperStakerItem[lowerIndex].staking;
            cachedSuperStakerItem[lowerIndex] = _item;
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
    columns << tr("Staker Name") << tr("Staker Address") << tr("Minimum Fee") << tr("Staking");

    priv = new SuperStakerItemPriv(this);
    priv->refreshSuperStakerItem(walletModel->wallet());

    worker = new SuperStakerWorker(walletModel);
    worker->moveToThread(&(t));
    connect(worker, &SuperStakerWorker::itemChanged, this, &SuperStakerItemModel::itemChanged);

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
        case StakerName:
            return rec->stakerName;
        case StakerAddress:
            return rec->stakerAddress;
        case MinFee:
            return rec->minFee;
        case Staking:
            return rec->staking;
        case Time:
            return rec->createTime;
        default:
            break;
        }
        break;
    case SuperStakerItemModel::HashRole:
        return QString::fromStdString(rec->hash.ToString());
        break;
    case SuperStakerItemModel::StakerNameRole:
        return rec->stakerName;
        break;
    case SuperStakerItemModel::StakerAddressRole:
        return rec->stakerAddress;
        break;
    case SuperStakerItemModel::MinFeeRole:
        return rec->minFee;
        break;
    case SuperStakerItemModel::StakingRole:
        return rec->staking;
        break;
    case SuperStakerItemModel::FormattedMinFeeRole:
        return formatMinFee(rec);
        break;
    case SuperStakerItemModel::BalanceRole:
        return rec->balance;
        break;
    case SuperStakerItemModel::StakeRole:
        return rec->stake;
        break;
    case SuperStakerItemModel::WeightRole:
        return rec->weight;
        break;
    case SuperStakerItemModel::FormattedWeightRole:
        return rec->weight / COIN;
        break;
    case SuperStakerItemModel::DelegationsWeightRole:
        return rec->delegationsWeight;
        break;
    case SuperStakerItemModel::FormattedDelegationsWeightRole:
        return rec->delegationsWeight / COIN;
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
    m_handler_superstaker_changed = walletModel->wallet().handleSuperStakerChanged(boost::bind(NotifySuperStakerChanged, this, boost::placeholders::_1, boost::placeholders::_2));
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
                              Q_ARG(QString, entry.stakerAddress));
}

QString SuperStakerItemModel::formatMinFee(const SuperStakerItemEntry *rec) const
{
    return QString("%1%").arg(rec->minFee);
}

void SuperStakerItemModel::itemChanged(QString hash, qint64 balance, qint64 stake, qint64 weight, qint64 delegationsWeight, bool staking)
{
    if(!priv)
        return;

    uint256 updated;
    updated.SetHex(hash.toStdString());

    // Update delegation
    for(int i = 0; i < priv->cachedSuperStakerItem.size(); i++)
    {
        SuperStakerItemEntry superStakerEntry = priv->cachedSuperStakerItem[i];
        if(superStakerEntry.hash == updated)
        {
            superStakerEntry.balance = balance;
            superStakerEntry.stake = stake;
            superStakerEntry.weight = weight;
            superStakerEntry.delegationsWeight = delegationsWeight;
            superStakerEntry.staking = staking;
            priv->cachedSuperStakerItem[i] = superStakerEntry;
            priv->updateEntry(superStakerEntry, CT_UPDATED);
        }
    }
}
