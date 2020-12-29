#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/qtumhwitool.h>
#include <qt/guiutil.h>
#include <qt/execrpccommand.h>
#include <qt/walletmodel.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <chainparams.h>
#include <outputtype.h>

#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantMap>
#include <QVariantList>

#include <atomic>

static const QString PARAM_START_HEIGHT = "start_height";
static const QString PARAM_STOP_HEIGHT = "stop_height";
static const QString PARAM_REQUESTS = "requests";
static const QString PARAM_PSBT = "psbt";
static const QString PARAM_HEXTX = "hextx";

class QtumHwiToolPriv
{
public:
    QtumHwiToolPriv(QObject *parent)
    {
        toolPath = GUIUtil::getHwiToolPath();
        QStringList optionalRescan = QStringList() << PARAM_START_HEIGHT << PARAM_STOP_HEIGHT;
        cmdRescan = new ExecRPCCommand("rescanblockchain", QStringList(), optionalRescan,  QMap<QString, QString>(), parent);
        QStringList mandatoryImport = QStringList() << PARAM_REQUESTS;
        cmdImport = new ExecRPCCommand("importmulti", mandatoryImport, QStringList(),  QMap<QString, QString>(), parent);
        QStringList mandatoryFinalize = QStringList() << PARAM_PSBT;
        cmdFinalize = new ExecRPCCommand("finalizepsbt", mandatoryFinalize, QStringList(),  QMap<QString, QString>(), parent);
        QStringList mandatorySend = QStringList() << PARAM_HEXTX;
        cmdSend = new ExecRPCCommand("finalizepsbt", mandatorySend, QStringList(),  QMap<QString, QString>(), parent);
        QStringList mandatoryDecode = QStringList() << PARAM_PSBT;
        cmdDecode = new ExecRPCCommand("decodepsbt", mandatoryDecode, QStringList(),  QMap<QString, QString>(), parent);
        if(gArgs.GetChainName() != CBaseChainParams::MAIN)
        {
            arguments << "--testnet";
        }
    }

    std::atomic<bool> fStarted{false};
    QProcess process;
    QString strStdout;
    QString strError;
    QString toolPath;
    QStringList arguments;

    ExecRPCCommand* cmdRescan = 0;
    ExecRPCCommand* cmdImport = 0;
    ExecRPCCommand* cmdFinalize = 0;
    ExecRPCCommand* cmdSend = 0;
    ExecRPCCommand* cmdDecode = 0;
    WalletModel* model = 0;
};

HWDevice::HWDevice()
{}

QString HWDevice::toString() const
{
    return QString("[ %1 \\ %2 \\ %3 \\ %4 ]").arg(type, model, path, fingerprint);
}

bool HWDevice::isValid() const
{
    return fingerprint != "";
}

QString HWDevice::errorMessage() const
{
    return QString("Error: %1\nCode: %2").arg(error, code);
}

QtumHwiTool::QtumHwiTool(QObject *parent) : QObject(parent)
{
    d = new QtumHwiToolPriv(this);
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

bool QtumHwiTool::getKeyPool(const QString &fingerprint, int type, QString &desc)
{
    // Get the key pool for a device
    if(isStarted())
        return false;

    if(!beginGetKeyPool(fingerprint, type, desc))
        return false;

    wait();

    return endGetKeyPool(fingerprint, type, desc);
}

bool QtumHwiTool::getKeyPoolPKH(const QString &fingerprint, QString &desc)
{
    return getKeyPool(fingerprint, (int)OutputType::LEGACY, desc);
}

bool QtumHwiTool::getKeyPoolP2SH(const QString &fingerprint, QString &desc)
{
    return getKeyPool(fingerprint, (int)OutputType::P2SH_SEGWIT, desc);
}

bool QtumHwiTool::getKeyPoolBech32(const QString &fingerprint, QString &desc)
{
    return getKeyPool(fingerprint, (int)OutputType::BECH32, desc);
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

    return d->strError;
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
        d->strError = d->process.readAllStandardError();
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

bool QtumHwiTool::beginGetKeyPool(const QString &fingerprint, int type, QString &desc)
{
    Q_UNUSED(desc);

    // Get the output type
    QString descType;
    switch (type) {
    case (int)OutputType::P2SH_SEGWIT:
        descType = "--sh_wpkh";
        break;
    case (int)OutputType::BECH32:
        descType = "--wpkh";
        break;
    default:
        break;
    }

    // Execute command line
    QStringList arguments = d->arguments;
    arguments << "-f" << fingerprint << "getkeypool" << descType << "0" << "1000";
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
        device.model = data["model"].toString();
        device.code = data["code"].toString();
        devices.push_back(device);

        // Set error message
        if(!device.isValid())
            addError(device.errorMessage());
    }

    return devices.size() > 0;
}

bool QtumHwiTool::endGetKeyPool(const QString &fingerprint, int type, QString &desc)
{
    Q_UNUSED(fingerprint);
    Q_UNUSED(type);

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

bool QtumHwiTool::rescanBlockchain(int startHeight, int stopHeight)
{
    if(!d->model) return false;

    // Add params for RPC
    QMap<QString, QString> lstParams;
    QVariant result;
    QString resultJson;
    ExecRPCCommand::appendParam(lstParams, PARAM_START_HEIGHT, QString::number(startHeight));
    if(stopHeight > -1)
    {
        ExecRPCCommand::appendParam(lstParams, PARAM_STOP_HEIGHT, QString::number(stopHeight));
    }

    // Exec RPC
    if(!execRPC(d->cmdRescan, lstParams, result, resultJson))
        return false;

    // Parse results
    QVariantMap variantMap = result.toMap();
    int resStartHeight = variantMap.value(PARAM_START_HEIGHT).toInt();
    int resStopHeight = variantMap.value(PARAM_STOP_HEIGHT).toInt();

    return resStartHeight < resStopHeight;
}

bool QtumHwiTool::importMulti(const QString &desc)
{
    if(!d->model) return false;

    // Add params for RPC
    QMap<QString, QString> lstParams;
    QVariant result;
    QString resultJson;
    ExecRPCCommand::appendParam(lstParams, PARAM_REQUESTS, desc);

    // Exec RPC
    if(!execRPC(d->cmdImport, lstParams, result, resultJson))
        return false;

    // Parse results
    int countSuccess = 0;
    QVariantList variantList = result.toList();
    for(const QVariant& item : variantList)
    {
        QVariantMap variantMap = item.toMap();
        if(variantMap.value("success").toBool())
            countSuccess++;
    }

    return countSuccess > 0;
}

bool QtumHwiTool::finalizePsbt(const QString &psbt, QString &hexTx, bool &complete)
{
    if(!d->model) return false;

    // Add params for RPC
    QMap<QString, QString> lstParams;
    QVariant result;
    QString resultJson;
    ExecRPCCommand::appendParam(lstParams, PARAM_PSBT, psbt);

    // Exec RPC
    if(!execRPC(d->cmdFinalize, lstParams, result, resultJson))
        return false;

    // Parse results
    QVariantMap variantMap = result.toMap();
    hexTx = variantMap.value("hex").toString();
    complete = variantMap.value("complete").toBool();

    return true;
}

bool QtumHwiTool::sendRawTransaction(const QString &hexTx)
{
    if(!d->model) return false;

    // Add params for RPC
    QMap<QString, QString> lstParams;
    QVariant result;
    QString resultStr;
    ExecRPCCommand::appendParam(lstParams, PARAM_HEXTX, hexTx);

    // Exec RPC
    if(!execRPC(d->cmdSend, lstParams, result, resultStr))
        return false;

    // Parse results
    std::string strHash = resultStr.toStdString();

    return strHash.length() == 64 && IsHex(strHash);
}

bool QtumHwiTool::decodePsbt(const QString &psbt, QString &decoded)
{
    if(!d->model) return false;

    // Add params for RPC
    QMap<QString, QString> lstParams;
    QVariant result;
    QString resultStr;
    ExecRPCCommand::appendParam(lstParams, PARAM_PSBT, psbt);

    // Exec RPC
    if(!execRPC(d->cmdDecode, lstParams, result, resultStr))
        return false;

    // Parse results
    decoded = resultStr;

    return true;
}

void QtumHwiTool::setModel(WalletModel *model)
{
    d->model = model;
}

bool QtumHwiTool::execRPC(ExecRPCCommand *cmd, const QMap<QString, QString> &lstParams, QVariant &result, QString &resultJson)
{
    d->strError.clear();
    if(!cmd->exec(d->model->node(), d->model, lstParams, result, resultJson, d->strError))
        return false;

    return true;
}

void QtumHwiTool::addError(const QString &error)
{
    if(d->strError != "")
        d->strError += "\n";
    d->strError += error;
}
