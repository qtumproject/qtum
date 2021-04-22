#ifndef QTUMLEDGER_H
#define QTUMLEDGER_H

#include <string>
#include <vector>
#include <sync.h>

extern RecursiveMutex cs_ledger;

class QtumLedgerPriv;

struct LedgerDevice
{
    /// Device data
    std::string fingerprint;
    std::string serial_number;
    std::string type;
    std::string path;
    std::string error;
    std::string model;
    std::string code;
};

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
     * @param path HD key path
     * @param vchSig Signature
     * @return true/false
     */
    bool signBlockHeader(const std::string& fingerprint, const std::string& header, const std::string& path, std::vector<unsigned char>& vchSig);

    /**
     * @brief isConnected Check if a device is connected
     * @param fingerprint Hardware wallet device fingerprint
     * @return success of the operation
     */
    bool isConnected(const std::string& fingerprint);

    /**
     * @brief enumerate Enumerate hardware wallet devices
     * @param devices List of devices
     * @return success of the operation
     */
    bool enumerate(std::vector<LedgerDevice>& devices);

    /**
     * @brief signTx Sign PSBT transaction
     * @param fingerprint Hardware wallet device fingerprint
     * @param psbt In/Out PSBT transaction
     * @return success of the operation
     */
    bool signTx(const std::string& fingerprint, std::string& psbt);

    /**
     * @brief signMessage Sign message
     * @param fingerprint Hardware wallet device fingerprint
     * @param message Message to sign
     * @param path HD key path
     * @param signature Signature of the message
     * @return success of the operation
     */
    bool signMessage(const std::string& fingerprint, const std::string& message, const std::string& path, std::string& signature);

    /**
     * @brief getKeyPool Get the key pool for a device
     * @param fingerprint Hardware wallet device fingerprint
     * @param type Type of output
     * @param desc Address descriptors
     * @return success of the operation
     */
    bool getKeyPool(const std::string& fingerprint, int type, std::string& desc);

    /**
     * @brief errorMessage Get the last error message
     * @return Last error message
     */
    std::string errorMessage();

    static QtumLedger &instance();

private:
    bool toolExists();
    bool isStarted();
    void wait();

    bool beginSignTx(const std::string& fingerprint, std::string& psbt);
    bool endSignTx(const std::string& fingerprint, std::string& psbt);

    bool beginSignBlockHeader(const std::string& fingerprint, const std::string& header, const std::string& path, std::vector<unsigned char>& vchSig);
    bool endSignBlockHeader(const std::string& fingerprint, const std::string& header, const std::string& path, std::vector<unsigned char>& vchSig);

    bool beginEnumerate(std::vector<LedgerDevice>& devices);
    bool endEnumerate(std::vector<LedgerDevice>& devices);

    bool beginSignMessage(const std::string& fingerprint, const std::string& message, const std::string& path, std::string &signature);
    bool endSignMessage(const std::string& fingerprint, const std::string& message, const std::string& path, std::string &signature);

    bool beginGetKeyPool(const std::string& fingerprint, int type, std::string& desc);
    bool endGetKeyPool(const std::string& fingerprint, int type, std::string& desc);

private:
    QtumLedger(const QtumLedger&);
    QtumLedger& operator=(const QtumLedger&);
    QtumLedgerPriv* d;
};
#endif
