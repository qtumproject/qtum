#include "tokentransactiondesc.h"
#include "tokentransactionrecord.h"
#include "bitcoinunits.h"
#include "guiutil.h"
#include "uint256.h"
#include "validation.h"
#include "wallet/wallet.h"

QString TokenTransactionDesc::FormatTxStatus(const CTokenTx& wtx)
{
    AssertLockHeld(cs_main);
    return QString();
}

QString TokenTransactionDesc::toHTML(CWallet *wallet, CTokenTx &wtx, TokenTransactionRecord *rec)
{
    QString strHTML;

    LOCK2(cs_main, wallet->cs_wallet);
    strHTML.reserve(4000);
    strHTML += "<html><font face='verdana, arial, helvetica, sans-serif'>";

    int64_t nTime = rec->time;
    dev::s256 nDebit = rec->debit;
    dev::s256 nCredit = rec->credit;
    dev::s256 nNet = nCredit + nDebit;
    QString unit = QString::fromStdString(rec->tokenSymbol);

    strHTML += "<b>" + tr("Status") + ":</b> " + FormatTxStatus(wtx);

    strHTML += "<br>";

    strHTML += "<b>" + tr("Date") + ":</b> " + (nTime ? GUIUtil::dateTimeStr(nTime) : "") + "<br>";

    strHTML += "<b>" + tr("Transaction ID") + ":</b> " + rec->getTxID() + "<br>";

    strHTML += "<b>" + tr("Token Address") + ":</b> " + QString::fromStdString(wtx.strContractAddress) + "<br>";

    strHTML += "<b>" + tr("From") + ":</b> " + QString::fromStdString(wtx.strSenderAddress) + "<br>";

    strHTML += "<b>" + tr("To") + ":</b> " + QString::fromStdString(wtx.strReceiverAddress) + "<br>";

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
