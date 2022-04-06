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
#include <qt/guiutil.h>

#include <QDateTime>
#include <QFont>
#include <QDebug>
#include <QThread>
#include <QBuffer>

class NftItemEntry
{
public:
    NftItemEntry()
    {}

    NftItemEntry(const interfaces::NftInfo &nftInfo)
    {
        hash = nftInfo.hash;
        createTime.setTime_t(nftInfo.create_time);
        name = QString::fromStdString(nftInfo.name);
        owner = QString::fromStdString(nftInfo.owner);
        url = QString::fromStdString(nftInfo.url);
        desc = QString::fromStdString(nftInfo.desc);
        balance = nftInfo.count;
        id = nftInfo.id;
        NFTId = nftInfo.NFTId;
        thumbnail = QString::fromStdString(nftInfo.thumbnail);
    }

    NftItemEntry( const NftItemEntry &obj)
    {
        hash = obj.hash;
        createTime = obj.createTime;
        name = obj.name;
        owner = obj.owner;
        url = obj.url;
        desc = obj.desc;
        balance = obj.balance;
        id = obj.id;
        NFTId = obj.NFTId;
        thumbnail = obj.thumbnail;
    }

    ~NftItemEntry()
    {}

    uint256 hash;
    QDateTime createTime;
    QString name;
    QString owner;
    QString url;
    QString desc;
    int32_t balance;
    uint256 id;
    uint256 NFTId;
    QString thumbnail;
};

class NftTxWorker : public QObject
{
    Q_OBJECT
public:
    WalletModel *walletModel;
    Nft nftAbi;
    int64_t fromBlock = 0;
    int64_t toBlock = -1;
    bool initThumbnail = true;
    QMap<QString, QString> thumbnailCache;
    QMap<QString, QDateTime> thumbnailTime;
    int thumbnailSize = 70; // 70 pix
    int thumbnailRecheck = 600; // 10 min
    NftTxWorker(WalletModel *_walletModel):
        walletModel(_walletModel) {}

private:
    void cacheThumbnail()
    {
        if(initThumbnail)
        {
            initThumbnail = false;
            for(interfaces::NftInfo nft : walletModel->wallet().getNfts())
            {
                QString url = QString::fromStdString(nft.url);
                QString thumbnail = QString::fromStdString(nft.thumbnail);
                if(!thumbnail.isEmpty())
                {
                    thumbnailCache[url] = thumbnail;
                }
            }
        }
    }

    bool checkThumbnailTime(const QString& hash)
    {
        QDateTime now = QDateTime::currentDateTime();
        bool ret = false;
        if(!thumbnailTime.contains(hash))
        {
            ret = true;
        }
        else
        {

            QDateTime time = thumbnailTime[hash];
            ret = time.secsTo(now) > thumbnailRecheck;
        }
        return ret;
    }

private Q_SLOTS:
    void updateNftTx()
    {
        if(walletModel && walletModel->node().shutdownRequested())
            return;

        // Get current height and block hash
        toBlock = walletModel->node().getNumBlocks();
        if(fromBlock < toBlock)
        {
            // List the events and update the nft tx
            std::vector<NftEvent> nftEvents;
            std::vector<interfaces::NftTx> nftTxs;
            nftAbi.transferEvents(nftEvents, fromBlock, toBlock);
            for(size_t i = 0; i < nftEvents.size(); i++)
            {
                NftEvent event = nftEvents[i];
                interfaces::NftTx nftTx;
                nftTx.sender = event.sender;
                nftTx.receiver = event.receiver;
                nftTx.id = event.id;
                nftTx.value = event.value;
                nftTx.tx_hash = event.transactionHash;
                nftTx.block_hash = event.blockHash;
                nftTx.block_number = event.blockNumber;
                nftTxs.push_back(nftTx);
            }
            walletModel->wallet().addNftTxEntries(nftTxs);
            fromBlock = toBlock - 10;
            if(fromBlock < 0) fromBlock = 0;
        }

        std::map<uint256, WalletNFTInfo> listNftInfo;
        for(interfaces::NftInfo nft : walletModel->wallet().getRawNftFromTx())
        {
            bool isOk = true;
            WalletNFTInfo info;
            auto search = listNftInfo.find(nft.id);
            nftAbi.setSender(nft.owner);
            if(search != listNftInfo.end())
            {
                info = search->second;
            }
            else
            {
                isOk &= nftAbi.walletNFTList(info, nft.id);
                if(isOk)
                {
                    listNftInfo[nft.id] = info;
                }
            }

            if(!isOk) continue;
            nft.NFTId = info.NFTId;
            nft.name = info.name;
            nft.url = info.url;
            nft.desc = info.desc;
            nft.create_time = info.createAt;
            int32_t count = 0;
            isOk &= nftAbi.balanceOf(nft.id, count);

            if(!isOk) continue;
            nft.count = count;
            walletModel->wallet().addNftEntry(nft);
        }
    }

    void updateThumbnail()
    {
        cacheThumbnail();

        for(interfaces::NftInfo nft : walletModel->wallet().getNfts())
        {
            QString hash = QString::fromStdString(nft.hash.ToString());
            QString url = QString::fromStdString(nft.url);
            if(nft.count > 0 && nft.thumbnail == "" &&
                    GUIUtil::HasPixmapForUrl(url) && checkThumbnailTime(hash))
            {
                QString thumbnail;
                if(thumbnailCache.contains(url))
                {
                    thumbnail = thumbnailCache[url];
                }
                else
                {
                    QPixmap pixmap;
                    if(GUIUtil::GetPixmapFromUrl(pixmap, url, thumbnailSize, thumbnailSize))
                    {
                        QByteArray data;
                        QBuffer buffer(&data);
                        buffer.open(QIODevice::WriteOnly);
                        pixmap.save(&buffer, "PNG");
                        thumbnail = data.toBase64();
                        if(!thumbnail.isEmpty())
                        {
                            thumbnailCache[url] = thumbnail;
                        }
                    }
                }

                if(!thumbnail.isEmpty())
                {
                    nft.thumbnail = thumbnail.toStdString();
                    walletModel->wallet().addNftEntry(nft);
                }
                QDateTime now = QDateTime::currentDateTime();
                thumbnailTime[hash] = now;
            }
        }
    }
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
                cachedNftItem.append(nftItem);
            }
        }
        std::sort(cachedNftItem.begin(), cachedNftItem.end(), NftItemEntryLessThan());
    }

    void updateEntry(const NftItemEntry &item, int status)
    {
        // Find address / label in model
        QList<NftItemEntry>::iterator lower = qLowerBound(
            cachedNftItem.begin(), cachedNftItem.end(), item, NftItemEntryLessThan());
        QList<NftItemEntry>::iterator upper = qUpperBound(
            cachedNftItem.begin(), cachedNftItem.end(), item, NftItemEntryLessThan());
        int lowerIndex = (lower - cachedNftItem.begin());
        int upperIndex = (upper - cachedNftItem.begin());
        bool inModel = (lower != upper);

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
            return rec->name;
        case Balance:
            return QString::number(rec->balance);
        default:
            break;
        }
        break;
    case NftItemModel::HashRole:
        return QString::fromStdString(rec->hash.ToString());
        break;
    case NftItemModel::NameRole:
        return rec->name;
        break;
    case NftItemModel::OwnerRole:
        return rec->owner;
        break;
    case NftItemModel::BalanceRole:
        return QString::number(rec->balance);
        break;
    case NftItemModel::IdRole:
        return QString::fromStdString(rec->id.ToString());
        break;
    case NftItemModel::UrlRole:
        return rec->url;
        break;
    case NftItemModel::DescRole:
        return rec->desc;
        break;
    case NftItemModel::NftIdRole:
        return QString::fromStdString(rec->NFTId.ToString());
        break;
    case NftItemModel::ThumbnailRole:
        return rec->thumbnail;
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

    // Update nft transactions
    if(fLogEvents)
    {
        QMetaObject::invokeMethod(worker, "updateNftTx", Qt::QueuedConnection);
        QMetaObject::invokeMethod(worker, "updateThumbnail", Qt::QueuedConnection);
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
