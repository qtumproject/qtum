#include <qt/tokentransactiontablemodel.h>

#include <qt/addresstablemodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/styleSheet.h>
#include <qt/platformstyle.h>
#include <qt/tokentransactiondesc.h>
#include <qt/tokentransactionrecord.h>
#include <qt/walletmodel.h>

#include <core_io.h>
#include <validation.h>
#include <sync.h>
#include <uint256.h>
#include <util/system.h>
#include <interfaces/wallet.h>
#include <interfaces/handler.h>
#include <interfaces/node.h>

#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QIcon>
#include <QList>

// Amount column is right-aligned it contains numbers
static int column_alignments[] = {
        Qt::AlignLeft|Qt::AlignVCenter, /* status */
        Qt::AlignLeft|Qt::AlignVCenter, /* date */
        Qt::AlignLeft|Qt::AlignVCenter, /* type */
        Qt::AlignLeft|Qt::AlignVCenter, /* address */
        Qt::AlignLeft|Qt::AlignVCenter, /* name */
        Qt::AlignRight|Qt::AlignVCenter /* amount */
    };

// Comparison operator for sort/binary search of model tx list
struct TokenTxLessThan
{
    bool operator()(const TokenTransactionRecord &a, const TokenTransactionRecord &b) const
    {
        return a.hash < b.hash;
    }
    bool operator()(const TokenTransactionRecord &a, const uint256 &b) const
    {
        return a.hash < b;
    }
    bool operator()(const uint256 &a, const TokenTransactionRecord &b) const
    {
        return a < b.hash;
    }
};

// Private implementation
class TokenTransactionTablePriv
{
public:
    TokenTransactionTablePriv(TokenTransactionTableModel *_parent) :
        parent(_parent)
    {
    }

    TokenTransactionTableModel *parent;

    /* Local cache of wallet.
     * As it is in the same order as the CWallet, by definition
     * this is sorted by sha256.
     */
    QList<TokenTransactionRecord> cachedWallet;

    /* Query entire wallet anew from core.
     */
    void refreshWallet(interfaces::Node& node, interfaces::Wallet& wallet)
    {
        qDebug() << "TokenTransactionTablePriv::refreshWallet";
        cachedWallet.clear();
        {
            for(interfaces::TokenTx wtokenTx : wallet.getTokenTxs())
            {
                // Update token transaction time if the block time is changed
                int64_t time = node.getBlockTime(wtokenTx.block_number);
                if(time && time != wtokenTx.time)
                {
                    wtokenTx.time = time;
                    wallet.addTokenTxEntry(wtokenTx, false);
                }

                // Add token tx to the cache
                cachedWallet.append(TokenTransactionRecord::decomposeTransaction(wallet, wtokenTx));
            }
        }
    }

    /* Update our model of the wallet incrementally, to synchronize our model of the wallet
       with that of the core.

       Call with transaction that was added, removed or changed.
     */
    void updateWallet(interfaces::Wallet& wallet, const uint256 &hash, int status, bool showTransaction)
    {
        qDebug() << "TokenTransactionTablePriv::updateWallet: " + QString::fromStdString(hash.ToString()) + " " + QString::number(status);

        // Find bounds of this transaction in model
        QList<TokenTransactionRecord>::iterator lower = qLowerBound(
            cachedWallet.begin(), cachedWallet.end(), hash, TokenTxLessThan());
        QList<TokenTransactionRecord>::iterator upper = qUpperBound(
            cachedWallet.begin(), cachedWallet.end(), hash, TokenTxLessThan());
        int lowerIndex = (lower - cachedWallet.begin());
        int upperIndex = (upper - cachedWallet.begin());
        bool inModel = (lower != upper);

        // Find transaction in wallet
        interfaces::TokenTx wtokenTx = wallet.getTokenTx(hash);
        if(wtokenTx.hash == hash)
        {
            showTransaction &= wallet.isTokenTxMine(wtokenTx);
        }
        else
        {
            showTransaction = false;
        }

        if(status == CT_UPDATED)
        {
            if(showTransaction && !inModel)
                status = CT_NEW; /* Not in model, but want to show, treat as new */
            if(!showTransaction && inModel)
                status = CT_DELETED; /* In model, but want to hide, treat as deleted */
        }

        qDebug() << "    inModel=" + QString::number(inModel) +
                    " Index=" + QString::number(lowerIndex) + "-" + QString::number(upperIndex) +
                    " showTransaction=" + QString::number(showTransaction) + " derivedStatus=" + QString::number(status);

        switch(status)
        {
        case CT_NEW:
            if(inModel)
            {
                qWarning() << "TokenTransactionTablePriv::updateWallet: Warning: Got CT_NEW, but transaction is already in model";
                break;
            }
            if(showTransaction)
            {
                if(wtokenTx.hash != hash)
                {
                    qWarning() << "TokenTransactionTablePriv::updateWallet: Warning: Got CT_NEW, but transaction is not in wallet";
                    break;
                }
                // Added -- insert at the right position
                QList<TokenTransactionRecord> toInsert =
                        TokenTransactionRecord::decomposeTransaction(wallet, wtokenTx);
                if(!toInsert.isEmpty()) /* only if something to insert */
                {
                    parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex+toInsert.size()-1);
                    int insert_idx = lowerIndex;
                    Q_FOREACH(const TokenTransactionRecord &rec, toInsert)
                    {
                        cachedWallet.insert(insert_idx, rec);
                        insert_idx += 1;
                    }
                    parent->endInsertRows();
                }
            }
            break;
        case CT_DELETED:
            if(!inModel)
            {
                qWarning() << "TokenTransactionTablePriv::updateWallet: Warning: Got CT_DELETED, but transaction is not in model";
                break;
            }
            // Removed -- remove entire transaction from table
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedWallet.erase(lower, upper);
            parent->endRemoveRows();
            break;
        case CT_UPDATED:
            if(!inModel)
            {
                qWarning() << "TokenTransactionTablePriv::updateWallet: Warning: Got CT_UPDATED, but entry is not in model";
                break;
            }
            if(showTransaction)
            {
                if(wtokenTx.hash != hash)
                {
                    qWarning() << "TokenTransactionTablePriv::updateWallet: Warning: Got CT_UPDATED, but transaction is not in wallet";
                    break;
                }
                // Updated -- update at the right position
                QList<TokenTransactionRecord> toUpdate =
                        TokenTransactionRecord::decomposeTransaction(wallet, wtokenTx);
                if(!toUpdate.isEmpty()) /* only if something to insert */
                {
                    int update_idx = lowerIndex;
                    Q_FOREACH(const TokenTransactionRecord &rec, toUpdate)
                    {
                        cachedWallet[update_idx] = rec;
                        parent->emitDataChanged(update_idx);
                        update_idx += 1;
                    }
                }
            }
            break;
        }
    }

    int size()
    {
        return cachedWallet.size();
    }

    TokenTransactionRecord *index(interfaces::Wallet& wallet, int idx)
    {
        if(idx >= 0 && idx < cachedWallet.size())
        {
            TokenTransactionRecord *rec = &cachedWallet[idx];

            // Get required locks upfront. This avoids the GUI from getting
            // stuck if the core is holding the locks for a longer time - for
            // example, during a wallet rescan.
            //
            // If a status update is needed (blocks came in since last check),
            //  update the status of this transaction from the wallet. Otherwise,
            // simply re-use the cached status.
            int blockNumber = -1;
            bool inMempool = false;
            int numBlocks = -1;
            if (wallet.tryGetTokenTxStatus(rec->hash, blockNumber, inMempool, numBlocks) && rec->statusUpdateNeeded(numBlocks)) {
                rec->updateStatus(blockNumber, numBlocks);
            }
            return rec;
        }
        return 0;
    }

    QString describe(interfaces::Wallet& wallet, TokenTransactionRecord *rec)
    {
        interfaces::TokenTx wtokenTx = wallet.getTokenTx(rec->hash);
        if(wtokenTx.hash == rec->hash)
        {
            return TokenTransactionDesc::toHTML(wallet, wtokenTx, rec);
        }
        return QString();
    }

    QString getTxHex(interfaces::Wallet& wallet, TokenTransactionRecord *rec)
    {
        auto tx = wallet.getTx(rec->hash);
        if (tx) {
            std::string strHex = EncodeHexTx(*tx);
            return QString::fromStdString(strHex);
        }
        return QString();
    }
};

TokenTransactionTableModel::TokenTransactionTableModel(const PlatformStyle *_platformStyle, WalletModel *parent):
        QAbstractTableModel(parent),
        walletModel(parent),
        priv(new TokenTransactionTablePriv(this)),
        fProcessingQueuedTransactions(false),
        platformStyle(_platformStyle)
{
    color_unconfirmed = GetColorStyleValue("guiconstants/color-unconfirmed", COLOR_UNCONFIRMED);
    color_negative = GetColorStyleValue("guiconstants/color-negative", COLOR_NEGATIVE);
    color_bareaddress = GetColorStyleValue("guiconstants/color-bareaddress", COLOR_BAREADDRESS);
    color_black = GetColorStyleValue("guiconstants/color-black", COLOR_BLACK);

    columns << QString() << tr("Date") << tr("Type") << tr("Label") << tr("Name") << tr("Amount");
    priv->refreshWallet(walletModel->node(), walletModel->wallet());

    subscribeToCoreSignals();
}

TokenTransactionTableModel::~TokenTransactionTableModel()
{
    unsubscribeFromCoreSignals();
    delete priv;
}

void TokenTransactionTableModel::updateTransaction(const QString &hash, int status, bool showTransaction)
{
    uint256 updated;
    updated.SetHex(hash.toStdString());

    priv->updateWallet(walletModel->wallet(), updated, status, showTransaction);
}

void TokenTransactionTableModel::updateConfirmations()
{
    // Blocks came in since last poll.
    // Invalidate status (number of confirmations) and (possibly) description
    //  for all rows. Qt is smart enough to only actually request the data for the
    //  visible rows.
    Q_EMIT dataChanged(index(0, Status), index(priv->size()-1, Status));
    Q_EMIT dataChanged(index(0, ToAddress), index(priv->size()-1, ToAddress));
}

int TokenTransactionTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int TokenTransactionTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QString TokenTransactionTableModel::formatTxStatus(const TokenTransactionRecord *wtx) const
{
    QString status;

    switch(wtx->status.status)
    {
    case TokenTransactionStatus::Unconfirmed:
        status = tr("Unconfirmed");
        break;
    case TokenTransactionStatus::Confirming:
        status = tr("Confirming (%1 of %2 recommended confirmations)").arg(wtx->status.depth).arg(TokenTransactionRecord::RecommendedNumConfirmations);
        break;
    case TokenTransactionStatus::Confirmed:
        status = tr("Confirmed (%1 confirmations)").arg(wtx->status.depth);
        break;
    }

    return status;
}

QString TokenTransactionTableModel::formatTxDate(const TokenTransactionRecord *wtx) const
{
    if(wtx->time)
    {
        return GUIUtil::dateTimeStr(wtx->time);
    }
    return QString();
}

/* Look up address in address book, if found return label (address)
   otherwise just return (address)
 */
QString TokenTransactionTableModel::lookupAddress(const std::string &address, const std::string &_label, bool tooltip) const
{
    QString label = QString::fromStdString(_label);
    if(label.isEmpty())
    {
        label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(address));
    }
    QString description;
    if(!label.isEmpty())
    {
        description += label;
    }
    if(label.isEmpty() || tooltip)
    {
        description += QString(" (") + QString::fromStdString(address) + QString(")");
    }
    return description;
}

QString TokenTransactionTableModel::formatTxType(const TokenTransactionRecord *wtx) const
{
    switch(wtx->type)
    {
    case TokenTransactionRecord::RecvWithAddress:
        return tr("Received with");
    case TokenTransactionRecord::RecvFromOther:
        return tr("Received from");
    case TokenTransactionRecord::SendToAddress:
    case TokenTransactionRecord::SendToOther:
        return tr("Sent to");
    case TokenTransactionRecord::SendToSelf:
        return tr("Payment to yourself");
    default:
        return QString();
    }
}

QVariant TokenTransactionTableModel::txAddressDecoration(const TokenTransactionRecord *wtx) const
{
    switch(wtx->type)
    {
    case TokenTransactionRecord::RecvWithAddress:
    case TokenTransactionRecord::RecvFromOther:
        return platformStyle->TableColorIcon(":/icons/receive_from", PlatformStyle::Input);
    case TokenTransactionRecord::SendToAddress:
    case TokenTransactionRecord::SendToOther:
        return platformStyle->TableColorIcon(":/icons/send_to", PlatformStyle::Output);
    default:
        return platformStyle->TableColorIcon(":/icons/tx_inout", PlatformStyle::Inout);
    }
}

QString TokenTransactionTableModel::formatTxToAddress(const TokenTransactionRecord *wtx, bool tooltip) const
{
    switch(wtx->type)
    {
    case TokenTransactionRecord::RecvFromOther:
        return QString::fromStdString(wtx->address);
    case TokenTransactionRecord::RecvWithAddress:
    case TokenTransactionRecord::SendToAddress:
        return lookupAddress(wtx->address, wtx->label, tooltip);
    case TokenTransactionRecord::SendToOther:
        return QString::fromStdString(wtx->address);
    case TokenTransactionRecord::SendToSelf:
    default:
        return tr("(n/a)");
    }
}

QString TokenTransactionTableModel::formatTxTokenSymbol(const TokenTransactionRecord *wtx) const
{
    return QString::fromStdString(wtx->tokenSymbol);
}

QVariant TokenTransactionTableModel::addressColor(const TokenTransactionRecord *wtx) const
{
    // Show addresses without label in a less visible color
    switch(wtx->type)
    {
    case TokenTransactionRecord::RecvWithAddress:
    case TokenTransactionRecord::SendToAddress:
        {
        QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->address));
        if(label.isEmpty())
            return color_bareaddress;
        } break;
    case TokenTransactionRecord::SendToSelf:
        return color_bareaddress;
    default:
        break;
    }
    return QVariant();
}

QString TokenTransactionTableModel::formatTxAmount(const TokenTransactionRecord *wtx, bool showUnconfirmed, BitcoinUnits::SeparatorStyle separators) const
{
    QString str = BitcoinUnits::formatToken(wtx->decimals, wtx->credit + wtx->debit, false, separators);
    if(showUnconfirmed)
    {
        if(!wtx->status.countsForBalance)
        {
            str = QString("[") + str + QString("]");
        }
    }
    return QString(str);
}

QString TokenTransactionTableModel::formatTxAmountWithUnit(const TokenTransactionRecord *wtx, bool showUnconfirmed, BitcoinUnits::SeparatorStyle separators) const
{
    QString unit = QString::fromStdString(wtx->tokenSymbol);
    QString str = BitcoinUnits::formatTokenWithUnit(unit, wtx->decimals, wtx->credit + wtx->debit, false, separators);
    return QString(str);
}

QVariant TokenTransactionTableModel::txStatusDecoration(const TokenTransactionRecord *wtx) const
{
    switch(wtx->status.status)
    {
    case TokenTransactionStatus::Unconfirmed:
        return platformStyle->TableColorIcon(":/icons/transaction_0", PlatformStyle::Normal);
    case TokenTransactionStatus::Confirming: {
        int iconNum = ((wtx->status.depth - 1) * CONFIRM_ICONS) / TokenTransactionRecord::RecommendedNumConfirmations + 1;
        if(iconNum > CONFIRM_ICONS) iconNum = CONFIRM_ICONS;
        return platformStyle->TableColorIcon(QString(":/icons/transaction_%1").arg(iconNum), PlatformStyle::Normal);
        };
    case TokenTransactionStatus::Confirmed:
        return platformStyle->TableColorIcon(":/icons/transaction_confirmed", PlatformStyle::Normal);
    default:
        return color_black;
    }
}

QString TokenTransactionTableModel::formatTooltip(const TokenTransactionRecord *rec) const
{
    QString tooltip = formatTxStatus(rec) + QString("\n") + formatTxType(rec);
    if(rec->type==TokenTransactionRecord::RecvFromOther || rec->type==TokenTransactionRecord::SendToOther ||
       rec->type==TokenTransactionRecord::SendToAddress || rec->type==TokenTransactionRecord::RecvWithAddress)
    {
        tooltip += QString(" ") + formatTxToAddress(rec, true);
    }
    return tooltip;
}

QVariant TokenTransactionTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    TokenTransactionRecord *rec = static_cast<TokenTransactionRecord*>(index.internalPointer());

    switch(role)
    {
    case RawDecorationRole:
        switch(index.column())
        {
        case Status:
            return txStatusDecoration(rec);
        case ToAddress:
            return txAddressDecoration(rec);
        }
        break;
    case Qt::DecorationRole:
    {
        return qvariant_cast<QIcon>(index.data(RawDecorationRole));
    }
    case Qt::DisplayRole:
        switch(index.column())
        {
        case Date:
            return formatTxDate(rec);
        case Type:
            return formatTxType(rec);
        case ToAddress:
            return formatTxToAddress(rec, false);
        case Name:
            return formatTxTokenSymbol(rec);
        case Amount:
            return formatTxAmount(rec, true, BitcoinUnits::separatorAlways);
        }
        break;
    case Qt::EditRole:
        // Edit role is used for sorting, so return the unformatted values
        switch(index.column())
        {
        case Status:
            return QString::fromStdString(rec->status.sortKey);
        case Date:
            return rec->time;
        case Type:
            return formatTxType(rec);
        case ToAddress:
            return formatTxToAddress(rec, true);
        case Name:
            return formatTxTokenSymbol(rec);
        case Amount:
            return QString::fromStdString((rec->credit + rec->debit).str());
        }
        break;
    case Qt::ToolTipRole:
        return formatTooltip(rec);
    case Qt::TextAlignmentRole:
        return column_alignments[index.column()];
    case Qt::ForegroundRole:
        // Non-confirmed as transactions are grey
        if(!rec->status.countsForBalance)
        {
            return color_unconfirmed;
        }
        if(index.column() == Amount && (rec->credit+rec->debit) < 0)
        {
            return color_negative;
        }
        if(index.column() == ToAddress)
        {
            return addressColor(rec);
        }
        break;
    case TypeRole:
        return rec->type;
    case DateRole:
        return QDateTime::fromTime_t(static_cast<uint>(rec->time));
    case LongDescriptionRole:
        return priv->describe(walletModel->wallet(), rec);
    case NameRole:
        return QString::fromStdString(rec->tokenSymbol);
    case AddressRole:
        return QString::fromStdString(rec->address);
    case LabelRole:
        return walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(rec->address));
    case AmountRole:
        return QString::fromStdString((rec->credit + rec->debit).str());
    case TxHashRole:
        return QString::fromStdString(rec->txid.ToString());
    case TxHexRole:
        return priv->getTxHex(walletModel->wallet(), rec);
    case TxPlainTextRole:
        {
            QString details;
            QDateTime date = QDateTime::fromTime_t(static_cast<uint>(rec->time));
            QString txLabel = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(rec->address));
            QString symbol = QString::fromStdString(rec->tokenSymbol);

            details.append(date.toString("M/d/yy HH:mm"));
            details.append(" ");
            details.append(formatTxStatus(rec));
            details.append(". ");
            if(!formatTxType(rec).isEmpty()) {
                details.append(formatTxType(rec));
                details.append(" ");
            }
            if(!rec->address.empty()) {
                if(txLabel.isEmpty())
                    details.append(tr("(no label)") + " ");
                else {
                    details.append("(");
                    details.append(txLabel);
                    details.append(") ");
                }
                details.append(QString::fromStdString(rec->address));
                details.append(" ");
            }
            details.append(formatTxAmount(rec, false, BitcoinUnits::separatorNever));
            details.append(" " + symbol);
            return details;
        }
    case ConfirmedRole:
        return rec->status.countsForBalance;
    case FormattedAmountRole:
        // Used for copy/export, so don't include separators
        return formatTxAmount(rec, false, BitcoinUnits::separatorNever);
    case FormattedAmountWithUnitRole:
        return formatTxAmountWithUnit(rec, false, BitcoinUnits::separatorAlways);
    case StatusRole:
        return rec->status.status;
    }
    return QVariant();
}

QVariant TokenTransactionTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole)
        {
            return columns[section];
        }
        else if (role == Qt::TextAlignmentRole)
        {
            return column_alignments[section];
        } else if (role == Qt::ToolTipRole)
        {
            switch(section)
            {
            case Status:
                return tr("Transaction status. Hover over this field to show number of confirmations.");
            case Date:
                return tr("Date and time that the transaction was received.");
            case Type:
                return tr("Type of transaction.");
            case ToAddress:
                return tr("User-defined intent/purpose of the transaction.");
            case Name:
                return tr("Token name.");
            case Amount:
                return tr("Amount removed from or added to balance.");
            }
        }
    }
    return QVariant();
}

QModelIndex TokenTransactionTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    TokenTransactionRecord *data = priv->index(walletModel->wallet(), row);
    if(data)
    {
        return createIndex(row, column, data);
    }
    return QModelIndex();
}

void TokenTransactionTableModel::emitDataChanged(int idx)
{
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}

// queue notifications to show a non freezing progress dialog e.g. for rescan
struct TokenTransactionNotification
{
public:
    TokenTransactionNotification() {}
    TokenTransactionNotification(uint256 _hash, ChangeType _status, bool _showTransaction):
        hash(_hash), status(_status), showTransaction(_showTransaction) {}

    void invoke(QObject *ttm)
    {
        QString strHash = QString::fromStdString(hash.GetHex());
        qDebug() << "NotifyTokenTransactionChanged: " + strHash + " status= " + QString::number(status);
        QMetaObject::invokeMethod(ttm, "updateTransaction", Qt::QueuedConnection,
                                  Q_ARG(QString, strHash),
                                  Q_ARG(int, status),
                                  Q_ARG(bool, showTransaction));
    }
private:
    uint256 hash;
    ChangeType status;
    bool showTransaction;
};

static bool fQueueNotifications = false;
static std::vector< TokenTransactionNotification > vQueueNotifications;

static void NotifyTokenTransactionChanged(TokenTransactionTableModel *ttm, const uint256 &hash, ChangeType status)
{
    TokenTransactionNotification notification(hash, status, true);

    if (fQueueNotifications)
    {
        vQueueNotifications.push_back(notification);
        return;
    }
    notification.invoke(ttm);
}

static void ShowProgress(TokenTransactionTableModel *ttm, const std::string &title, int nProgress)
{
    if (nProgress == 0)
        fQueueNotifications = true;

    if (nProgress == 100)
    {
        fQueueNotifications = false;
        if (vQueueNotifications.size() > 10) // prevent balloon spam, show maximum 10 balloons
            QMetaObject::invokeMethod(ttm, "setProcessingQueuedTransactions", Qt::QueuedConnection, Q_ARG(bool, true));
        for (unsigned int i = 0; i < vQueueNotifications.size(); ++i)
        {
            if (vQueueNotifications.size() - i <= 10)
                QMetaObject::invokeMethod(ttm, "setProcessingQueuedTransactions", Qt::QueuedConnection, Q_ARG(bool, false));

            vQueueNotifications[i].invoke(ttm);
        }
        std::vector<TokenTransactionNotification >().swap(vQueueNotifications); // clear
    }
}

void TokenTransactionTableModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    m_handler_token_transaction_changed = walletModel->wallet().handleTokenTransactionChanged(boost::bind(NotifyTokenTransactionChanged, this, boost::placeholders::_1, boost::placeholders::_2));
    m_handler_show_progress = walletModel->wallet().handleShowProgress(boost::bind(ShowProgress, this, boost::placeholders::_1, boost::placeholders::_2));
}

void TokenTransactionTableModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    m_handler_token_transaction_changed->disconnect();
    m_handler_show_progress->disconnect();
}
