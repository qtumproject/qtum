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
    DelegationItemEntry():
        balance(0),
        stake(0),
        weight(0),
        status(0)
    {}

    DelegationItemEntry(const interfaces::DelegationInfo &delegationInfo)
    {
        hash = delegationInfo.hash;
        createTime.setTime_t(delegationInfo.time);
        delegateAddress = QString::fromStdString(delegationInfo.delegate_address);
        stakerName = QString::fromStdString(delegationInfo.staker_name);
        stakerAddress = QString::fromStdString(delegationInfo.staker_address);
        fee = delegationInfo.fee;
        blockNumber = delegationInfo.block_number;
        createTxHash = delegationInfo.create_tx_hash;
        removeTxHash = delegationInfo.remove_tx_hash;
        balance = 0;
        stake = 0;
        weight = 0;
        status = 0;
    }

    DelegationItemEntry( const DelegationItemEntry &obj)
    {
        hash = obj.hash;
        createTime = obj.createTime;
        delegateAddress = obj.delegateAddress;
        stakerName = obj.stakerName;
        stakerAddress = obj.stakerAddress;
        fee = obj.fee;
        blockNumber = obj.blockNumber;
        createTxHash = obj.createTxHash;
        removeTxHash = obj.removeTxHash;
        balance = obj.balance;
        stake = obj.stake;
        weight = obj.weight;
        status = obj.status;
    }

    ~DelegationItemEntry()
    {}

    uint256 hash;
    QDateTime createTime;
    QString delegateAddress;
    QString stakerName;
    QString stakerAddress;
    quint8 fee;
    qint32 blockNumber;
    uint256 createTxHash;
    uint256 removeTxHash;
    qint64 balance;
    qint64 stake;
    qint64 weight;
    qint32 status;
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
        // Find delegation details
        std::string sHash = hash.toStdString();
        std::string sDelegateAddress = delegateAddress.toStdString();
        std::string sStakerAddress = stakerAddress.toStdString();
        interfaces::DelegationDetails details = walletModel->wallet().getDelegationDetails(sDelegateAddress);

        // Get delegation info
        interfaces::DelegationInfo info = details.toInfo();

        // No delegation contract, no update
        if(!details.c_contract_return)
            return;

        if(details.w_hash.ToString() == sHash)
        {
            if(details.c_entry_exist)
            {
                // Update the entry when the delegation exist
                if(details.c_delegate_address == sDelegateAddress && details.c_staker_address == sStakerAddress)
                {
                    if(details.c_fee != fee || details.c_block_number != blockNumber)
                    {
                        info.fee = details.c_fee;
                        info.block_number = details.c_block_number;
                        walletModel->wallet().addDelegationEntry(info);
                    }
                }
                // Update the entry when staker changed
                else if(details.c_delegate_address == sDelegateAddress && details.c_staker_address != sStakerAddress)
                {
                    walletModel->wallet().removeDelegationEntry(sHash);
                    info = details.toInfo(false);
                    walletModel->wallet().addDelegationEntry(info);
                }
                else
                {
                    info.block_number = -1;
                    walletModel->wallet().addDelegationEntry(info);
                }
            }
            else
            {
                // Update the entry when the delegation not exist
                if(details.w_remove_exist)
                {
                    // Remove the entry when marked for deletion
                    if(details.w_remove_in_main_chain)
                    {
                        // The entry is deleted
                        walletModel->wallet().removeDelegationEntry(sHash);
                    }
                }
                else
                {
                    // Update the entry when not marked for deletion
                    if(info.block_number != -1)
                    {
                        info.block_number = -1;
                        walletModel->wallet().addDelegationEntry(info);
                    }
                }
            }
        }

        // Get address balance
        CAmount balance = 0;
        CAmount stake = 0;
        CAmount weight = 0;

        // Get status for the create and remove transactions
        qint32 status = DelegationItemModel::NoTx;
        if(details.c_block_number > 0 && !details.w_remove_exist)
        {
            status = DelegationItemModel::CreateTxConfirmed;
        }
        else if(details.c_block_number <= 0 && details.w_create_exist)
        {
            if(details.w_create_in_mempool)
            {
                status = DelegationItemModel::CreateTxNotConfirmed;
            }
            else
            {
                status = DelegationItemModel::CreateTxError;
            }
        }
        else if(details.w_remove_exist)
        {
            if(details.w_remove_in_mempool)
            {
                status = DelegationItemModel::RemoveTxNotConfirmed;
            }
            else
            {
                status = DelegationItemModel::RemoveTxError;
            }
        }

        std::string sAddress = delegateAddress.toStdString();
        walletModel->wallet().getStakerAddressBalance(sAddress, balance, stake, weight);
        Q_EMIT itemChanged(hash, balance, stake, weight, status);
    }

Q_SIGNALS:
    // Signal that item in changed
    void itemChanged(QString hash, qint64 balance, qint64 stake, qint64 weight, qint32 status);
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
        DelegationItemEntry _item = item;

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
            _item.balance = cachedDelegationItem[lowerIndex].balance;
            _item.stake = cachedDelegationItem[lowerIndex].stake;
            _item.weight = cachedDelegationItem[lowerIndex].weight;
            _item.status = cachedDelegationItem[lowerIndex].status;
            cachedDelegationItem[lowerIndex] = _item;
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
    columns << tr("Delegate") << tr("Staker Name") << tr("Staker Address") << tr("Fee") << tr("Height") << tr("Time");

    priv = new DelegationItemPriv(this);
    priv->refreshDelegationItem(walletModel->wallet());

    worker = new DelegationWorker(walletModel);
    worker->moveToThread(&(t));
    connect(worker, &DelegationWorker::itemChanged, this, &DelegationItemModel::itemChanged);

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
        case StakerName:
            return rec->stakerName;
        case StakerAddress:
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
    case DelegationItemModel::StakerNameRole:
        return rec->stakerName;
        break;
    case DelegationItemModel::StakerAddressRole:
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
    case DelegationItemModel::FormattedFeeRole:
        return formatFee(rec);
        break;
    case DelegationItemModel::BalanceRole:
        return rec->balance;
        break;
    case DelegationItemModel::StakeRole:
        return rec->stake;
        break;
    case DelegationItemModel::WeightRole:
        return rec->weight;
        break;
    case DelegationItemModel::FormattedWeightRole:
        return rec->weight / COIN;
        break;
    case DelegationItemModel::TxStatusRole:
        return rec->status;
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
    m_handler_delegation_changed = walletModel->wallet().handleDelegationChanged(boost::bind(NotifyDelegationChanged, this, boost::placeholders::_1, boost::placeholders::_2));
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

QString DelegationItemModel::formatFee(const DelegationItemEntry *rec) const
{
    return QString("%1%").arg(rec->fee);
}

void DelegationItemModel::itemChanged(QString hash, qint64 balance, qint64 stake, qint64 weight, qint32 status)
{
    if(!priv)
        return;

    uint256 updated;
    updated.SetHex(hash.toStdString());

    // Update delegation
    for(int i = 0; i < priv->cachedDelegationItem.size(); i++)
    {
        DelegationItemEntry delegationEntry = priv->cachedDelegationItem[i];
        if(delegationEntry.hash == updated)
        {
            delegationEntry.balance = balance;
            delegationEntry.stake = stake;
            delegationEntry.weight = weight;
            delegationEntry.status = status;
            priv->cachedDelegationItem[i] = delegationEntry;
            priv->updateEntry(delegationEntry, CT_UPDATED);
        }
    }
}
