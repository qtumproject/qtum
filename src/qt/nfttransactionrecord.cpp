#include <qt/nfttransactionrecord.h>

#include <base58.h>
#include <consensus/consensus.h>
#include <validation.h>
#include <timedata.h>
#include <wallet/wallet.h>
#include <interfaces/wallet.h>

#include <stdint.h>

/*
 * Decompose CWallet transaction to model transaction records.
 */
QList<NftTransactionRecord> NftTransactionRecord::decomposeTransaction(interfaces::Wallet &wallet, const interfaces::NftTx &wtx)
{
    // Initialize variables
    QList<NftTransactionRecord> parts;
    qint32 credit = 0;
    qint32 debit = 0;
    if(wtx.value && wallet.getNftTxDetails(wtx, credit, debit))
    {
        // Get nft transaction data
        NftTransactionRecord rec;
        rec.time = wtx.create_time;
        rec.credit = credit;
        rec.debit = -debit;
        rec.hash = wtx.hash;
        rec.txid = wtx.tx_hash;
        qint32 net = rec.credit + rec.debit;

        // Determine type
        if(net == 0)
        {
            rec.type = SendToSelf;
        }
        else if(net > 0)
        {
           rec.type = RecvWithAddress;
        }
        else
        {
            rec.type = SendToAddress;
        }

        if(net)
        {
            rec.status.countsForBalance = true;
        }

        // Set address
        switch (rec.type) {
        case SendToSelf:
        case SendToAddress:
        case SendToOther:
        case RecvWithAddress:
        case RecvFromOther:
            rec.address = wtx.receiver;
        default:
            break;
        }

        // Append record
        if(rec.type != Other)
            parts.append(rec);
    }

    return parts;
}

void NftTransactionRecord::updateStatus(int block_number, int num_blocks)
{
    // Determine transaction status
    status.cur_num_blocks = num_blocks;
    if(block_number == -1)
    {
        status.depth = 0;
    }
    else
    {
        status.depth = status.cur_num_blocks - block_number + 1;
    }

    if (status.depth == 0)
    {
        status.status = NftTransactionStatus::Unconfirmed;
    }
    else if (status.depth < RecommendedNumConfirmations)
    {
        status.status = NftTransactionStatus::Confirming;
    }
    else
    {
        status.status = NftTransactionStatus::Confirmed;
    }
}

bool NftTransactionRecord::statusUpdateNeeded(int numBlocks)
{
    return status.cur_num_blocks != numBlocks;
}

QString NftTransactionRecord::getTxID() const
{
    return QString::fromStdString(txid.ToString());
}
