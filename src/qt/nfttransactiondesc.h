#ifndef QTUM_QT_NFTTRANSACTIONDESC_H
#define QTUM_QT_NFTTRANSACTIONDESC_H

#include <QObject>
#include <QString>

class NftTransactionRecord;

namespace interfaces {
class Wallet;
struct NftTx;
}

/** Provide a human-readable extended HTML description of a token transaction.
 */
class NftTransactionDesc: public QObject
{
    Q_OBJECT

public:
    static QString toHTML(interfaces::Wallet& wallet, interfaces::NftTx& wtx, NftTransactionRecord *rec);

private:
    NftTransactionDesc() {}

    static QString FormatTxStatus(interfaces::Wallet& wallet, const interfaces::NftTx& wtx);
};

#endif // QTUM_QT_NFTTRANSACTIONDESC_H
