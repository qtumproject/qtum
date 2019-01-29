#include <qt/tokenitemmodel.h>
#include <qt/token.h>
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

class TokenItemEntry
{
public:
    TokenItemEntry()
    {}

    TokenItemEntry(const interfaces::TokenInfo &tokenInfo)
    {
        hash = tokenInfo.hash;
        createTime.setTime_t(tokenInfo.time);
        contractAddress = QString::fromStdString(tokenInfo.contract_address);
        tokenName = QString::fromStdString(tokenInfo.token_name);
        tokenSymbol = QString::fromStdString(tokenInfo.token_symbol);
        decimals = tokenInfo.decimals;
        senderAddress = QString::fromStdString(tokenInfo.sender_address);
    }

    TokenItemEntry( const TokenItemEntry &obj)
    {
        hash = obj.hash;
        createTime = obj.createTime;
        contractAddress = obj.contractAddress;
        tokenName = obj.tokenName;
        tokenSymbol = obj.tokenSymbol;
        decimals = obj.decimals;
        senderAddress = obj.senderAddress;
        balance = obj.balance;
    }

    ~TokenItemEntry()
    {}

    uint256 hash;
    QDateTime createTime;
    QString contractAddress;
    QString tokenName;
    QString tokenSymbol;
    quint8 decimals;
    QString senderAddress;
    int256_t balance;
};

class TokenTxWorker : public QObject
{
    Q_OBJECT
public:
    WalletModel *walletModel;
    bool first;
    Token tokenAbi;
    TokenTxWorker(WalletModel *_walletModel):
        walletModel(_walletModel), first(true) {}
    
private Q_SLOTS:
    void updateTokenTx(const QString &hash)
    {
        // Initialize variables
        uint256 tokenHash = uint256S(hash.toStdString());
        int64_t fromBlock = 0;
        int64_t toBlock = -1;
        interfaces::TokenInfo tokenInfo;
        uint256 blockHash;
        bool found = false;

        int64_t backInPast = first ? COINBASE_MATURITY : 10;
        first = false;

        // Get current height and block hash
        toBlock = walletModel->node().getNumBlocks();
        blockHash = walletModel->node().getBlockHash(toBlock);

        if(toBlock > -1)
        {
            // Find the token tx in the wallet
            tokenInfo = walletModel->wallet().getToken(tokenHash);
            found = tokenInfo.hash == tokenHash;
            if(found)
            {
                // Get the start location for search the event log
                if(tokenInfo.block_number < toBlock)
                {
                    if(walletModel->node().getBlockHash(tokenInfo.block_number) == tokenInfo.block_hash)
                    {
                        fromBlock = tokenInfo.block_number;
                    }
                    else
                    {
                        fromBlock = tokenInfo.block_number - backInPast;
                    }
                }
                else
                {
                    fromBlock = toBlock - backInPast;
                }
                if(fromBlock < 0)
                    fromBlock = 0;

                tokenInfo.block_hash = blockHash;
                tokenInfo.block_number = toBlock;
            }
        }

        if(found)
        {
            // List the events and update the token tx
            std::vector<TokenEvent> tokenEvents;
            tokenAbi.setAddress(tokenInfo.contract_address);
            tokenAbi.setSender(tokenInfo.sender_address);
            tokenAbi.transferEvents(tokenEvents, fromBlock, toBlock);
            for(size_t i = 0; i < tokenEvents.size(); i++)
            {
                TokenEvent event = tokenEvents[i];
                interfaces::TokenTx tokenTx;
                tokenTx.contract_address = event.address;
                tokenTx.sender_address = event.sender;
                tokenTx.receiver_address = event.receiver;
                tokenTx.value = event.value;
                tokenTx.tx_hash = event.transactionHash;
                tokenTx.block_hash = event.blockHash;
                tokenTx.block_number = event.blockNumber;
                walletModel->wallet().addTokenTxEntry(tokenTx, false);
            }

            walletModel->wallet().addTokenEntry(tokenInfo);
        }
    }

    void cleanTokenTxEntries()
    {
        if(walletModel) walletModel->wallet().cleanTokenTxEntries();
    }

    void updateBalance(QString hash, QString contractAddress, QString senderAddress)
    {
        tokenAbi.setAddress(contractAddress.toStdString());
        tokenAbi.setSender(senderAddress.toStdString());
        std::string strBalance;
        if(tokenAbi.balanceOf(strBalance))
        {
            QString balance = QString::fromStdString(strBalance);
            Q_EMIT balanceChanged(hash, balance);
        }
    }

Q_SIGNALS:
    // Signal that balance in token changed
    void balanceChanged(QString hash, QString balance);
};

#include "tokenitemmodel.moc"

struct TokenItemEntryLessThan
{
    bool operator()(const TokenItemEntry &a, const TokenItemEntry &b) const
    {
        return a.hash < b.hash;
    }
    bool operator()(const TokenItemEntry &a, const uint256 &b) const
    {
        return a.hash < b;
    }
    bool operator()(const uint256 &a, const TokenItemEntry &b) const
    {
        return a < b.hash;
    }
};

class TokenItemPriv
{
public:
    QList<TokenItemEntry> cachedTokenItem;
    TokenItemModel *parent;

    TokenItemPriv(TokenItemModel *_parent):
        parent(_parent) {}

    void refreshTokenItem(interfaces::Wallet& wallet)
    {
        cachedTokenItem.clear();
        {
            for(interfaces::TokenInfo token : wallet.getTokens())
            {
                TokenItemEntry tokenItem(token);
                if(parent)
                {
                    parent->updateBalance(tokenItem);
                }
                cachedTokenItem.append(tokenItem);
            }
        }
        std::sort(cachedTokenItem.begin(), cachedTokenItem.end(), TokenItemEntryLessThan());
    }

    void updateEntry(const TokenItemEntry &_item, int status)
    {
        // Find address / label in model
        TokenItemEntry item;
        QList<TokenItemEntry>::iterator lower = qLowerBound(
            cachedTokenItem.begin(), cachedTokenItem.end(), _item, TokenItemEntryLessThan());
        QList<TokenItemEntry>::iterator upper = qUpperBound(
            cachedTokenItem.begin(), cachedTokenItem.end(), _item, TokenItemEntryLessThan());
        int lowerIndex = (lower - cachedTokenItem.begin());
        int upperIndex = (upper - cachedTokenItem.begin());
        bool inModel = (lower != upper);
        item = _item;
        if(inModel)
        {
            item.balance = cachedTokenItem[lowerIndex].balance;
        }

        switch(status)
        {
        case CT_NEW:
            if(inModel)
            {
                qWarning() << "TokenItemPriv::updateEntry: Warning: Got CT_NEW, but entry is already in model";
                break;
            }
            parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
            cachedTokenItem.insert(lowerIndex, item);
            parent->endInsertRows();
            break;
        case CT_UPDATED:
            if(!inModel)
            {
                qWarning() << "TokenItemPriv::updateEntry: Warning: Got CT_UPDATED, but entry is not in model";
                break;
            }
            cachedTokenItem[lowerIndex] = item;
            parent->emitDataChanged(lowerIndex);
            break;
        case CT_DELETED:
            if(!inModel)
            {
                qWarning() << "TokenItemPriv::updateEntry: Warning: Got CT_DELETED, but entry is not in model";
                break;
            }
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedTokenItem.erase(lower, upper);
            parent->endRemoveRows();
            break;
        }
    }

    int updateBalance(QString hash, QString balance)
    {
        uint256 updated;
        updated.SetHex(hash.toStdString());
        int256_t val(balance.toStdString());

        for(int i = 0; i < cachedTokenItem.size(); i++)
        {
            TokenItemEntry item = cachedTokenItem[i];
            if(item.hash == updated && item.balance != val)
            {
                item.balance = val;
                cachedTokenItem[i] = item;
                return i;
            }
        }

        return -1;
    }

    int size()
    {
        return cachedTokenItem.size();
    }

    TokenItemEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedTokenItem.size())
        {
            return &cachedTokenItem[idx];
        }
        else
        {
            return 0;
        }
    }
};

TokenItemModel::TokenItemModel(WalletModel *parent):
    QAbstractItemModel(parent),
    walletModel(parent),
    priv(0),
    worker(0),
    tokenTxCleaned(false)
{
    columns << tr("Token Name") << tr("Token Symbol") << tr("Balance");

    priv = new TokenItemPriv(this);
    priv->refreshTokenItem(walletModel->wallet());

    worker = new TokenTxWorker(walletModel);
    worker->tokenAbi.setModel(walletModel);
    worker->moveToThread(&(t));
    connect(worker, SIGNAL(balanceChanged(QString,QString)), this, SLOT(balanceChanged(QString,QString)));

    t.start();

    subscribeToCoreSignals();
}

TokenItemModel::~TokenItemModel()
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

QModelIndex TokenItemModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    TokenItemEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    return QModelIndex();
}

QModelIndex TokenItemModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);
    return QModelIndex();
}

int TokenItemModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int TokenItemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant TokenItemModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    TokenItemEntry *rec = static_cast<TokenItemEntry*>(index.internalPointer());

    switch (role) {
    case Qt::DisplayRole:
        switch(index.column())
        {
        case Name:
            return rec->tokenName;
        case Symbol:
            return rec->tokenSymbol;
        case Balance:
            return BitcoinUnits::formatToken(rec->decimals, rec->balance, false, BitcoinUnits::separatorAlways);
        default:
            break;
        }
        break;
    case TokenItemModel::HashRole:
        return QString::fromStdString(rec->hash.ToString());
        break;
    case TokenItemModel::AddressRole:
        return rec->contractAddress;
        break;
    case TokenItemModel::NameRole:
        return rec->tokenName;
        break;
    case TokenItemModel::SymbolRole:
        return rec->tokenSymbol;
        break;
    case TokenItemModel::DecimalsRole:
        return rec->decimals;
        break;
    case TokenItemModel::SenderRole:
        return rec->senderAddress;
        break;
    case TokenItemModel::BalanceRole:
        return BitcoinUnits::formatToken(rec->decimals, rec->balance, false, BitcoinUnits::separatorAlways);
        break;
    case TokenItemModel::RawBalanceRole:
        return QString::fromStdString(rec->balance.str());
        break;
    default:
        break;
    }

    return QVariant();
}

void TokenItemModel::updateToken(const QString &hash, int status, bool showToken)
{
    // Find token in wallet
    uint256 updated;
    updated.SetHex(hash.toStdString());
    interfaces::TokenInfo token =walletModel->wallet().getToken(updated);
    showToken &= token.hash == updated;

    TokenItemEntry tokenEntry;
    if(showToken)
    {
        tokenEntry = TokenItemEntry(token);
        updateBalance(tokenEntry);
    }
    else
    {
        tokenEntry.hash = updated;
    }
    priv->updateEntry(tokenEntry, status);
}

void TokenItemModel::checkTokenBalanceChanged()
{
    if(!priv)
        return;

    // Update token balance
    for(int i = 0; i < priv->cachedTokenItem.size(); i++)
    {
        TokenItemEntry tokenEntry = priv->cachedTokenItem[i];
        updateBalance(tokenEntry);
    }

    // Update token transactions
    if(fLogEvents)
    {
        // Search for token transactions
        for(int i = 0; i < priv->cachedTokenItem.size(); i++)
        {
            TokenItemEntry tokenEntry = priv->cachedTokenItem[i];
            QString hash = QString::fromStdString(tokenEntry.hash.ToString());
            QMetaObject::invokeMethod(worker, "updateTokenTx", Qt::QueuedConnection,
                                      Q_ARG(QString, hash));
        }

        // Clean token transactions
        if(!tokenTxCleaned)
        {
            tokenTxCleaned = true;
            QMetaObject::invokeMethod(worker, "cleanTokenTxEntries", Qt::QueuedConnection);
        }
    }
}

void TokenItemModel::emitDataChanged(int idx)
{
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}

struct TokenNotification
{
public:
    TokenNotification() {}
    TokenNotification(uint256 _hash, ChangeType _status, bool _showToken):
        hash(_hash), status(_status), showToken(_showToken) {}

    void invoke(QObject *tim)
    {
        QString strHash = QString::fromStdString(hash.GetHex());
        qDebug() << "NotifyTokenChanged: " + strHash + " status= " + QString::number(status);

        QMetaObject::invokeMethod(tim, "updateToken", Qt::QueuedConnection,
                                  Q_ARG(QString, strHash),
                                  Q_ARG(int, status),
                                  Q_ARG(bool, showToken));
    }
private:
    uint256 hash;
    ChangeType status;
    bool showToken;
};

static void NotifyTokenChanged(TokenItemModel *tim, const uint256 &hash, ChangeType status)
{
    TokenNotification notification(hash, status, true);
    notification.invoke(tim);
}

void TokenItemModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    m_handler_token_changed = walletModel->wallet().handleTokenChanged(boost::bind(NotifyTokenChanged, this, _1, _2));
}

void TokenItemModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    m_handler_token_changed->disconnect();
}

void TokenItemModel::balanceChanged(QString hash, QString balance)
{
    int index = priv->updateBalance(hash, balance);
    if(index > -1)
    {
        emitDataChanged(index);
    }
}

void TokenItemModel::updateBalance(const TokenItemEntry &entry)
{
    QString hash = QString::fromStdString(entry.hash.ToString());
    QMetaObject::invokeMethod(worker, "updateBalance", Qt::QueuedConnection,
                              Q_ARG(QString, hash), Q_ARG(QString, entry.contractAddress), Q_ARG(QString, entry.senderAddress));
}
