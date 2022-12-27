#ifndef HARDWARESIGNTX_H
#define HARDWARESIGNTX_H

#include <QObject>
#include <QWidget>
#include <QString>
#include <QList>
#include <QVariantMap>
class WalletModel;
class QtumHwiTool;

/**
 * @brief The HardwareSignTx class Communicate with the Qtum Hardware Wallet Interface Tool
 */
class HardwareSignTx : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief HardwareSignTx Constructor
     * @param parent Parent object
     */
    explicit HardwareSignTx(QWidget *widget);

    /**
     * @brief ~HardwareSignTx Destructor
     */
    ~HardwareSignTx();

    /**
     * @brief setModel Set wallet model
     * @param model Wallet model
     */
    void setModel(WalletModel *model);

    /**
     * @brief setPsbt Set psbt transaction
     * @param psbt Raw transaction
     */
    void setPsbt(const QString& psbt);

    /**
     * @brief askDevice Ask for hardware device
     * @param stake Use the device for staking
     * @param pFingerprint Pointer to selected ledger fingerprint
     * @return success of the operation
     */
    bool askDevice(bool stake = false, QString* pFingerprint = nullptr);

    /**
     * @brief sign Sign transaction
     * @return success of the operation
     */
    bool sign();

    /**
     * @brief send Send transaction
     * @param result
     * @return success of the operation
     */
    bool send(QVariantMap& result);

    /**
     * @brief process Process transaction
     * @param widget Parent widget
     * @param model Wallet model
     * @param psbt Transaction
     * @param result Result to update
     * @param send Send the transaction
     * @return success of the operation
     */
    static bool process(QWidget *widget, WalletModel *model, const QString& psbt, QVariantMap& result, bool send = true);

Q_SIGNALS:

public Q_SLOTS:

public:
    WalletModel* model = 0;
    QtumHwiTool* tool = 0;
    QString psbt;
    QString hexTx;
    bool complete = false;

private:
    QWidget *widget = 0;
};

#endif // HARDWARESIGNTX_H
