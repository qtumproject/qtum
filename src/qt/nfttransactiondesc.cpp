#include <qt/nfttransactiondesc.h>
#include <qt/nfttransactionrecord.h>
#include <qt/bitcoinunits.h>
#include <qt/guiutil.h>
#include <qt/styleSheet.h>
#include <uint256.h>
#include <validation.h>
#include <wallet/wallet.h>
#include <timedata.h>
#include <interfaces/wallet.h>
#include <consensus/params.h>
#include <qt/guiconstants.h>

class NftTransactionFormater
{
public:
    NftTransactionFormater()
    {
        itemNameColor = GetStringStyleValue("nfttransactiondesc/item-name-color", "#ffffff");
        itemColor = GetStringStyleValue("nfttransactiondesc/item-color", "#ffffff");
        itemFontBold = GetIntStyleValue("nfttransactiondesc/item-font-bold", true);
        network = Params().NetworkIDString();
    }

    static const NftTransactionFormater& instance()
    {
        static NftTransactionFormater data;
        return data;
    }

    static QString ItemColor()
    {
        return " color='" + instance().itemColor + "'";
    }

    static QString ItemNameColor(const QString& name, bool space = true)
    {
        QString ret;
        if(instance().itemFontBold)
        {
            ret += "<b>";
        }

        ret += "<font color='" + instance().itemNameColor + "'>" + name +":</font>";

        if(instance().itemFontBold)
        {
            ret += "</b>";
        }

        if(space) ret += " ";

        return ret;
    }

    static QString TxIdLink(const QString& txHash)
    {
        if(instance().network == "main")
        {
            return QString(QTUM_INFO_MAINNET).arg("tx", txHash);
        }
        else if(instance().network == "test")
        {
            return QString(QTUM_INFO_TESTNET).arg("tx", txHash);
        }

        return txHash;
    }

private:
    QString itemNameColor;
    QString itemColor;
    bool itemFontBold;
    std::string network;
};

QString NftTransactionDesc::FormatTxStatus(interfaces::Wallet &wallet, const interfaces::NftTx &wtx)
{
    // Get nft tx status
    int blockNumber = -1;
    bool inMempool = false;
    int numBlocks = -1;
    wallet.getNftTxStatus(wtx.hash, blockNumber, inMempool, numBlocks);

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
    else if (nDepth < NftTransactionRecord::RecommendedNumConfirmations)
        return tr("%1/unconfirmed").arg(nDepth);
    else
        return tr("%1 confirmations").arg(nDepth);
}

QString NftTransactionDesc::toHTML(interfaces::Wallet &wallet, interfaces::NftTx &wtx, NftTransactionRecord *rec)
{
    QString strHTML;

    strHTML.reserve(4000);
    strHTML += "<html><font face='verdana, arial, helvetica, sans-serif'"+NftTransactionFormater::ItemColor()+">";

    int64_t nTime = rec->time;
    int32_t nDebit = rec->debit;
    int32_t nCredit = rec->credit;
    int32_t nNet = nCredit + nDebit;

    strHTML += NftTransactionFormater::ItemNameColor(tr("Status")) + FormatTxStatus(wallet, wtx);

    strHTML += "<br>";

    strHTML += NftTransactionFormater::ItemNameColor(tr("Date")) + (nTime ? GUIUtil::dateTimeStr(nTime) : "") + "<br>";

    strHTML += NftTransactionFormater::ItemNameColor(tr("Transaction ID")) + NftTransactionFormater::TxIdLink(rec->getTxID()) + "<br>";

    strHTML += NftTransactionFormater::ItemNameColor(tr("From")) + QString::fromStdString(wtx.sender) + "<br>";

    strHTML += NftTransactionFormater::ItemNameColor(tr("To")) + QString::fromStdString(wtx.receiver) + "<br>";

    if(nCredit > 0)
    {
         strHTML += NftTransactionFormater::ItemNameColor(tr("Credit")) + BitcoinUnits::formatInt(nCredit, true) + "<br>";
    }
    if(nDebit < 0)
    {
         strHTML += NftTransactionFormater::ItemNameColor(tr("Debit")) + BitcoinUnits::formatInt(nDebit, true) + "<br>";
    }
    strHTML += NftTransactionFormater::ItemNameColor(tr("Net Amount")) + BitcoinUnits::formatInt(nNet, true) + "<br>";


    strHTML += "</font></html>";
    return strHTML;
}
