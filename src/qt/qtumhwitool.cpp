#include <qt/qtumhwitool.h>

#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantMap>

#include <atomic>

class QtumHwiToolPriv
{
public:
    std::atomic<bool> fStarted{false};
    QProcess process;
    QString strStdout;
    QString strStderr;
    QString toolPath;
    QStringList arguments;
};

QtumHwiTool::QtumHwiTool(QObject *parent) : QObject(parent)
{
    d = new QtumHwiToolPriv();
}

QtumHwiTool::~QtumHwiTool()
{
    delete d;
}

bool QtumHwiTool::enumerate(QList<HWDevice> &devices)
{
    // Enumerate hardware wallet devices
    if(isStarted())
        return false;

    if(!beginEnumerate(devices))
        return false;

    wait();

    return endEnumerate(devices);
}

bool QtumHwiTool::isConnected(const QString &fingerprint)
{
    // Check if a device is connected
    QList<HWDevice> devices;
    if(enumerate(devices))
    {
        for(HWDevice device: devices)
        {
            if(device.fingerprint == fingerprint)
                return true;
        }
    }
    return false;
}

bool QtumHwiTool::getKeyPool(const QString &fingerprint, QString &desc)
{
    // Get the key pool for a device
    if(isStarted())
        return false;

    if(!beginGetKeyPool(fingerprint, desc))
        return false;

    wait();

    return endGetKeyPool(fingerprint, desc);
}

bool QtumHwiTool::signTx(const QString &fingerprint, QString &psbt)
{
    // Sign PSBT transaction
    if(isStarted())
        return false;

    if(!beginSignTx(fingerprint, psbt))
        return false;

    wait();

    return endSignTx(fingerprint, psbt);
}

QString QtumHwiTool::errorMessage()
{
    // Get the last error message
    if(d->fStarted)
        return tr("Started");

    return d->strStderr;
}

bool QtumHwiTool::isStarted()
{
    return d->fStarted;
}

void QtumHwiTool::wait()
{
    if(d->fStarted)
    {
        d->process.waitForFinished(-1);
        d->strStdout = d->process.readAllStandardOutput();
        d->strStderr = d->process.readAllStandardError();
        d->fStarted = false;
    }
}

bool QtumHwiTool::beginEnumerate(QList<HWDevice> &devices)
{
    Q_UNUSED(devices);

    // Execute command line
    QStringList arguments = d->arguments;
    arguments << "enumerate";
    d->process.start(d->toolPath, arguments);
    d->fStarted = true;

    return d->fStarted;
}

bool QtumHwiTool::beginGetKeyPool(const QString &fingerprint, QString &desc)
{
    Q_UNUSED(desc);

    // Execute command line
    QStringList arguments = d->arguments;
    arguments << "-f" << fingerprint << "getkeypool" << "--wpkh" << "0" << "1000";
    d->process.start(d->toolPath, arguments);
    d->fStarted = true;

    return d->fStarted;
}

bool QtumHwiTool::beginSignTx(const QString &fingerprint, QString &psbt)
{
    // Execute command line
    QStringList arguments = d->arguments;
    arguments << "-f" << fingerprint << "signtx" << psbt;
    d->process.start(d->toolPath, arguments);
    d->fStarted = true;

    return d->fStarted;
}

bool QtumHwiTool::endEnumerate(QList<HWDevice> &devices)
{
    // Decode command line results
    QJsonDocument jsonDocument = QJsonDocument::fromJson(d->strStdout.toUtf8());
    QJsonArray jsonDevices = jsonDocument.array();
    for(QJsonValue jsonDevice : jsonDevices)
    {
        if(!jsonDevice.isObject())
            return false;

        // Get device info
        QVariantMap data = jsonDevice.toObject().toVariantMap();
        HWDevice device;
        device.fingerprint = data["fingerprint"].toString();
        device.serial_number = data["serial_number"].toString();
        device.type = data["type"].toString();
        device.path = data["path"].toString();
        device.error = data["error"].toString();
        devices.push_back(device);
    }

    return devices.size() > 0;
}

bool QtumHwiTool::endGetKeyPool(const QString &fingerprint, QString &desc)
{
    Q_UNUSED(fingerprint);

    // Decode command line results
    desc = d->strStdout;

    return d->strStdout.contains("desc");
}

bool QtumHwiTool::endSignTx(const QString &fingerprint, QString &psbt)
{
    Q_UNUSED(fingerprint);

    // Decode command line results
    QJsonDocument jsonDocument = QJsonDocument::fromJson(d->strStdout.toUtf8());
    QVariantMap data = jsonDocument.object().toVariantMap();
    QString psbtSigned = data["psbt"].toString();
    if(!psbtSigned.isEmpty())
    {
        psbt = psbtSigned;
        return true;
    }

    return false;
}
