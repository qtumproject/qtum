#ifndef QTUMHWITOOL_H
#define QTUMHWITOOL_H

#include <QObject>
#include <QString>
#include <QList>
class QtumHwiToolPriv;

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

    /// Device data
    QString fingerprint;
    QString serial_number;
    QString type;
    QString path;
    QString error;
    QString model;
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
     * @param desc Address descriptors
     * @return success of the operation
     */
    bool getKeyPool(const QString& fingerprint, QString& desc);

    /**
     * @brief signTx Sign PSBT transaction
     * @param fingerprint Hardware wallet device fingerprint
     * @param psbt In/Out PSBT transaction
     * @return success of the operation
     */
    bool signTx(const QString& fingerprint, QString& psbt);

    /**
     * @brief errorMessage Get the last error message
     * @return Last error message
     */
    QString errorMessage();

Q_SIGNALS:

public Q_SLOTS:

private:
    bool isStarted();
    void wait();

    bool beginEnumerate(QList<HWDevice>& devices);
    bool beginGetKeyPool(const QString& fingerprint, QString& desc);
    bool beginSignTx(const QString& fingerprint, QString& psbt);
    bool endEnumerate(QList<HWDevice>& devices);
    bool endGetKeyPool(const QString& fingerprint, QString& desc);
    bool endSignTx(const QString& fingerprint, QString& psbt);

    QtumHwiToolPriv* d;
};

#endif // QTUMHWITOOL_H
