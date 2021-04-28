#ifndef QTUMHWITOOL_H
#define QTUMHWITOOL_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QVariantMap>
class QtumHwiToolPriv;
class InstallDevicePriv;
class WalletModel;
class ExecRPCCommand;

/**
 * @brief The HWDevice class Hardware wallet device
 */
class HWDevice
{
public:
    /**
     * @brief HWDevice Constructor
     */
    HWDevice();

    /**
     * @brief toString Convert the result into string
     * @return String representation of the device data
     */
    QString toString() const;

    /**
     * @brief isValid Is device valid
     * @return true: valid; false: not valid
     */
    bool isValid() const;

    /**
     * @brief error Get error message for device
     * @return Error message
     */
    QString errorMessage() const;

    /// Device data
    QString fingerprint;
    QString serial_number;
    QString type;
    QString path;
    QString error;
    QString model;
    QString code;
};

/**
 * @brief The InstallDevice class Install Qtum app to device
 */
class InstallDevice
{
public:
    /**
     * @brief The DeviceType enum Supported device type to install
     */
    enum DeviceType
    {
        WalletNanoS,
        StakeNanoS
    };

    /**
     * @brief InstallDevice Constructor
     */
    InstallDevice(InstallDevice::DeviceType type = InstallDevice::WalletNanoS);

    /**
     * @brief ~InstallDevice Destructor
     */
    ~InstallDevice();

    /**
     * @brief deviceToString Device type to string
     * @param type Device type
     * @return String result
     */
    static QString deviceToString(InstallDevice::DeviceType type);

    /**
     * @brief loadCommand Get the load command
     * @param program Program to start
     * @param arguments Program arguments
     * @return Success of the operation
     */
    bool loadCommand(QString &program, QStringList &arguments);

    /**
     * @brief deleteCommand Get the delete command
     * @param program Program to start
     * @param arguments Program arguments
     * @return Success of the operation
     */
    bool deleteCommand(QString &program, QStringList &arguments);

private:
    bool getRCCommand(const QString &rcPath, QString &program, QStringList &arguments);
    QString parse(QString arg);

private:
    InstallDevicePriv *d;
};

/**
 * @brief The QtumHwiTool class Communicate with the Qtum Hardware Wallet Interface Tool
 */
class QtumHwiTool : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief QtumHwiTool Constructor
     * @param parent Parent object
     */
    explicit QtumHwiTool(QObject *parent = nullptr);

    /**
     * @brief ~QtumHwiTool Destructor
     */
    ~QtumHwiTool();

    /**
     * @brief enumerate Enumerate hardware wallet devices
     * @param devices List of devices
     * @return success of the operation
     */
    bool enumerate(QList<HWDevice>& devices);

    /**
     * @brief isConnected Check if a device is connected
     * @param fingerprint Hardware wallet device fingerprint
     * @return success of the operation
     */
    bool isConnected(const QString& fingerprint);

    /**
     * @brief getKeyPool Get the key pool for a device
     * @param fingerprint Hardware wallet device fingerprint
     * @param type Type of output
     * @param path The derivation path, if empty it is used the default
     * @param internal Needed when the derivation path is specified, to determine if the address pool is for change addresses.
     * If path is empty both internal and external addresses are loaded into the pool, so the parameter is not used.
     * @param desc Address descriptors
     * @return success of the operation
     */
    bool getKeyPool(const QString& fingerprint, int type, const QString& path, bool internal, QString& desc);

    /**
     * @brief getKeyPool Get the key pool for a device
     * @param fingerprint Hardware wallet device fingerprint
     * @param type Type of output
     * @param path The derivation path, if empty it is used the default
     * @param descs Address descriptors list
     * @return success of the operation
     */
    bool getKeyPool(const QString& fingerprint, int type, const QString& path, QStringList& descs);

    /**
     * @brief getKeyPoolPkh Get the PKH key pool for a device
     * @param fingerprint Hardware wallet device fingerprint
     * @param path The derivation path, if empty it is used the default
     * @param descs Address descriptors list
     * @return success of the operation
     */
    bool getKeyPoolPKH(const QString& fingerprint, const QString& path, QStringList& descs);

    /**
     * @brief getKeyPoolP2SH Get the P2SH key pool for a device
     * @param fingerprint Hardware wallet device fingerprint
     * @param path The derivation path, if empty it is used the default
     * @param descs Address descriptors list
     * @return success of the operation
     */
    bool getKeyPoolP2SH(const QString& fingerprint, const QString& path, QStringList& descs);

    /**
     * @brief getKeyPoolBech32 Get the Bech32 key pool for a device
     * @param fingerprint Hardware wallet device fingerprint
     * @param path The derivation path, if empty it is used the default
     * @param descs Address descriptors list
     * @return success of the operation
     */
    bool getKeyPoolBech32(const QString& fingerprint, const QString& path, QStringList& descs);

    /**
     * @brief signTx Sign PSBT transaction
     * @param fingerprint Hardware wallet device fingerprint
     * @param psbt In/Out PSBT transaction
     * @return success of the operation
     */
    bool signTx(const QString& fingerprint, QString& psbt);

    /**
     * @brief signMessage Sign message
     * @param fingerprint Hardware wallet device fingerprint
     * @param message Message to sign
     * @param path HD key path
     * @param signature Signature of the message
     * @return success of the operation
     */
    bool signMessage(const QString& fingerprint, const QString& message, const QString& path, QString& signature);

    /**
     * @brief signDelegate Sign delegate for psbt transaction
     * @param fingerprint Hardware wallet device fingerprint
     * @param psbt In/Out PSBT transaction
     * @return success of the operation
     */
    bool signDelegate(const QString& fingerprint, QString& psbt);

    /**
     * @brief rescanBlockchain Rescan blockchain
     * @param startHeight Start height
     * @param stopHeight Stop height
     * @return success of the operation
     */
    bool rescanBlockchain(int startHeight =0, int stopHeight =-1);

    /**
     * @brief importAddresses Import address descriptions
     * @param desc Address descriptions
     * @return success of the operation
     */
    bool importAddresses(const QString& desc);

    /**
     * @brief importMulti Import list of address descriptions
     * @param desc Address descriptions
     * @return success of the operation
     */
    bool importMulti(const QStringList& descs);

    /**
     * @brief finalizePsbt Finalize psbt
     * @param psbt Psbt transaction
     * @param hexTx Hex transaction
     * @param complete Is the set of signatures complete
     * @return success of the operation
     */
    bool finalizePsbt(const QString& psbt, QString& hexTx, bool & complete);

    /**
     * @brief sendRawTransaction Send raw transaction
     * @param hexTx Hex transaction
     * @param variantMap Result of the send operation
     * @return success of the operation
     */
    bool sendRawTransaction(const QString& hexTx, QVariantMap& variantMap);

    /**
     * @brief decodePsbt Decode psbt transaction
     * @param psbt Psbt transaction
     * @param decoded Decoded transaction
     * @return success of the operation
     */
    bool decodePsbt(const QString& psbt, QString& decoded);

    /**
     * @brief installApp Install Qtum App to ledger
     * @param type Ledger device type
     * @return success of the operation
     */
    bool installApp(InstallDevice::DeviceType type);

    /**
     * @brief removeApp Remove Qtum App to ledger
     * @param type Ledger device type
     * @return success of the operation
     */
    bool removeApp(InstallDevice::DeviceType type);

    /**
     * @brief errorMessage Get the last error message
     * @return Last error message
     */
    QString errorMessage();

    /**
     * @brief setModel Set wallet model
     * @param model Wallet model
     */
    void setModel(WalletModel *model);

Q_SIGNALS:

public Q_SLOTS:

private:
    bool isStarted();
    void wait();

    bool beginGetKeyPool(const QString& fingerprint, int type, QString& desc);
    bool endGetKeyPool(const QString& fingerprint, int type, QString& desc);
    bool execRPC(ExecRPCCommand* cmd, const QMap<QString, QString>& lstParams, QVariant& result, QString& resultJson);
    void addError(const QString& error);

    QtumHwiToolPriv* d;
};

#endif // QTUMHWITOOL_H
