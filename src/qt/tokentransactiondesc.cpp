#include <qt/tokentransactiondesc.h>
#include <qt/tokentransactionrecord.h>
#include <qt/bitcoinunits.h>
#include <qt/guiutil.h>
#include <uint256.h>
#include <validation.h>
#include <wallet/wallet.h>
#include <timedata.h>
#include <interfaces/wallet.h>

QString TokenTransactionDesc::FormatTxStatus(interfaces::Wallet &wallet, const interfaces::TokenTx &wtx)
{
    // Get token tx status
    int blockNumber = -1;
    bool inMempool = false;
    int numBlocks = -1;
    wallet.getTokenTxStatus(wtx.hash, blockNumber, inMempool, numBlocks);

    // Determine transaction status
    int nDepth= 0;
    if(wtx.block_number == -1)
    {
        nDepth = 0;
    }
    else
    {
        nDepth = numBlocks - wtx.block_number + 1;
    }

    if (nDepth < 0)
        return tr("conflicted with a transaction with %1 confirmations").arg(-nDepth);
    else if (nDepth == 0)
    {
        if(inMempool)
            return tr("0/unconfirmed, in memory pool");
        else
            return tr("0/unconfirmed, not in memory pool");
    }
    else if (nDepth < TokenTransactionRecord::RecommendedNumConfirmations)
        return tr("%1/unconfirmed").arg(nDepth);
    else
        return tr("%1 confirmations").arg(nDepth);
}

QString TokenTransactionDesc::toHTML(interfaces::Wallet &wallet, interfaces::TokenTx &wtx, TokenTransactionRecord *rec)
{
    QString strHTML;

    strHTML.reserve(4000);
    strHTML += "<html><font face='verdana, arial, helvetica, sans-serif'>";

    int64_t nTime = rec->time;
    dev::s256 nDebit = rec->debit;
    dev::s256 nCredit = rec->credit;
    dev::s256 nNet = nCredit + nDebit;
    QString unit = QString::fromStdString(rec->tokenSymbol);

    strHTML += "<b>" + tr("Status") + ":</b> " + FormatTxStatus(wallet, wtx);

    strHTML += "<br>";

    strHTML += "<b>" + tr("Date") + ":</b> " + (nTime ? GUIUtil::dateTimeStr(nTime) : "") + "<br>";

    strHTML += "<b>" + tr("Transaction ID") + ":</b> " + rec->getTxID() + "<br>";

    strHTML += "<b>" + tr("Token Address") + ":</b> " + QString::fromStdString(wtx.contract_address) + "<br>";

    strHTML += "<b>" + tr("From") + ":</b> " + QString::fromStdString(wtx.sender_address) + "<br>";

    strHTML += "<b>" + tr("To") + ":</b> " + QString::fromStdString(wtx.receiver_address) + "<br>";

    if(nCredit > 0)
    {
         strHTML += "<b>" + tr("Credit") + ":</b> " + BitcoinUnits::formatTokenWithUnit(unit, rec->decimals, nCredit, true) + "<br>";
    }
    if(nDebit < 0)
    {
         strHTML += "<b>" + tr("Debit") + ":</b> " + BitcoinUnits::formatTokenWithUnit(unit, rec->decimals, nDebit, true) + "<br>";
    }
    strHTML += "<b>" + tr("Net Amount") + ":</b> " + BitcoinUnits::formatTokenWithUnit(unit, rec->decimals, nNet, true) + "<br>";


    strHTML += "</font></html>";
    return strHTML;
}
