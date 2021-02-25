#include <qtum/qtumledger.h>

class QtumLedgerPriv
{
public:
    QtumLedgerPriv()
    {}
};

QtumLedger::QtumLedger():
    priv(0)
{
    priv = new QtumLedgerPriv();
}

QtumLedger::~QtumLedger()
{
    if(priv)
        delete priv;
    priv = 0;
}

bool QtumLedger::signCoinStake(const std::string &fingerprint, std::string &psbt)
{
    return false;
}

bool QtumLedger::signBlockHeader(const std::string &fingerprint, const std::string &header, std::vector<unsigned char> &vchSig)
{
    return false;
}
