#ifndef QTUM_QT_NFTTRANSACTIONRECORD_H
#define QTUM_QT_NFTTRANSACTIONRECORD_H

#include <amount.h>
#include <uint256.h>

#include <QList>
#include <QString>

namespace interfaces {
class Wallet;
struct NftTx;
}
/** UI model for nft transaction status. The nft transaction status is the part of a nft transaction that will change over time.
 */
class NftTransactionStatus
{
public:
    NftTransactionStatus():
        countsForBalance(false), sortKey(""),
        status(Unconfirmed), depth(0), cur_num_blocks(-1)
    { }

    enum Status {
        Confirmed,          /**< Have 6 or more confirmations (normal tx) or fully mature (mined tx) **/
        /// Normal (sent/received) nft transactions
        Unconfirmed,        /**< Not yet mined into a block **/
        Confirming         /**< Confirmed, but waiting for the recommended number of confirmations **/
    };

    /// Nft transaction counts towards available balance
    bool countsForBalance;
    /// Sorting key based on status
    std::string sortKey;

    /** @name Reported status
       @{*/
    Status status;
    qint64 depth;

    /**@}*/

    /** Current number of blocks (to know whether cached status is still valid) */
    int cur_num_blocks;
};

/** UI model for a nft transaction. A core nft transaction can be represented by multiple UI nft transactions if it has
    multiple outputs.
 */
class NftTransactionRecord
{
public:
    enum Type
    {
        Other,
        SendToAddress,
        SendToOther,
        RecvWithAddress,
        RecvFromOther,
        SendToSelf
    };

    /** Number of confirmation recommended for accepting a nft transaction */
    static const int RecommendedNumConfirmations = 10;

    NftTransactionRecord():
            hash(), txid(), time(0), type(Other), address(""), debit(0), credit(0), label("")
    {
    }

    /** Decompose Nft transaction into a record.
     */
    static QList<NftTransactionRecord> decomposeTransaction(interfaces::Wallet &wallet, const interfaces::NftTx &wtx);

    /** @name Immutable nft transaction attributes
      @{*/
    uint256 hash;
    uint256 txid;
    qint64 time;
    Type type;
    std::string address;
    qint32 debit;
    qint32 credit;
    std::string label;
    /**@}*/

    /** Return the unique identifier for this transaction (part) */
    QString getTxID() const;

    /** Status: can change with block chain update */
    NftTransactionStatus status;

    /** Update status from core wallet tx.
     */
    void updateStatus(int block_number, int num_blocks);

    /** Return whether a status update is needed.
     */
    bool statusUpdateNeeded(int numBlocks);
};

#endif // BITCOIN_QT_TRANSACTIONRECORD_H
