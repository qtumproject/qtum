#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/qtumhwitool.h>
#include <qt/guiutil.h>
#include <qt/execrpccommand.h>
#include <qt/walletmodel.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <qtum/qtumledger.h>
#include <chainparams.h>
#include <outputtype.h>

#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantMap>
#include <QVariantList>
#include <QFile>
#include <QStringList>

#include <atomic>

static const QString PARAM_START_HEIGHT = "start_height";
static const QString PARAM_STOP_HEIGHT = "stop_height";
static const QString PARAM_REQUESTS = "requests";
static const QString PARAM_PSBT = "psbt";
static const QString PARAM_HEXTX = "hextx";
static const QString PARAM_MAXFEERATE = "maxfeerate";
static const QString PARAM_SHOWCONTRACTDATA = "showcontractdata";
static const QString LOAD_FORMAT = ":/ledger/%1_load";
static const QString DELETE_FORMAT = ":/ledger/%1_delete";
static const QString RC_PATH_FORMAT = ":/ledger";
static const int ADDRESS_FROM = 0;
static const int ADDRESS_TO = 1000;

class QtumHwiToolPriv
{
public:
    QtumHwiToolPriv(QObject *parent)
    {
        QStringList optionalRescan = QStringList() << PARAM_START_HEIGHT << PARAM_STOP_HEIGHT;
        cmdRescan = new ExecRPCCommand("rescanblockchain", QStringList(), optionalRescan,  QMap<QString, QString>(), parent);
        QStringList mandatoryImport = QStringList() << PARAM_REQUESTS;
        cmdImport = new ExecRPCCommand("importmulti", mandatoryImport, QStringList(),  QMap<QString, QString>(), parent);
        QStringList mandatoryFinalize = QStringList() << PARAM_PSBT;
        cmdFinalize = new ExecRPCCommand("finalizepsbt", mandatoryFinalize, QStringList(),  QMap<QString, QString>(), parent);
        QStringList mandatorySend = QStringList() << PARAM_HEXTX << PARAM_MAXFEERATE << PARAM_SHOWCONTRACTDATA;
        cmdSend = new ExecRPCCommand("sendrawtransaction", mandatorySend, QStringList(),  QMap<QString, QString>(), parent);
        QStringList mandatoryDecode = QStringList() << PARAM_PSBT;
        cmdDecode = new ExecRPCCommand("decodepsbt", mandatoryDecode, QStringList(),  QMap<QString, QString>(), parent);
    }

    std::atomic<bool> fStarted{false};
    QProcess process;
    QString strStdout;
    QString strError;
    int from = ADDRESS_FROM;
    int to = ADDRESS_TO;

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
    return QString("[ %1 \\ %2 \\ %3 \\ %4 ]").arg(type, model, fingerprint, app_name);
}

bool HWDevice::isValid() const
{
    return fingerprint != "";
}

QString HWDevice::errorMessage() const
{
    return QString("Error: %1\nCode: %2").arg(error, code);
}

HWDevice toHWDevice(const LedgerDevice& device)
{
    HWDevice hwDevice;
    hwDevice.fingerprint = QString::fromStdString(device.fingerprint);
    hwDevice.serial_number = QString::fromStdString(device.serial_number);
    hwDevice.type = QString::fromStdString(device.type);
    hwDevice.path = QString::fromStdString(device.path);
    hwDevice.error = QString::fromStdString(device.error);
    hwDevice.model = QString::fromStdString(device.model);
    hwDevice.code = QString::fromStdString(device.code);
    hwDevice.app_name = QString::fromStdString(device.app_name);
    return hwDevice;
}

QtumHwiTool::QtumHwiTool(QObject *parent) : QObject(parent)
{
    d = new QtumHwiToolPriv(this);
}

QtumHwiTool::~QtumHwiTool()
{
    delete d;
}

bool QtumHwiTool::enumerate(QList<HWDevice> &devices, bool stake)
{
    LOCK(cs_ledger);
    devices.clear();
    std::vector<LedgerDevice> vecDevices;
    if(QtumLedger::instance().enumerate(vecDevices, stake))
    {
        for(LedgerDevice device : vecDevices)
        {
            // Get device info
            HWDevice hwDevice = toHWDevice(device);
            devices.push_back(hwDevice);

            // Set error message
            if(!hwDevice.isValid())
                addError(hwDevice.errorMessage());
        }
    }
    else
    {
        d->strError = QString::fromStdString(QtumLedger::instance().errorMessage());
    }

    return devices.size() > 0;
}

bool QtumHwiTool::isConnected(const QString &fingerprint, bool stake)
{
    LOCK(cs_ledger);
    std::string strFingerprint = fingerprint.toStdString();
    bool ret = QtumLedger::instance().isConnected(strFingerprint, stake);
    if(!ret) d->strError = QString::fromStdString(QtumLedger::instance().errorMessage());
    return ret;
}

bool QtumHwiTool::getKeyPool(const QString &fingerprint, int type, const QString& path, bool internal, QString &desc)
{
    LOCK(cs_ledger);
    std::string strFingerprint = fingerprint.toStdString();
    std::string strDesc = desc.toStdString();
    std::string strPath = path.toStdString();
    if(!strPath.empty())
    {
        strPath += internal ? "/1/*" : "/0/*";
    }
    bool ret = QtumLedger::instance().getKeyPool(strFingerprint, type, strPath, internal, d->from, d->to, strDesc);
    desc = QString::fromStdString(strDesc);
    if(ret)
    {
        desc = "\"" + desc.replace("\"", "\\\"") + "\"";
    }
    else
    {
        d->strError = QString::fromStdString(QtumLedger::instance().errorMessage());
    }
    return ret;
}

bool QtumHwiTool::getKeyPool(const QString &fingerprint, int type, const QString &path, QStringList &descs)
{
    LOCK(cs_ledger);
    bool ret = true;
    QString desc;
    ret &= getKeyPool(fingerprint, type, path, false, desc);
    if(ret) descs.push_back(desc);

    if(!path.isEmpty())
    {
        desc.clear();
        ret &= getKeyPool(fingerprint, type, path, true, desc);
        if(ret) descs.push_back(desc);
    }

    return ret;
}

bool QtumHwiTool::getKeyPoolPKH(const QString &fingerprint, const QString& path, QStringList &descs)
{
    return getKeyPool(fingerprint, (int)OutputType::LEGACY, path, descs);
}

bool QtumHwiTool::getKeyPoolP2SH(const QString &fingerprint, const QString& path, QStringList &descs)
{
    return getKeyPool(fingerprint, (int)OutputType::P2SH_SEGWIT, path, descs);
}

bool QtumHwiTool::getKeyPoolBech32(const QString &fingerprint, const QString& path, QStringList &descs)
{
    return getKeyPool(fingerprint, (int)OutputType::BECH32, path, descs);
}

bool QtumHwiTool::signTx(const QString &fingerprint, QString &psbt)
{
    LOCK(cs_ledger);
    std::string strFingerprint = fingerprint.toStdString();
    std::string strPsbt = psbt.toStdString();
    bool ret = QtumLedger::instance().signTx(strFingerprint, strPsbt);
    psbt = QString::fromStdString(strPsbt);
    if(!ret) d->strError = QString::fromStdString(QtumLedger::instance().errorMessage());
    return ret;
}

bool QtumHwiTool::signMessage(const QString &fingerprint, const QString &message, const QString &path, QString &signature)
{
    LOCK(cs_ledger);
    std::string strFingerprint = fingerprint.toStdString();
    std::string strMessage = message.toStdString();
    std::string strPath = path.toStdString();
    std::string strSignature = signature.toStdString();
    bool ret = QtumLedger::instance().signMessage(strFingerprint, strMessage, strPath, strSignature);
    signature = QString::fromStdString(strSignature);
    if(!ret) d->strError = QString::fromStdString(QtumLedger::instance().errorMessage());
    return ret;
}

bool QtumHwiTool::signDelegate(const QString &fingerprint, QString &psbt)
{
    if(!d->model) return false;

    // Get the delegation data to sign
    std::string strPsbt = psbt.toStdString();
    std::map<int, interfaces::SignDelegation> signData;
    std::string strError;
    if(d->model->wallet().getAddDelegationData(strPsbt, signData, strError) == false)
    {
        d->strError = QString::fromStdString(strError);
        return false;
    }

    // Sign the delegation data
    for (std::map<int, interfaces::SignDelegation>::iterator it = signData.begin(); it != signData.end(); it++)
    {
        QString message = QString::fromStdString(it->second.staker);
        QString path = QString::fromStdString(it->second.delegate);
        QString signature;
        if(signMessage(fingerprint, message, path, signature))
        {
            it->second.PoD = signature.toStdString();
        }
        else
        {
            return false;
        }
    }

    // Update the transaction
    if(signData.size() > 0)
    {
        if(d->model->wallet().setAddDelegationData(strPsbt, signData, strError) == false)
        {
            d->strError = QString::fromStdString(strError);
            return false;
        }
        psbt = QString::fromStdString(strPsbt);
    }

    return true;
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
        bool wasStarted = false;
        if(d->process.waitForStarted())
        {
            wasStarted = true;
            d->process.waitForFinished(-1);
        }
        d->strStdout = d->process.readAllStandardOutput();
        d->strError = d->process.readAllStandardError();
        d->fStarted = false;
        if(!wasStarted && d->strError.isEmpty())
        {
            d->strError = tr("Application %1 fail to start.").arg(d->process.program());
        }
    }
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

bool QtumHwiTool::importAddresses(const QString &desc)
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

bool QtumHwiTool::importMulti(const QStringList &descs)
{
    bool ret = true;
    for(QString desc : descs)
    {
        ret &= importAddresses(desc);
        if(!ret) break;
    }

    return ret;
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

bool QtumHwiTool::sendRawTransaction(const QString &hexTx, QVariantMap& variantMap)
{
    if(!d->model) return false;

    // Add params for RPC
    QMap<QString, QString> lstParams;
    QVariant result;
    QString resultStr;
    ExecRPCCommand::appendParam(lstParams, PARAM_HEXTX, hexTx);
    ExecRPCCommand::appendParam(lstParams, PARAM_MAXFEERATE, "null");
    ExecRPCCommand::appendParam(lstParams, PARAM_SHOWCONTRACTDATA, "true");

    // Exec RPC
    if(!execRPC(d->cmdSend, lstParams, result, resultStr))
        return false;

    // Parse results
    std::string strHash = resultStr.toStdString();
    if(strHash.length() == 64 && IsHex(strHash))
    {
        variantMap["txid"] = resultStr;
    }
    else
    {
        variantMap = result.toMap();
    }

    return variantMap.contains("txid");
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

QString QtumHwiTool::derivationPathPKH()
{
    std::string path = QtumLedger::instance().derivationPath((int)OutputType::LEGACY);
    return QString::fromStdString(path);
}

QString QtumHwiTool::derivationPathP2SH()
{
    std::string path = QtumLedger::instance().derivationPath((int)OutputType::P2SH_SEGWIT);
    return QString::fromStdString(path);
}

QString QtumHwiTool::derivationPathBech32()
{
    std::string path = QtumLedger::instance().derivationPath((int)OutputType::BECH32);
    return QString::fromStdString(path);
}
