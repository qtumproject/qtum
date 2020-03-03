#include <qt/delegationitemmodel.h>
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

class DelegationItemEntry
{
public:
    DelegationItemEntry()
    {}

    DelegationItemEntry(const interfaces::DelegationInfo &delegationInfo)
    {
        hash = delegationInfo.hash;
        createTime.setTime_t(delegationInfo.time);
        delegateAddress = QString::fromStdString(delegationInfo.delegate_address);
        stakerAddress = QString::fromStdString(delegationInfo.staker_address);
        fee = delegationInfo.fee;
        blockNumber = delegationInfo.block_number;
        createTxHash = delegationInfo.create_tx_hash;
        removeTxHash = delegationInfo.remove_tx_hash;
    }

    DelegationItemEntry( const DelegationItemEntry &obj)
    {
        hash = obj.hash;
        createTime = obj.createTime;
        delegateAddress = obj.delegateAddress;
        stakerAddress = obj.stakerAddress;
        fee = obj.fee;
        blockNumber = obj.blockNumber;
        createTxHash = obj.createTxHash;
        removeTxHash = obj.removeTxHash;
    }

    ~DelegationItemEntry()
    {}

    uint256 hash;
    QDateTime createTime;
    QString delegateAddress;
    QString stakerAddress;
    quint8 fee;
    qint32 blockNumber;
    uint256 createTxHash;
    uint256 removeTxHash;
};

class DelegationWorker : public QObject
{
    Q_OBJECT
public:
    WalletModel *walletModel;
    bool first;
    DelegationWorker(WalletModel *_walletModel):
        walletModel(_walletModel), first(true) {}

private Q_SLOTS:
    void updateDelegationData(QString hash, QString delegateAddress, QString stakerAddress, quint8 fee, qint32 blockNumber)
    {
        // Find delegation in contract
        std::string sHash = hash.toStdString();
        std::string sDelegateAddress = delegateAddress.toStdString();
        std::string sStakerAddress = stakerAddress.toStdString();
        bool validated = false;
        bool contractRet = false;
        interfaces::DelegationInfo delegation =walletModel->wallet().getDelegationContract(sHash, validated, contractRet);

        // No delegation contract, no update
        if(!contractRet)
            return;

        if(validated && delegation.delegate_address == sDelegateAddress && delegation.staker_address == sStakerAddress)
        {
            // Update delegation entry
            if(delegation.fee != fee || delegation.block_number != blockNumber)
            {
                walletModel->wallet().addDelegationEntry(delegation);
            }
        }
        else if(delegation.remove_tx_hash != uint256())
        {
            // The entry is deleted
            walletModel->wallet().removeDelegationEntry(sHash);
        }
    }

Q_SIGNALS:
};

#include <qt/delegationitemmodel.moc>

struct DelegationItemEntryLessThan
{
    bool operator()(const DelegationItemEntry &a, const DelegationItemEntry &b) const
    {
        return a.hash < b.hash;
    }
    bool operator()(const DelegationItemEntry &a, const uint256 &b) const
    {
        return a.hash < b;
    }
    bool operator()(const uint256 &a, const DelegationItemEntry &b) const
    {
        return a < b.hash;
    }
};

class DelegationItemPriv
{
public:
    QList<DelegationItemEntry> cachedDelegationItem;
    DelegationItemModel *parent;

    DelegationItemPriv(DelegationItemModel *_parent):
        parent(_parent) {}

    void refreshDelegationItem(interfaces::Wallet& wallet)
    {
        cachedDelegationItem.clear();
        {
            for(interfaces::DelegationInfo delegation : wallet.getDelegations())
            {
                DelegationItemEntry delegationItem(delegation);
                if(parent)
                {
                    parent->updateDelegationData(delegationItem);
                }
                cachedDelegationItem.append(delegationItem);
            }
        }
        std::sort(cachedDelegationItem.begin(), cachedDelegationItem.end(), DelegationItemEntryLessThan());
    }

    void updateEntry(const DelegationItemEntry &item, int status)
    {
        // Find delegation in model
        QList<DelegationItemEntry>::iterator lower = qLowerBound(
            cachedDelegationItem.begin(), cachedDelegationItem.end(), item, DelegationItemEntryLessThan());
        QList<DelegationItemEntry>::iterator upper = qUpperBound(
            cachedDelegationItem.begin(), cachedDelegationItem.end(), item, DelegationItemEntryLessThan());
        int lowerIndex = (lower - cachedDelegationItem.begin());
        int upperIndex = (upper - cachedDelegationItem.begin());
        bool inModel = (lower != upper);

        switch(status)
        {
        case CT_NEW:
            if(inModel)
            {
                qWarning() << "DelegationItemPriv::updateEntry: Warning: Got CT_NEW, but entry is already in model";
                break;
            }
            parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
            cachedDelegationItem.insert(lowerIndex, item);
            parent->endInsertRows();
            break;
        case CT_UPDATED:
            if(!inModel)
            {
                qWarning() << "DelegationItemPriv::updateEntry: Warning: Got CT_UPDATED, but entry is not in model";
                break;
            }
            cachedDelegationItem[lowerIndex] = item;
            parent->emitDataChanged(lowerIndex);
            break;
        case CT_DELETED:
            if(!inModel)
            {
                qWarning() << "DelegationItemPriv::updateEntry: Warning: Got CT_DELETED, but entry is not in model";
                break;
            }
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedDelegationItem.erase(lower, upper);
            parent->endRemoveRows();
            break;
        }
    }

    int size()
    {
        return cachedDelegationItem.size();
    }

    DelegationItemEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedDelegationItem.size())
        {
            return &cachedDelegationItem[idx];
        }
        else
        {
            return 0;
        }
    }
};

DelegationItemModel::DelegationItemModel(WalletModel *parent):
    QAbstractItemModel(parent),
    walletModel(parent),
    priv(0),
    worker(0)
{
    columns << tr("Delegate") << tr("Staker") << tr("Fee") << tr("Height") << tr("Time");

    priv = new DelegationItemPriv(this);
    priv->refreshDelegationItem(walletModel->wallet());

    worker = new DelegationWorker(walletModel);
    worker->moveToThread(&(t));

    t.start();

    subscribeToCoreSignals();
}

DelegationItemModel::~DelegationItemModel()
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

QModelIndex DelegationItemModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    DelegationItemEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    return QModelIndex();
}

QModelIndex DelegationItemModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);
    return QModelIndex();
}

int DelegationItemModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int DelegationItemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant DelegationItemModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    DelegationItemEntry *rec = static_cast<DelegationItemEntry*>(index.internalPointer());

    switch (role) {
    case Qt::DisplayRole:
        switch(index.column())
        {
        case Address:
            return rec->delegateAddress;
        case Staker:
            return rec->stakerAddress;
        case Fee:
            return rec->fee;
        case Height:
            return rec->blockNumber;
        case Time:
            return rec->createTime;
        default:
            break;
        }
        break;
    case DelegationItemModel::HashRole:
        return QString::fromStdString(rec->hash.ToString());
        break;
    case DelegationItemModel::AddressRole:
        return rec->delegateAddress;
        break;
    case DelegationItemModel::StakerRole:
        return rec->stakerAddress;
        break;
    case DelegationItemModel::FeeRole:
        return rec->fee;
        break;
    case DelegationItemModel::BlockHeightRole:
        return rec->blockNumber;
        break;
    case DelegationItemModel::CreateTxHashRole:
        return QString::fromStdString(rec->createTxHash.ToString());
        break;
    case DelegationItemModel::RemoveTxHashRole:
        return QString::fromStdString(rec->removeTxHash.ToString());
        break;
    default:
        break;
    }

    return QVariant();
}

void DelegationItemModel::updateDelegationData(const QString &hash, int status, bool showDelegation)
{
    // Find delegation in wallet
    uint256 updated;
    updated.SetHex(hash.toStdString());
    interfaces::DelegationInfo delegation =walletModel->wallet().getDelegation(updated);
    showDelegation &= delegation.hash == updated;

    DelegationItemEntry delegationEntry;
    if(showDelegation)
    {
        delegationEntry = DelegationItemEntry(delegation);
        updateDelegationData(delegationEntry);
    }
    else
    {
        delegationEntry.hash = updated;
    }
    priv->updateEntry(delegationEntry, status);
}

void DelegationItemModel::checkDelegationChanged()
{
    if(!priv)
        return;

    // Update delegation from contract
    for(int i = 0; i < priv->cachedDelegationItem.size(); i++)
    {
        DelegationItemEntry delegationEntry = priv->cachedDelegationItem[i];
        updateDelegationData(delegationEntry);
    }
}

void DelegationItemModel::emitDataChanged(int idx)
{
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}

struct DelegationNotification
{
public:
    DelegationNotification() {}
    DelegationNotification(uint256 _hash, ChangeType _status, bool _showDelegation):
        hash(_hash), status(_status), showDelegation(_showDelegation) {}

    void invoke(QObject *tim)
    {
        QString strHash = QString::fromStdString(hash.GetHex());
        qDebug() << "NotifyDelegationChanged: " + strHash + " status= " + QString::number(status);

        QMetaObject::invokeMethod(tim, "updateDelegationData", Qt::QueuedConnection,
                                  Q_ARG(QString, strHash),
                                  Q_ARG(int, status),
                                  Q_ARG(bool, showDelegation));
    }
private:
    uint256 hash;
    ChangeType status;
    bool showDelegation;
};

static void NotifyDelegationChanged(DelegationItemModel *tim, const uint256 &hash, ChangeType status)
{
    DelegationNotification notification(hash, status, true);
    notification.invoke(tim);
}

void DelegationItemModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    m_handler_delegation_changed = walletModel->wallet().handleDelegationChanged(boost::bind(NotifyDelegationChanged, this, _1, _2));
}

void DelegationItemModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    m_handler_delegation_changed->disconnect();
}

void DelegationItemModel::updateDelegationData(const DelegationItemEntry &entry)
{
    QString hash = QString::fromStdString(entry.hash.ToString());
    QMetaObject::invokeMethod(worker, "updateDelegationData", Qt::QueuedConnection,
                              Q_ARG(QString, hash),
                              Q_ARG(QString, entry.delegateAddress),
                              Q_ARG(QString, entry.stakerAddress),
                              Q_ARG(quint8, entry.fee),
                              Q_ARG(qint32, entry.blockNumber));
}
