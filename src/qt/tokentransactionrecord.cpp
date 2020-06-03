#include <qt/tokentransactionrecord.h>

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
QList<TokenTransactionRecord> TokenTransactionRecord::decomposeTransaction(interfaces::Wallet &wallet, const interfaces::TokenTx &wtx)
{
    // Initialize variables
    QList<TokenTransactionRecord> parts;
    uint256 credit;
    uint256 debit;
    std::string tokenSymbol;
    uint8_t decimals = 18;
    if(!wtx.value.IsNull() && wallet.getTokenTxDetails(wtx, credit, debit, tokenSymbol, decimals))
    {
        // Get token transaction data
        TokenTransactionRecord rec;
        rec.time = wtx.time;
        rec.credit = dev::u2s(uintTou256(credit));
        rec.debit = -dev::u2s(uintTou256(debit));
        rec.hash = wtx.hash;
        rec.txid = wtx.tx_hash;
        rec.tokenSymbol = tokenSymbol;
        rec.decimals = decimals;
        rec.label = wtx.label;
        dev::s256 net = rec.credit + rec.debit;

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
            rec.address = wtx.receiver_address;
        default:
            break;
        }

        // Append record
        if(rec.type != Other)
            parts.append(rec);
    }

    return parts;
}

void TokenTransactionRecord::updateStatus(int block_number, int num_blocks)
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
        status.status = TokenTransactionStatus::Unconfirmed;
    }
    else if (status.depth < RecommendedNumConfirmations)
    {
        status.status = TokenTransactionStatus::Confirming;
    }
    else
    {
        status.status = TokenTransactionStatus::Confirmed;
    }
}

bool TokenTransactionRecord::statusUpdateNeeded(int numBlocks)
{
    return status.cur_num_blocks != numBlocks;
}

QString TokenTransactionRecord::getTxID() const
{
    return QString::fromStdString(txid.ToString());
}
