// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/optionsmodel.h>

#include <qt/bitcoinunits.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>

#include <interfaces/node.h>
#include <mapport.h>
#include <net.h>
#include <netbase.h>
#include <txdb.h>       // for -dbcache defaults
#include <util/string.h>
#include <validation.h> // For DEFAULT_SCRIPTCHECK_THREADS

#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#include <wallet/walletdb.h>
#endif
#include <QDebug>
#include <QLatin1Char>
#include <QSettings>
#include <QStringList>
#include <util/moneystr.h>
//using namespace wallet;

const char *DEFAULT_GUI_PROXY_HOST = "127.0.0.1";

static const QString GetDefaultProxyAddress();

OptionsModel::OptionsModel(QObject *parent, bool resetSettings) :
    QAbstractListModel(parent), restartApp(false)
{
    Init(resetSettings);
}

void OptionsModel::addOverriddenOption(const std::string &option)
{
    strOverriddenByCommandLine += QString::fromStdString(option) + "=" + QString::fromStdString(gArgs.GetArg(option, "")) + " ";
}

// Writes all missing QSettings with their default values
void OptionsModel::Init(bool resetSettings)
{
    if (resetSettings)
        Reset();

    checkAndMigrate();

    QSettings settings;

    // Ensure restart flag is unset on client startup
    setRestartRequired(false);

    // These are Qt-only settings:

    // Window
    if (!settings.contains("fHideTrayIcon")) {
        settings.setValue("fHideTrayIcon", false);
    }
    m_show_tray_icon = !settings.value("fHideTrayIcon").toBool();
    Q_EMIT showTrayIconChanged(m_show_tray_icon);

    if (!settings.contains("fMinimizeToTray"))
        settings.setValue("fMinimizeToTray", false);
    fMinimizeToTray = settings.value("fMinimizeToTray").toBool() && m_show_tray_icon;

    if (!settings.contains("fMinimizeOnClose"))
        settings.setValue("fMinimizeOnClose", false);
    fMinimizeOnClose = settings.value("fMinimizeOnClose").toBool();

    // Display
    if (!settings.contains("nDisplayUnit"))
        settings.setValue("nDisplayUnit", BitcoinUnits::BTC);
    nDisplayUnit = settings.value("nDisplayUnit").toInt();

    if (!settings.contains("strThirdPartyTxUrls"))
        settings.setValue("strThirdPartyTxUrls", "");
    strThirdPartyTxUrls = settings.value("strThirdPartyTxUrls", "").toString();

    if (!settings.contains("fCoinControlFeatures"))
        settings.setValue("fCoinControlFeatures", false);
    fCoinControlFeatures = settings.value("fCoinControlFeatures", false).toBool();

    if (!settings.contains("enable_psbt_controls")) {
        settings.setValue("enable_psbt_controls", false);
    }
    m_enable_psbt_controls = settings.value("enable_psbt_controls", false).toBool();

    // These are shared with the core or have a command-line parameter
    // and we want command-line parameters to overwrite the GUI settings.
    //
    // If setting doesn't exist create it with defaults.
    //
    // If gArgs.SoftSetArg() or gArgs.SoftSetBoolArg() return false we were overridden
    // by command-line and show this in the UI.

    // Main
    if (!settings.contains("bPrune"))
        settings.setValue("bPrune", false);
    if (!settings.contains("nPruneSize"))
        settings.setValue("nPruneSize", DEFAULT_PRUNE_TARGET_GB);
    SetPruneEnabled(settings.value("bPrune").toBool());

    if (!settings.contains("nDatabaseCache"))
        settings.setValue("nDatabaseCache", (qint64)nDefaultDbCache);
    if (!gArgs.SoftSetArg("-dbcache", settings.value("nDatabaseCache").toString().toStdString()))
        addOverriddenOption("-dbcache");
#ifdef ENABLE_WALLET
    if (!settings.contains("fSuperStaking"))
        settings.setValue("fSuperStaking", false);
    bool fSuperStaking = settings.value("fSuperStaking").toBool();
    if (!gArgs.SoftSetBoolArg("-superstaking", fSuperStaking))
        addOverriddenOption("-superstaking");
    if(fSuperStaking)
    {
        if (!gArgs.SoftSetBoolArg("-staking", true))
            addOverriddenOption("-staking");
        if (!gArgs.SoftSetBoolArg("-logevents", true))
            addOverriddenOption("-logevents");
        if (!gArgs.SoftSetBoolArg("-addrindex", true))
            addOverriddenOption("-addrindex");
    }
#endif

    if (!settings.contains("fLogEvents"))
        settings.setValue("fLogEvents", fLogEvents);
    if (!gArgs.SoftSetBoolArg("-logevents", settings.value("fLogEvents").toBool()))
        addOverriddenOption("-logevents");

#ifdef ENABLE_WALLET
    if (!settings.contains("nReserveBalance"))
        settings.setValue("nReserveBalance", (long long)wallet::DEFAULT_RESERVE_BALANCE);
    if (!gArgs.SoftSetArg("-reservebalance", FormatMoney(settings.value("nReserveBalance").toLongLong())))
        addOverriddenOption("-reservebalance");
#endif
    if (!settings.contains("nThreadsScriptVerif"))
        settings.setValue("nThreadsScriptVerif", DEFAULT_SCRIPTCHECK_THREADS);
    if (!gArgs.SoftSetArg("-par", settings.value("nThreadsScriptVerif").toString().toStdString()))
        addOverriddenOption("-par");

    if (!settings.contains("strDataDir"))
        settings.setValue("strDataDir", GUIUtil::getDefaultDataDirectory());

    // Wallet
#ifdef ENABLE_WALLET
    if (!settings.contains("bSpendZeroConfChange"))
        settings.setValue("bSpendZeroConfChange", true);
    if (!gArgs.SoftSetBoolArg("-spendzeroconfchange", settings.value("bSpendZeroConfChange").toBool()))
        addOverriddenOption("-spendzeroconfchange");

    if (!settings.contains("external_signer_path"))
        settings.setValue("external_signer_path", "");

    if (!gArgs.SoftSetArg("-signer", settings.value("external_signer_path").toString().toStdString())) {
        addOverriddenOption("-signer");
    }

    if (!settings.contains("bZeroBalanceAddressToken"))
        settings.setValue("bZeroBalanceAddressToken", wallet::DEFAULT_ZERO_BALANCE_ADDRESS_TOKEN);
    bZeroBalanceAddressToken = settings.value("bZeroBalanceAddressToken").toBool();

    if (!settings.contains("signPSBTWithHWITool"))
        settings.setValue("signPSBTWithHWITool", wallet::DEFAULT_SIGN_PSBT_WITH_HWI_TOOL);
    if (!gArgs.SoftSetBoolArg("-signpsbtwithhwitool", settings.value("signPSBTWithHWITool").toBool()))
        addOverriddenOption("-signpsbtwithhwitool");

    if (!settings.contains("SubFeeFromAmount")) {
        settings.setValue("SubFeeFromAmount", false);
    }
    m_sub_fee_from_amount = settings.value("SubFeeFromAmount", false).toBool();
#endif

    if (!settings.contains("fCheckForUpdates"))
        settings.setValue("fCheckForUpdates", DEFAULT_CHECK_FOR_UPDATES);
    fCheckForUpdates = settings.value("fCheckForUpdates").toBool();

    // Network
    if (!settings.contains("fUseUPnP"))
        settings.setValue("fUseUPnP", DEFAULT_UPNP);
    if (!gArgs.SoftSetBoolArg("-upnp", settings.value("fUseUPnP").toBool()))
        addOverriddenOption("-upnp");

    if (!settings.contains("fUseNatpmp")) {
        settings.setValue("fUseNatpmp", DEFAULT_NATPMP);
    }
    if (!gArgs.SoftSetBoolArg("-natpmp", settings.value("fUseNatpmp").toBool())) {
        addOverriddenOption("-natpmp");
    }

    if (!settings.contains("fListen"))
        settings.setValue("fListen", DEFAULT_LISTEN);
    if (!gArgs.SoftSetBoolArg("-listen", settings.value("fListen").toBool())) {
        addOverriddenOption("-listen");
    } else if (!settings.value("fListen").toBool()) {
        gArgs.SoftSetBoolArg("-listenonion", false);
    }

    if (!settings.contains("server")) {
        settings.setValue("server", false);
    }
    if (!gArgs.SoftSetBoolArg("-server", settings.value("server").toBool())) {
        addOverriddenOption("-server");
    }

#ifdef ENABLE_WALLET
    if (!settings.contains("fUseChangeAddress"))
    {
        // Set the default value
        bool useChangeAddress = wallet::DEFAULT_USE_CHANGE_ADDRESS;

        // Get the old parameter value if exist
        if(settings.contains("fNotUseChangeAddress"))
            useChangeAddress = !settings.value("fNotUseChangeAddress").toBool();

        // Set the parameter value
        settings.setValue("fUseChangeAddress", useChangeAddress);
    }
    if (!gArgs.SoftSetBoolArg("-usechangeaddress", settings.value("fUseChangeAddress").toBool()))
        addOverriddenOption("-usechangeaddress");
#endif

    if (!settings.contains("fUseProxy"))
        settings.setValue("fUseProxy", false);
    if (!settings.contains("addrProxy"))
        settings.setValue("addrProxy", GetDefaultProxyAddress());
    // Only try to set -proxy, if user has enabled fUseProxy
    if ((settings.value("fUseProxy").toBool() && !gArgs.SoftSetArg("-proxy", settings.value("addrProxy").toString().toStdString())))
        addOverriddenOption("-proxy");
    else if(!settings.value("fUseProxy").toBool() && !gArgs.GetArg("-proxy", "").empty())
        addOverriddenOption("-proxy");

    if (!settings.contains("fUseSeparateProxyTor"))
        settings.setValue("fUseSeparateProxyTor", false);
    if (!settings.contains("addrSeparateProxyTor"))
        settings.setValue("addrSeparateProxyTor", GetDefaultProxyAddress());
    // Only try to set -onion, if user has enabled fUseSeparateProxyTor
    if ((settings.value("fUseSeparateProxyTor").toBool() && !gArgs.SoftSetArg("-onion", settings.value("addrSeparateProxyTor").toString().toStdString())))
        addOverriddenOption("-onion");
    else if(!settings.value("fUseSeparateProxyTor").toBool() && !gArgs.GetArg("-onion", "").empty())
        addOverriddenOption("-onion");

    // Display
    if (!settings.contains("language"))
        settings.setValue("language", "");
    if (!gArgs.SoftSetArg("-lang", settings.value("language").toString().toStdString()))
        addOverriddenOption("-lang");

    language = settings.value("language").toString();

    if (!settings.contains("UseEmbeddedMonospacedFont")) {
        settings.setValue("UseEmbeddedMonospacedFont", "true");
    }
    m_use_embedded_monospaced_font = settings.value("UseEmbeddedMonospacedFont").toBool();
    Q_EMIT useEmbeddedMonospacedFontChanged(m_use_embedded_monospaced_font);

    if (!settings.contains("Theme"))
        settings.setValue("Theme", "");

    theme = settings.value("Theme").toString();

#ifdef ENABLE_WALLET
    if (!settings.contains("HWIToolPath"))
        settings.setValue("HWIToolPath", "");

    if (!gArgs.SoftSetArg("-hwitoolpath", settings.value("HWIToolPath").toString().toStdString()))
        addOverriddenOption("-hwitoolpath");

    if (!settings.contains("StakeLedgerId"))
        settings.setValue("StakeLedgerId", "");

    if (!gArgs.SoftSetArg("-stakerledgerid", settings.value("StakeLedgerId").toString().toStdString()))
        addOverriddenOption("-stakerledgerid");
#endif

}

/** Helper function to copy contents from one QSettings to another.
 * By using allKeys this also covers nested settings in a hierarchy.
 */
static void CopySettings(QSettings& dst, const QSettings& src)
{
    for (const QString& key : src.allKeys()) {
        dst.setValue(key, src.value(key));
    }
}

/** Back up a QSettings to an ini-formatted file. */
static void BackupSettings(const fs::path& filename, const QSettings& src)
{
    qInfo() << "Backing up GUI settings to" << GUIUtil::PathToQString(filename);
    QSettings dst(GUIUtil::PathToQString(filename), QSettings::IniFormat);
    dst.clear();
    CopySettings(dst, src);
}

void OptionsModel::Reset()
{
    QSettings settings;

    // Backup old settings to chain-specific datadir for troubleshooting
    BackupSettings(gArgs.GetDataDirNet() / "guisettings.ini.bak", settings);

    // Save the strDataDir setting
    QString dataDir = GUIUtil::getDefaultDataDirectory();
    dataDir = settings.value("strDataDir", dataDir).toString();

    // Remove all entries from our QSettings object
    settings.clear();

    // Set strDataDir
    settings.setValue("strDataDir", dataDir);

    // Set that this was reset
    settings.setValue("fReset", true);

    // default setting for OptionsModel::StartAtStartup - disabled
    if (GUIUtil::GetStartOnSystemStartup())
        GUIUtil::SetStartOnSystemStartup(false);
}

int OptionsModel::rowCount(const QModelIndex & parent) const
{
    return OptionIDRowCount;
}

struct ProxySetting {
    bool is_set;
    QString ip;
    QString port;
};

static ProxySetting GetProxySetting(QSettings &settings, const QString &name)
{
    static const ProxySetting default_val = {false, DEFAULT_GUI_PROXY_HOST, QString("%1").arg(DEFAULT_GUI_PROXY_PORT)};
    // Handle the case that the setting is not set at all
    if (!settings.contains(name)) {
        return default_val;
    }
    // contains IP at index 0 and port at index 1
    QStringList ip_port = GUIUtil::SplitSkipEmptyParts(settings.value(name).toString(), ":");
    if (ip_port.size() == 2) {
        return {true, ip_port.at(0), ip_port.at(1)};
    } else { // Invalid: return default
        return default_val;
    }
}

static void SetProxySetting(QSettings &settings, const QString &name, const ProxySetting &ip_port)
{
    settings.setValue(name, QString{ip_port.ip + QLatin1Char(':') + ip_port.port});
}

static const QString GetDefaultProxyAddress()
{
    return QString("%1:%2").arg(DEFAULT_GUI_PROXY_HOST).arg(DEFAULT_GUI_PROXY_PORT);
}

void OptionsModel::SetPruneEnabled(bool prune, bool force)
{
    QSettings settings;
    settings.setValue("bPrune", prune);
    const int64_t prune_target_mib = PruneGBtoMiB(settings.value("nPruneSize").toInt());
    std::string prune_val = prune ? ToString(prune_target_mib) : "0";
    if (force) {
        gArgs.ForceSetArg("-prune", prune_val);
        return;
    }
    if (!gArgs.SoftSetArg("-prune", prune_val)) {
        addOverriddenOption("-prune");
    }
}

void OptionsModel::SetPruneTargetGB(int prune_target_gb, bool force)
{
    const bool prune = prune_target_gb > 0;
    if (prune) {
        QSettings settings;
        settings.setValue("nPruneSize", prune_target_gb);
    }
    SetPruneEnabled(prune, force);
}

// read QSettings values and return them
QVariant OptionsModel::data(const QModelIndex & index, int role) const
{
    if(role == Qt::EditRole)
    {
        QSettings settings;
        switch(index.row())
        {
        case StartAtStartup:
            return GUIUtil::GetStartOnSystemStartup();
        case ShowTrayIcon:
            return m_show_tray_icon;
        case MinimizeToTray:
            return fMinimizeToTray;
        case MapPortUPnP:
#ifdef USE_UPNP
            return settings.value("fUseUPnP");
#else
            return false;
#endif // USE_UPNP
        case MapPortNatpmp:
#ifdef USE_NATPMP
            return settings.value("fUseNatpmp");
#else
            return false;
#endif // USE_NATPMP
        case MinimizeOnClose:
            return fMinimizeOnClose;

        // default proxy
        case ProxyUse:
            return settings.value("fUseProxy", false);
        case ProxyIP:
            return GetProxySetting(settings, "addrProxy").ip;
        case ProxyPort:
            return GetProxySetting(settings, "addrProxy").port;

        // separate Tor proxy
        case ProxyUseTor:
            return settings.value("fUseSeparateProxyTor", false);
        case ProxyIPTor:
            return GetProxySetting(settings, "addrSeparateProxyTor").ip;
        case ProxyPortTor:
            return GetProxySetting(settings, "addrSeparateProxyTor").port;

#ifdef ENABLE_WALLET
        case SpendZeroConfChange:
            return settings.value("bSpendZeroConfChange");
        case ExternalSignerPath:
            return settings.value("external_signer_path");
        case SubFeeFromAmount:
            return m_sub_fee_from_amount;
        case ZeroBalanceAddressToken:
            return settings.value("bZeroBalanceAddressToken");
        case ReserveBalance:
            return settings.value("nReserveBalance");
        case SignPSBTWithHWITool:
            return settings.value("signPSBTWithHWITool");
#endif
        case DisplayUnit:
            return nDisplayUnit;
        case ThirdPartyTxUrls:
            return strThirdPartyTxUrls;
        case Language:
            return settings.value("language");
        case UseEmbeddedMonospacedFont:
            return m_use_embedded_monospaced_font;
        case CoinControlFeatures:
            return fCoinControlFeatures;
        case EnablePSBTControls:
            return settings.value("enable_psbt_controls");
        case Prune:
            return settings.value("bPrune");
        case PruneSize:
            return settings.value("nPruneSize");
        case DatabaseCache:
            return settings.value("nDatabaseCache");
        case LogEvents:
            return settings.value("fLogEvents");
#ifdef ENABLE_WALLET
        case SuperStaking:
            return settings.value("fSuperStaking");
#endif
        case ThreadsScriptVerif:
            return settings.value("nThreadsScriptVerif");
        case Listen:
            return settings.value("fListen");
        case Server:
            return settings.value("server");
#ifdef ENABLE_WALLET
        case UseChangeAddress:
            return settings.value("fUseChangeAddress");
#endif
        case CheckForUpdates:
            return settings.value("fCheckForUpdates");
        case Theme:
            return settings.value("Theme");
#ifdef ENABLE_WALLET
        case HWIToolPath:
            return settings.value("HWIToolPath");
        case StakeLedgerId:
            return settings.value("StakeLedgerId");
#endif
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// write QSettings values
bool OptionsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    bool successful = true; /* set to false on parse error */
    if(role == Qt::EditRole)
    {
        QSettings settings;
        switch(index.row())
        {
        case StartAtStartup:
            successful = GUIUtil::SetStartOnSystemStartup(value.toBool());
            break;
        case ShowTrayIcon:
            m_show_tray_icon = value.toBool();
            settings.setValue("fHideTrayIcon", !m_show_tray_icon);
            Q_EMIT showTrayIconChanged(m_show_tray_icon);
            break;
        case MinimizeToTray:
            fMinimizeToTray = value.toBool();
            settings.setValue("fMinimizeToTray", fMinimizeToTray);
            break;
        case MapPortUPnP: // core option - can be changed on-the-fly
            settings.setValue("fUseUPnP", value.toBool());
            break;
        case MapPortNatpmp: // core option - can be changed on-the-fly
            settings.setValue("fUseNatpmp", value.toBool());
            break;
        case MinimizeOnClose:
            fMinimizeOnClose = value.toBool();
            settings.setValue("fMinimizeOnClose", fMinimizeOnClose);
            break;

        // default proxy
        case ProxyUse:
            if (settings.value("fUseProxy") != value) {
                settings.setValue("fUseProxy", value.toBool());
                setRestartRequired(true);
            }
            break;
        case ProxyIP: {
            auto ip_port = GetProxySetting(settings, "addrProxy");
            if (!ip_port.is_set || ip_port.ip != value.toString()) {
                ip_port.ip = value.toString();
                SetProxySetting(settings, "addrProxy", ip_port);
                setRestartRequired(true);
            }
        }
        break;
        case ProxyPort: {
            auto ip_port = GetProxySetting(settings, "addrProxy");
            if (!ip_port.is_set || ip_port.port != value.toString()) {
                ip_port.port = value.toString();
                SetProxySetting(settings, "addrProxy", ip_port);
                setRestartRequired(true);
            }
        }
        break;

        // separate Tor proxy
        case ProxyUseTor:
            if (settings.value("fUseSeparateProxyTor") != value) {
                settings.setValue("fUseSeparateProxyTor", value.toBool());
                setRestartRequired(true);
            }
            break;
        case ProxyIPTor: {
            auto ip_port = GetProxySetting(settings, "addrSeparateProxyTor");
            if (!ip_port.is_set || ip_port.ip != value.toString()) {
                ip_port.ip = value.toString();
                SetProxySetting(settings, "addrSeparateProxyTor", ip_port);
                setRestartRequired(true);
            }
        }
        break;
        case ProxyPortTor: {
            auto ip_port = GetProxySetting(settings, "addrSeparateProxyTor");
            if (!ip_port.is_set || ip_port.port != value.toString()) {
                ip_port.port = value.toString();
                SetProxySetting(settings, "addrSeparateProxyTor", ip_port);
                setRestartRequired(true);
            }
        }
        break;

#ifdef ENABLE_WALLET
        case SpendZeroConfChange:
            if (settings.value("bSpendZeroConfChange") != value) {
                settings.setValue("bSpendZeroConfChange", value);
                setRestartRequired(true);
            }
            break;
        case ExternalSignerPath:
            if (settings.value("external_signer_path") != value.toString()) {
                settings.setValue("external_signer_path", value.toString());
                setRestartRequired(true);
            }
            break;
        case SubFeeFromAmount:
            m_sub_fee_from_amount = value.toBool();
            settings.setValue("SubFeeFromAmount", m_sub_fee_from_amount);
            break;
        case ZeroBalanceAddressToken:
            bZeroBalanceAddressToken = value.toBool();
            settings.setValue("bZeroBalanceAddressToken", bZeroBalanceAddressToken);
            Q_EMIT zeroBalanceAddressTokenChanged(bZeroBalanceAddressToken);
            break;
        case SignPSBTWithHWITool:
            if (settings.value("signPSBTWithHWITool") != value) {
                settings.setValue("signPSBTWithHWITool", value);
                setRestartRequired(true);
            }
            break;
#endif
        case DisplayUnit:
            setDisplayUnit(value);
            break;
        case ThirdPartyTxUrls:
            if (strThirdPartyTxUrls != value.toString()) {
                strThirdPartyTxUrls = value.toString();
                settings.setValue("strThirdPartyTxUrls", strThirdPartyTxUrls);
                setRestartRequired(true);
            }
            break;
        case Language:
            if (settings.value("language") != value) {
                settings.setValue("language", value);
                setRestartRequired(true);
            }
            break;
        case UseEmbeddedMonospacedFont:
            m_use_embedded_monospaced_font = value.toBool();
            settings.setValue("UseEmbeddedMonospacedFont", m_use_embedded_monospaced_font);
            Q_EMIT useEmbeddedMonospacedFontChanged(m_use_embedded_monospaced_font);
            break;
        case CoinControlFeatures:
            fCoinControlFeatures = value.toBool();
            settings.setValue("fCoinControlFeatures", fCoinControlFeatures);
            Q_EMIT coinControlFeaturesChanged(fCoinControlFeatures);
            break;
        case EnablePSBTControls:
            m_enable_psbt_controls = value.toBool();
            settings.setValue("enable_psbt_controls", m_enable_psbt_controls);
            break;
        case Prune:
            if (settings.value("bPrune") != value) {
                settings.setValue("bPrune", value);
                setRestartRequired(true);
            }
            break;
        case PruneSize:
            if (settings.value("nPruneSize") != value) {
                settings.setValue("nPruneSize", value);
                setRestartRequired(true);
            }
            break;
        case DatabaseCache:
            if (settings.value("nDatabaseCache") != value) {
                settings.setValue("nDatabaseCache", value);
                setRestartRequired(true);
            }
            break;
        case LogEvents:
            if (settings.value("fLogEvents") != value) {
                settings.setValue("fLogEvents", value);
                setRestartRequired(true);
            }
            break;
#ifdef ENABLE_WALLET
        case SuperStaking:
            if (settings.value("fSuperStaking") != value) {
                settings.setValue("fSuperStaking", value);
                setRestartRequired(true);
            }
            break;
        case ReserveBalance:
            if (settings.value("nReserveBalance") != value) {
                settings.setValue("nReserveBalance", value);
                setRestartRequired(true);
            }
            break;
#endif
        case ThreadsScriptVerif:
            if (settings.value("nThreadsScriptVerif") != value) {
                settings.setValue("nThreadsScriptVerif", value);
                setRestartRequired(true);
            }
            break;
        case Listen:
            if (settings.value("fListen") != value) {
                settings.setValue("fListen", value);
                setRestartRequired(true);
            }
            break;
        case Server:
            if (settings.value("server") != value) {
                settings.setValue("server", value);
                setRestartRequired(true);
            }
            break;
#ifdef ENABLE_WALLET
        case UseChangeAddress:
            if (settings.value("fUseChangeAddress") != value) {
                settings.setValue("fUseChangeAddress", value);
                // Set fNotUseChangeAddress for backward compatibility reason
                settings.setValue("fNotUseChangeAddress", !value.toBool());
                setRestartRequired(true);
            }
            break;
#endif
        case CheckForUpdates:
            if (settings.value("fCheckForUpdates") != value) {
                settings.setValue("fCheckForUpdates", value);
                fCheckForUpdates = value.toBool();
            }
            break;
        case Theme:
            if (settings.value("Theme") != value) {
                settings.setValue("Theme", value);
                setRestartRequired(true);
            }
            break;
#ifdef ENABLE_WALLET
        case HWIToolPath:
            if (settings.value("HWIToolPath") != value) {
                settings.setValue("HWIToolPath", value);
                setRestartRequired(true);
            }
            break;
        case StakeLedgerId:
            if (settings.value("StakeLedgerId") != value) {
                settings.setValue("StakeLedgerId", value);
                setRestartRequired(true);
            }
            break;
#endif
        default:
            break;
        }
    }

    Q_EMIT dataChanged(index, index);

    return successful;
}

/** Updates current unit in memory, settings and emits displayUnitChanged(newUnit) signal */
void OptionsModel::setDisplayUnit(const QVariant &value)
{
    if (!value.isNull())
    {
        QSettings settings;
        nDisplayUnit = value.toInt();
        settings.setValue("nDisplayUnit", nDisplayUnit);
        Q_EMIT displayUnitChanged(nDisplayUnit);
    }
}

void OptionsModel::setRestartRequired(bool fRequired)
{
    QSettings settings;
    return settings.setValue("fRestartRequired", fRequired);
}

bool OptionsModel::isRestartRequired() const
{
    QSettings settings;
    return settings.value("fRestartRequired", false).toBool();
}

void OptionsModel::checkAndMigrate()
{
    // Migration of default values
    // Check if the QSettings container was already loaded with this client version
    QSettings settings;
    static const char strSettingsVersionKey[] = "nSettingsVersion";
    int settingsVersion = settings.contains(strSettingsVersionKey) ? settings.value(strSettingsVersionKey).toInt() : 0;
    if (settingsVersion < CLIENT_VERSION)
    {
        // -dbcache was bumped from 100 to 300 in 0.13
        // see https://github.com/bitcoin/bitcoin/pull/8273
        // force people to upgrade to the new value if they are using 100MB
        if (settingsVersion < 130000 && settings.contains("nDatabaseCache") && settings.value("nDatabaseCache").toLongLong() == 100)
            settings.setValue("nDatabaseCache", (qint64)nDefaultDbCache);

        settings.setValue(strSettingsVersionKey, CLIENT_VERSION);
    }

    // Overwrite the 'addrProxy' setting in case it has been set to an illegal
    // default value (see issue #12623; PR #12650).
    if (settings.contains("addrProxy") && settings.value("addrProxy").toString().endsWith("%2")) {
        settings.setValue("addrProxy", GetDefaultProxyAddress());
    }

    // Overwrite the 'addrSeparateProxyTor' setting in case it has been set to an illegal
    // default value (see issue #12623; PR #12650).
    if (settings.contains("addrSeparateProxyTor") && settings.value("addrSeparateProxyTor").toString().endsWith("%2")) {
        settings.setValue("addrSeparateProxyTor", GetDefaultProxyAddress());
    }
}

bool OptionsModel::getRestartApp() const
{
    return restartApp;
}

void OptionsModel::setRestartApp(bool value)
{
    restartApp = value;
}
