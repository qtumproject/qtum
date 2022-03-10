#include <qt/nftitemmodel.h>
#include <qt/nft.h>
#include <qt/walletmodel.h>
#include <interfaces/wallet.h>
#include <validation.h>
#include <qt/bitcoinunits.h>
#include <interfaces/node.h>
#include <interfaces/handler.h>
#include <algorithm>
#include <consensus/consensus.h>
#include <chainparams.h>

#include <QDateTime>
#include <QFont>
#include <QDebug>
#include <QThread>

class NftItemEntry
{
public:
    NftItemEntry()
    {}

    NftItemEntry(const interfaces::NftInfo &nftInfo)
    {
        hash = nftInfo.hash;
        createTime.setTime_t(nftInfo.time);
        contractAddress = QString::fromStdString(nftInfo.contract_address);
        nftName = QString::fromStdString(nftInfo.nft_name);
        senderAddress = QString::fromStdString(nftInfo.sender_address);
    }

    NftItemEntry( const NftItemEntry &obj)
    {
        hash = obj.hash;
        createTime = obj.createTime;
        contractAddress = obj.contractAddress;
        nftName = obj.nftName;
        senderAddress = obj.senderAddress;
        balance = obj.balance;
    }

    ~NftItemEntry()
    {}

    uint256 hash;
    QDateTime createTime;
    QString contractAddress;
    QString nftName;
    QString senderAddress;
    int256_t balance;
};

class NftTxWorker : public QObject
{
    Q_OBJECT
public:
    WalletModel *walletModel;
    bool first;
    Nft nftAbi;
    NftTxWorker(WalletModel *_walletModel):
        walletModel(_walletModel), first(true) {}

private Q_SLOTS:
    void updateNftTx(const QString &hash)
    {
        if(walletModel && walletModel->node().shutdownRequested())
            return;

        // Initialize variables
        uint256 nftHash = uint256S(hash.toStdString());
        int64_t fromBlock = 0;
        int64_t toBlock = -1;
        interfaces::NftInfo nftInfo;
        uint256 blockHash;
        bool found = false;

        int64_t backInPast = first ? Params().GetConsensus().MaxCheckpointSpan() : 10;
        first = false;

        // Get current height and block hash
        toBlock = walletModel->node().getNumBlocks();
        blockHash = walletModel->node().getBlockHash(toBlock);

        if(toBlock > -1)
        {
            // Find the nft tx in the wallet
            nftInfo = walletModel->wallet().getNft(nftHash);
            found = nftInfo.hash == nftHash;
            if(found)
            {
                // Get the start location for search the event log
                if(nftInfo.block_number < toBlock)
                {
                    if(walletModel->node().getBlockHash(nftInfo.block_number) == nftInfo.block_hash)
                    {
                        fromBlock = nftInfo.block_number;
                    }
                    else
                    {
                        fromBlock = nftInfo.block_number - backInPast;
                    }
                }
                else
                {
                    fromBlock = toBlock - backInPast;
                }
                if(fromBlock < 0)
                    fromBlock = 0;

                nftInfo.block_hash = blockHash;
                nftInfo.block_number = toBlock;
            }
        }

        if(found)
        {
            // List the events and update the nft tx
            std::vector<NftEvent> nftEvents;
            nftAbi.setAddress(nftInfo.contract_address);
            nftAbi.setSender(nftInfo.sender_address);
            nftAbi.transferEvents(nftEvents, fromBlock, toBlock);
            for(size_t i = 0; i < nftEvents.size(); i++)
            {
                NftEvent event = nftEvents[i];
                interfaces::NftTx nftTx;
                nftTx.contract_address = event.address;
                nftTx.sender_address = event.sender;
                nftTx.receiver_address = event.receiver;
                nftTx.value = event.value;
                nftTx.tx_hash = event.transactionHash;
                nftTx.block_hash = event.blockHash;
                nftTx.block_number = event.blockNumber;
                walletModel->wallet().addNftTxEntry(nftTx, false);
            }

            walletModel->wallet().addNftEntry(nftInfo);
        }
    }

    void updateBalance(QString hash, QString contractAddress, QString senderAddress)
    {
        if(walletModel && walletModel->node().shutdownRequested())
            return;

        nftAbi.setAddress(contractAddress.toStdString());
        nftAbi.setSender(senderAddress.toStdString());
        std::string strBalance;
        if(nftAbi.balanceOf(strBalance))
        {
            QString balance = QString::fromStdString(strBalance);
            Q_EMIT balanceChanged(hash, balance);
        }
    }

Q_SIGNALS:
    // Signal that balance in nft changed
    void balanceChanged(QString hash, QString balance);
};

#include <qt/nftitemmodel.moc>

struct NftItemEntryLessThan
{
    bool operator()(const NftItemEntry &a, const NftItemEntry &b) const
    {
        return a.hash < b.hash;
    }
    bool operator()(const NftItemEntry &a, const uint256 &b) const
    {
        return a.hash < b;
    }
    bool operator()(const uint256 &a, const NftItemEntry &b) const
    {
        return a < b.hash;
    }
};

class NftItemPriv
{
public:
    QList<NftItemEntry> cachedNftItem;
    NftItemModel *parent;

    NftItemPriv(NftItemModel *_parent):
        parent(_parent) {}

    void refreshNftItem(interfaces::Wallet& wallet)
    {
        cachedNftItem.clear();
        {
            for(interfaces::NftInfo nft : wallet.getNfts())
            {
                NftItemEntry nftItem(nft);
                if(parent)
                {
                    parent->updateBalance(nftItem);
                }
                cachedNftItem.append(nftItem);
            }
        }
        std::sort(cachedNftItem.begin(), cachedNftItem.end(), NftItemEntryLessThan());
    }

    void updateEntry(const NftItemEntry &_item, int status)
    {
        // Find address / label in model
        NftItemEntry item;
        QList<NftItemEntry>::iterator lower = qLowerBound(
            cachedNftItem.begin(), cachedNftItem.end(), _item, NftItemEntryLessThan());
        QList<NftItemEntry>::iterator upper = qUpperBound(
            cachedNftItem.begin(), cachedNftItem.end(), _item, NftItemEntryLessThan());
        int lowerIndex = (lower - cachedNftItem.begin());
        int upperIndex = (upper - cachedNftItem.begin());
        bool inModel = (lower != upper);
        item = _item;
        if(inModel)
        {
            item.balance = cachedNftItem[lowerIndex].balance;
        }

        switch(status)
        {
        case CT_NEW:
            if(inModel)
            {
                qWarning() << "NftItemPriv::updateEntry: Warning: Got CT_NEW, but entry is already in model";
                break;
            }
            parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
            cachedNftItem.insert(lowerIndex, item);
            parent->endInsertRows();
            break;
        case CT_UPDATED:
            if(!inModel)
            {
                qWarning() << "NftItemPriv::updateEntry: Warning: Got CT_UPDATED, but entry is not in model";
                break;
            }
            cachedNftItem[lowerIndex] = item;
            parent->emitDataChanged(lowerIndex);
            break;
        case CT_DELETED:
            if(!inModel)
            {
                qWarning() << "NftItemPriv::updateEntry: Warning: Got CT_DELETED, but entry is not in model";
                break;
            }
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedNftItem.erase(lower, upper);
            parent->endRemoveRows();
            break;
        }
    }

    int updateBalance(QString hash, QString balance)
    {
        uint256 updated;
        updated.SetHex(hash.toStdString());
        int256_t val(balance.toStdString());

        for(int i = 0; i < cachedNftItem.size(); i++)
        {
            NftItemEntry item = cachedNftItem[i];
            if(item.hash == updated && item.balance != val)
            {
                item.balance = val;
                cachedNftItem[i] = item;
                return i;
            }
        }

        return -1;
    }

    int size()
    {
        return cachedNftItem.size();
    }

    NftItemEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedNftItem.size())
        {
            return &cachedNftItem[idx];
        }
        else
        {
            return 0;
        }
    }
};

NftItemModel::NftItemModel(WalletModel *parent):
    QAbstractItemModel(parent),
    walletModel(parent),
    priv(0),
    worker(0),
    nftTxCleaned(false)
{
    columns << tr("Nft Name") << tr("Balance");

    priv = new NftItemPriv(this);
    priv->refreshNftItem(walletModel->wallet());

    worker = new NftTxWorker(walletModel);
    worker->nftAbi.setModel(walletModel);
    worker->moveToThread(&(t));
    connect(worker, &NftTxWorker::balanceChanged, this, &NftItemModel::balanceChanged);

    t.start();

    subscribeToCoreSignals();
}

NftItemModel::~NftItemModel()
{
    unsubscribeFromCoreSignals();

    join();

    if(priv)
    {
        delete priv;
        priv = 0;
    }
}

QModelIndex NftItemModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    NftItemEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    return QModelIndex();
}

QModelIndex NftItemModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);
    return QModelIndex();
}

int NftItemModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int NftItemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant NftItemModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    NftItemEntry *rec = static_cast<NftItemEntry*>(index.internalPointer());

    switch (role) {
    case Qt::DisplayRole:
        switch(index.column())
        {
        case Name:
            return rec->nftName;
        case Balance:
            return BitcoinUnits::formatInt256(rec->balance, false, BitcoinUnits::SeparatorStyle::ALWAYS);
        default:
            break;
        }
        break;
    case NftItemModel::HashRole:
        return QString::fromStdString(rec->hash.ToString());
        break;
    case NftItemModel::AddressRole:
        return rec->contractAddress;
        break;
    case NftItemModel::NameRole:
        return rec->nftName;
        break;
    case NftItemModel::SenderRole:
        return rec->senderAddress;
        break;
    case NftItemModel::BalanceRole:
        return BitcoinUnits::formatInt256(rec->balance, false, BitcoinUnits::SeparatorStyle::ALWAYS);
        break;
    case NftItemModel::RawBalanceRole:
        return QString::fromStdString(rec->balance.str());
        break;
    default:
        break;
    }

    return QVariant();
}

void NftItemModel::updateNft(const QString &hash, int status, bool showNft)
{
    // Find nft in wallet
    uint256 updated;
    updated.SetHex(hash.toStdString());
    interfaces::NftInfo nft =walletModel->wallet().getNft(updated);
    showNft &= nft.hash == updated;

    NftItemEntry nftEntry;
    if(showNft)
    {
        nftEntry = NftItemEntry(nft);
        updateBalance(nftEntry);
    }
    else
    {
        nftEntry.hash = updated;
    }
    priv->updateEntry(nftEntry, status);
}

void NftItemModel::checkNftBalanceChanged()
{
    if(!priv)
        return;

    // Update nft balance
    for(int i = 0; i < priv->cachedNftItem.size(); i++)
    {
        NftItemEntry nftEntry = priv->cachedNftItem[i];
        updateBalance(nftEntry);
    }

    // Update nft transactions
    if(fLogEvents)
    {
        // Search for nft transactions
        for(int i = 0; i < priv->cachedNftItem.size(); i++)
        {
            NftItemEntry nftEntry = priv->cachedNftItem[i];
            QString hash = QString::fromStdString(nftEntry.hash.ToString());
            QMetaObject::invokeMethod(worker, "updateNftTx", Qt::QueuedConnection,
                                      Q_ARG(QString, hash));
        }
    }
}

void NftItemModel::emitDataChanged(int idx)
{
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}

struct NftNotification
{
public:
    NftNotification() {}
    NftNotification(uint256 _hash, ChangeType _status, bool _showNft):
        hash(_hash), status(_status), showNft(_showNft) {}

    void invoke(QObject *tim)
    {
        QString strHash = QString::fromStdString(hash.GetHex());
        qDebug() << "NotifyNftChanged: " + strHash + " status= " + QString::number(status);

        QMetaObject::invokeMethod(tim, "updateNft", Qt::QueuedConnection,
                                  Q_ARG(QString, strHash),
                                  Q_ARG(int, status),
                                  Q_ARG(bool, showNft));
    }
private:
    uint256 hash;
    ChangeType status;
    bool showNft;
};

static void NotifyNftChanged(NftItemModel *tim, const uint256 &hash, ChangeType status)
{
    NftNotification notification(hash, status, true);
    notification.invoke(tim);
}

void NftItemModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    m_handler_nft_changed = walletModel->wallet().handleNftChanged(boost::bind(NotifyNftChanged, this, boost::placeholders::_1, boost::placeholders::_2));
}

void NftItemModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    m_handler_nft_changed->disconnect();
}

void NftItemModel::balanceChanged(QString hash, QString balance)
{
    int index = priv->updateBalance(hash, balance);
    if(index > -1)
    {
        emitDataChanged(index);
    }
}

void NftItemModel::updateBalance(const NftItemEntry &entry)
{
    QString hash = QString::fromStdString(entry.hash.ToString());
    QMetaObject::invokeMethod(worker, "updateBalance", Qt::QueuedConnection,
                              Q_ARG(QString, hash), Q_ARG(QString, entry.contractAddress), Q_ARG(QString, entry.senderAddress));
}

void NftItemModel::join()
{
    if(t.isRunning())
    {
        if(worker)
            worker->disconnect(this);
        t.quit();
        t.wait();
    }
}
