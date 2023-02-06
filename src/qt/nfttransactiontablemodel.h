#ifndef QTUM_QT_NFTTRANSACTIONTABLEMODEL_H
#define QTUM_QT_NFTTRANSACTIONTABLEMODEL_H

#include <qt/bitcoinunits.h>

#include <QAbstractTableModel>
#include <QStringList>
#include <QColor>

#include <memory>

namespace interfaces {
class Handler;
}

class PlatformStyle;
class NftTransactionRecord;
class NftTransactionTablePriv;
class WalletModel;

/** UI model for the transaction table of a wallet.
 */
class NftTransactionTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit NftTransactionTableModel(const PlatformStyle *platformStyle, WalletModel *parent = 0);
    ~NftTransactionTableModel();

    enum ColumnIndex {
        Status = 0,
        Date = 1,
        Type = 2,
        ToAddress = 3,
        Name = 4,
        Amount = 5
    };

    /** Roles to get specific information from a transaction row.
        These are independent of column.
    */
    enum RoleIndex {
        /** Type of transaction */
        TypeRole = Qt::UserRole,
        /** Date and time this transaction was created */
        DateRole,
        /** Long description (HTML format) */
        LongDescriptionRole,
        /** Name of the nft */
        NameRole,
        /** Address of transaction */
        AddressRole,
        /** Label of address related to transaction */
        LabelRole,
        /** Net amount of transaction */
        AmountRole,
        /** Transaction hash */
        TxHashRole,
        /** Transaction data, hex-encoded */
        TxHexRole,
        /** Whole transaction as plain text */
        TxPlainTextRole,
        /** Is transaction confirmed? */
        ConfirmedRole,
        /** Formatted amount, without brackets when unconfirmed */
        FormattedAmountRole,
        /** Formatted amount, with unit */
        FormattedAmountWithUnitRole,
        /** Transaction status (NftTransactionRecord::Status) */
        StatusRole,
        /** Unprocessed icon */
        RawDecorationRole,
    };

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    bool processingQueuedTransactions() { return fProcessingQueuedTransactions; }

private:
    WalletModel *walletModel;
    std::unique_ptr<interfaces::Handler> m_handler_nft_transaction_changed;
    std::unique_ptr<interfaces::Handler> m_handler_show_progress;
    QStringList columns;
    NftTransactionTablePriv *priv;
    bool fProcessingQueuedTransactions;
    const PlatformStyle *platformStyle;
    QColor color_unconfirmed;
    QColor color_negative;
    QColor color_bareaddress;
    QColor color_black;

    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();

    QString lookupAddress(const std::string &address, const std::string &label, bool tooltip) const;
    QVariant addressColor(const NftTransactionRecord *wtx) const;
    QString formatTxStatus(const NftTransactionRecord *wtx) const;
    QString formatTxDate(const NftTransactionRecord *wtx) const;
    QString formatTxType(const NftTransactionRecord *wtx) const;
    QString formatTxToAddress(const NftTransactionRecord *wtx, bool tooltip) const;
    QString formatTxName(const NftTransactionRecord *wtx) const;
    QString formatTxAmount(const NftTransactionRecord *wtx, bool showUnconfirmed=true, BitcoinUnits::SeparatorStyle separators=BitcoinUnits::SeparatorStyle::STANDARD) const;
    QString formatTxAmountWithUnit(const NftTransactionRecord *wtx, bool showUnconfirmed=true, BitcoinUnits::SeparatorStyle separators=BitcoinUnits::SeparatorStyle::STANDARD) const;
    QString formatTooltip(const NftTransactionRecord *rec) const;
    QVariant txStatusDecoration(const NftTransactionRecord *wtx) const;
    QVariant txAddressDecoration(const NftTransactionRecord *wtx) const;

public Q_SLOTS:
    /* Notify listeners that data changed. */
    void emitDataChanged(int index);
    /* New transaction, or transaction changed status */
    void updateTransaction(const QString &hash, int status, bool showTransaction);
    void updateConfirmations();
    /* Needed to update fProcessingQueuedTransactions through a QueuedConnection */
    void setProcessingQueuedTransactions(bool value) { fProcessingQueuedTransactions = value; }

    friend class NftTransactionTablePriv;
};

#endif // QTUM_QT_NFTTRANSACTIONTABLEMODEL_H
