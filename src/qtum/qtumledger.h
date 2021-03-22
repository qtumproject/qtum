#ifndef QTUMLEDGER_H
#define QTUMLEDGER_H

#include <string>
#include <vector>

class QtumLedgerPriv;

/**
 * @brief The QtumLedger class Communicate with the qtum ledger
 */
class QtumLedger {
    
public:
    /**
     * @brief QtumLedger Constructor
     */
    QtumLedger();

    /**
     * @brief ~QtumLedger Destructor
     */
    virtual ~QtumLedger();

    /**
     * @brief signCoinStake Sign proof of stake transaction
     * @param fingerprint Fingerprint of the ledger
     * @param psbt Proof of stake transaction
     * @return true/false
     */
    bool signCoinStake(const std::string& fingerprint, std::string& psbt);

    /**
     * @brief signBlockHeader Sign block header
     * @param fingerprint Fingerprint of the ledger
     * @param header Block header for the new block
     * @param vchSig Signature
     * @return true/false
     */
    bool signBlockHeader(const std::string& fingerprint, const std::string& header, std::vector<unsigned char>& vchSig);

private:
    bool toolExists();
    bool isStarted();
    void wait();

    bool beginSignTx(const std::string& fingerprint, std::string& psbt);
    bool endSignTx(const std::string& fingerprint, std::string& psbt);

    bool beginSignBlockHeader(const std::string& fingerprint, const std::string& header, std::vector<unsigned char>& vchSig);
    bool endSignBlockHeader(const std::string& fingerprint, const std::string& header, std::vector<unsigned char>& vchSig);

private:
    QtumLedger(const QtumLedger&);
    QtumLedger& operator=(const QtumLedger&);
    QtumLedgerPriv* d;
};
#endif
