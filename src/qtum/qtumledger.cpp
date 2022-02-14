#include <qtum/qtumledger.h>
#include <util/system.h>
#include <chainparams.h>
#include <univalue.h>
#include <util/strencodings.h>
#include <pubkey.h>
#include <logging.h>
#include <outputtype.h>
#if defined(WIN32) && !defined(__kernel_entry)
// A workaround for boost 1.71 incompatibility with mingw-w64 compiler.
// For details see https://github.com/bitcoin/bitcoin/pull/22348.
#define __kernel_entry
#endif
#include <boost/process.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#ifdef WIN32
#include <boost/process/windows.hpp>
#endif

RecursiveMutex cs_ledger;

namespace QtumLedger_NS {
// Read json document
UniValue json_read_doc(const std::string& jsondata)
{
    UniValue v;
    v.read(jsondata);
    return v;
}

// Read json object
UniValue json_get_object(const UniValue& jsondata)
{
    UniValue v(UniValue::VOBJ);
    if(jsondata.isObject())
        v = jsondata.get_obj();
    return v;
}

// Read json array
UniValue json_get_array(const UniValue& jsondata)
{
    UniValue v(UniValue::VARR);
    if(jsondata.isArray())
        v = jsondata.get_array();
    return v;
}

// Get json string for key
std::string json_get_key_string(const UniValue& jsondata, std::string key)
{
    UniValue v(UniValue::VSTR);
    if(jsondata.exists(key))
    {
        UniValue data = jsondata[key];
        if(data.isStr())
            v = data;
    }

    return v.get_str();
}

// Get json int for key
int json_get_key_int(const UniValue& jsondata, std::string key)
{
    UniValue v(0);
    if(jsondata.exists(key))
    {
        UniValue data = jsondata[key];
        if(data.isNum())
            v = data;
    }

    return v.get_int();
}

// Append data to vector
std::vector<std::string>& operator<<(std::vector<std::string>& os, const std::string& dt)
{
    os.push_back(dt);
    return os;
}

#ifdef WIN32
// Check suffix
bool endsWith(const std::string& str, const std::string& suffix)
{
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

// Check if the path is to python executable
bool isPyPath(const std::string& str)
{
    std::size_t found = str.find("python");
    return found!=std::string::npos;
}
#endif

// Start process from qtumd
class CProcess
{
public:
    ~CProcess()
    {
        clean();
    }

    // Set process params
    void start(const std::string& prog, const std::vector<std::string> &arg)
    {
        clean();
        m_program = prog;
        m_arguments = arg;
    }

    // Start and wait for it to finish
    void waitForFinished()
    {
        try
        {
            boost::asio::io_service svc;
            boost::asio::streambuf out, err;
    #ifdef WIN32
            boost::process::child child(m_program, ::boost::process::windows::create_no_window, boost::process::args(m_arguments),
                                        boost::process::std_out > out, boost::process::std_err > err, svc);
    #else
            boost::process::child child(m_program, boost::process::args(m_arguments),
                                        boost::process::std_out > out, boost::process::std_err > err, svc);
    #endif

            svc.run();
            child.wait();
            m_std_out = toString(&out);
            m_std_err = toString(&err);
        }
        catch(...)
        {
            m_std_err = "Fail to create process for: " + m_program;
        }
    }

    // Read all standard output
    std::string readAllStandardOutput()
    {
        return m_std_out;
    }

    // Read all standard error
    std::string readAllStandardError()
    {
        return m_std_err;
    }

    // Clean process
    void clean()
    {
        m_program = "";
        m_std_out = "";
        m_std_err = "";
    }


private:
    std::string toString(boost::asio::streambuf* stream)
    {
        std::istream is(stream);
        std::ostringstream os;
        is >> os.rdbuf();
        return os.str();
    }

private:
    std::string m_program;
    std::vector<std::string> m_arguments;
    std::string m_std_out;
    std::string m_std_err;
};
}
using namespace QtumLedger_NS;

class QtumLedgerPriv
{
public:
    QtumLedgerPriv()
    {
        toolPath = gArgs.GetArg("-hwitoolpath", "");
        toolExists = boost::filesystem::exists(toolPath);
        initToolPath();

        if(gArgs.GetChainName() != CBaseChainParams::MAIN)
        {
            ledgerMainPath = false;
        }

        arguments << "--chain" << gArgs.GetChainName();

        if(!toolExists)
        {
            LogPrintf("QtumLedger(): HWI tool not found %s\n", toolPath);
        }
    }

#ifdef WIN32
    bool getToolPath(std::string pythonProgram)
    {
        toolPath = boost::process::search_path(pythonProgram).string();
        toolExists &= isPyPath(toolPath);
        if(!toolExists)
        {
            std::string prog = boost::process::search_path("cmd").string();
            std::vector<std::string> arg;
            arg << "/c" << pythonProgram << "-c" << "import sys; print(sys.executable)";
            process.start(prog, arg);
            process.waitForFinished();
            toolPath = process.readAllStandardOutput();
            boost::erase_all(toolPath, "\r");
            boost::erase_all(toolPath, "\n");
            toolExists = isPyPath(toolPath);
            process.clean();
        }
        return toolExists;
    }
#endif

    void initToolPath()
    {
#ifdef WIN32
        if(endsWith(toolPath, ".py") ||
                endsWith(toolPath, ".PY") ||
                endsWith(toolPath, ".Py") ||
                endsWith(toolPath, ".pY"))
        {
            arguments << toolPath;
            if(!getToolPath("python3"))
                getToolPath("python");
        }
#endif
    }

    std::atomic<bool> fStarted{false};
    CProcess process;
    std::string strStdout;
    std::string strError;
    std::string toolPath;
    std::vector<std::string> arguments;
    bool toolExists = false;
    bool ledgerMainPath = true;
};

QtumLedger::QtumLedger():
    d(0)
{
    d = new QtumLedgerPriv();
}

QtumLedger::~QtumLedger()
{
    if(d)
        delete d;
    d = 0;
}

QtumLedger &QtumLedger::instance()
{
    static QtumLedger device;
    return device;
}

bool QtumLedger::signCoinStake(const std::string &fingerprint, std::string &psbt)
{
    LOCK(cs_ledger);
    // Check if tool exists
    if(!toolExists())
        return false;

    // Sign PSBT transaction
    if(isStarted())
        return false;

    if(!beginSignTx(fingerprint, psbt))
        return false;

    wait();

    return endSignTx(fingerprint, psbt);
}

bool QtumLedger::signBlockHeader(const std::string &fingerprint, const std::string &header, const std::string &path, std::vector<unsigned char> &vchSig)
{
    LOCK(cs_ledger);
    // Check if tool exists
    if(!toolExists())
        return false;

    // Sign block header
    if(isStarted())
        return false;

    if(!beginSignBlockHeader(fingerprint, header, path, vchSig))
        return false;

    wait();

    return endSignBlockHeader(fingerprint, header, path, vchSig);
}

bool QtumLedger::isConnected(const std::string &fingerprint, bool stake)
{
    // Check if a device is connected
    std::vector<LedgerDevice> devices;
    if(enumerate(devices, stake))
    {
        for(LedgerDevice device: devices)
        {
            if(device.fingerprint == fingerprint)
                return true;
        }
    }

    return false;
}

bool QtumLedger::enumerate(std::vector<LedgerDevice> &devices, bool stake)
{
    LOCK(cs_ledger);
    // Check if tool exists
    if(!toolExists())
        return false;

    // Enumerate hardware wallet devices
    if(isStarted())
        return false;

    if(!beginEnumerate(devices))
        return false;

    wait();

    return endEnumerate(devices, stake);
}

bool QtumLedger::signTx(const std::string &fingerprint, std::string &psbt)
{
    LOCK(cs_ledger);
    // Check if tool exists
    if(!toolExists())
        return false;

    // Sign PSBT transaction
    if(isStarted())
        return false;

    if(!beginSignTx(fingerprint, psbt))
        return false;

    wait();

    return endSignTx(fingerprint, psbt);
}

bool QtumLedger::signMessage(const std::string &fingerprint, const std::string &message, const std::string &path, std::string &signature)
{
    LOCK(cs_ledger);
    // Check if tool exists
    if(!toolExists())
        return false;

    // Sign message
    if(isStarted())
        return false;

    if(!beginSignMessage(fingerprint, message, path, signature))
        return false;

    wait();

    return endSignMessage(fingerprint, message, path, signature);
}

bool QtumLedger::getKeyPool(const std::string &fingerprint, int type, const std::string& path, bool internal, int from, int to, std::string &desc)
{
    LOCK(cs_ledger);
    // Check if tool exists
    if(!toolExists())
        return false;

    // Get the key pool for a device
    if(isStarted())
        return false;

    if(!beginGetKeyPool(fingerprint, type, path, internal, from, to, desc))
        return false;

    wait();

    return endGetKeyPool(fingerprint, type, path, internal, from, to, desc);
}

std::string QtumLedger::errorMessage()
{
    LOCK(cs_ledger);
    if(d->strError.empty() == false)
        return d->strError;
    if(d->strStdout.empty() == false)
    {
        try
        {
            UniValue jsonDocument = json_read_doc(d->strStdout);
            UniValue data = json_get_object(jsonDocument);
            std::string error = json_get_key_string(data, "error");
            int code = json_get_key_int(data, "code");
            if(data.exists("error") || data.exists("code"))
                return strprintf("Error: %s, Code: %d", error, code);
        }
        catch(...)
        {}
    }
    return "unknown error";
}

bool QtumLedger::toolExists()
{
    return d->toolExists;
}

bool QtumLedger::isStarted()
{
    return d->fStarted;
}

void QtumLedger::wait()
{
    if(d->fStarted)
    {
        d->process.waitForFinished();
        d->strStdout = d->process.readAllStandardOutput();
        d->strError = d->process.readAllStandardError();
        d->fStarted = false;
    }
}

bool QtumLedger::beginSignTx(const std::string &fingerprint, std::string &psbt)
{
    // Execute command line
    std::vector<std::string> arguments = d->arguments;
    arguments << "-f" << fingerprint << "signtx" << psbt;
    d->process.start(d->toolPath, arguments);
    d->fStarted = true;

    return d->fStarted;
}

bool QtumLedger::endSignTx(const std::string &, std::string &psbt)
{
    // Decode command line results
    UniValue jsonDocument = json_read_doc(d->strStdout);
    UniValue data = json_get_object(jsonDocument);
    std::string psbtSigned = json_get_key_string(data, "psbt");
    if(!psbtSigned.empty())
    {
        psbt = psbtSigned;
        return true;
    }

    return false;
}

bool QtumLedger::beginSignBlockHeader(const std::string &fingerprint, const std::string &header, const std::string &path, std::vector<unsigned char> &)
{
    // Execute command line
    std::vector<std::string> arguments = d->arguments;
    arguments << "-f" << fingerprint << "signheader" << header << path;
    d->process.start(d->toolPath, arguments);
    d->fStarted = true;

    return d->fStarted;
}

bool QtumLedger::endSignBlockHeader(const std::string &, const std::string &, const std::string &, std::vector<unsigned char> &vchSig)
{
    // Decode command line results
    UniValue jsonDocument = json_read_doc(d->strStdout);
    UniValue data = json_get_object(jsonDocument);
    std::string headerSigned = json_get_key_string(data, "signature");
    if(!headerSigned.empty())
    {
        vchSig = DecodeBase64(headerSigned.c_str());
        return vchSig.size() == CPubKey::COMPACT_SIGNATURE_SIZE;
    }

    return false;
}

bool QtumLedger::beginEnumerate(std::vector<LedgerDevice> &)
{
    // Execute command line
    std::vector<std::string> arguments = d->arguments;
    arguments << "enumerate";
    d->process.start(d->toolPath, arguments);
    d->fStarted = true;

    return d->fStarted;
}

bool QtumLedger::endEnumerate(std::vector<LedgerDevice> &devices, bool stake)
{
    // Decode command line results
    UniValue jsonDocument = json_read_doc(d->strStdout);
    UniValue jsonDevices = json_get_array(jsonDocument);
    for(size_t i = 0; i < jsonDevices.size(); i++)
    {
        const UniValue& jsonDevice = jsonDevices[i];
        if(!jsonDevice.isObject())
            return false;

        // Get device info
        UniValue data = json_get_object(jsonDevice);
        LedgerDevice device;
        device.fingerprint = json_get_key_string(data, "fingerprint");
        device.serial_number = json_get_key_string(data, "serial_number");
        device.type = json_get_key_string(data, "type");
        device.path = json_get_key_string(data, "path");
        device.error = json_get_key_string(data, "error");
        device.model = json_get_key_string(data, "model");
        device.code = json_get_key_string(data, "code");
        device.app_name = json_get_key_string(data, "app_name");
        bool isStakeApp = device.app_name == "Qtum Stake" || device.app_name == "Qtum Stake Test";
        if(isStakeApp == stake)
        {
            devices.push_back(device);
        }
    }

    return devices.size() > 0;
}

bool QtumLedger::beginSignMessage(const std::string &fingerprint, const std::string &message, const std::string &path, std::string &)
{
    // Execute command line
    std::vector<std::string> arguments = d->arguments;
    arguments << "-f" << fingerprint << "signmessage" << message << path;
    d->process.start(d->toolPath, arguments);
    d->fStarted = true;

    return d->fStarted;
}

bool QtumLedger::endSignMessage(const std::string &, const std::string &, const std::string &, std::string &signature)
{
    // Decode command line results
    UniValue jsonDocument = json_read_doc(d->strStdout);
    UniValue data = json_get_object(jsonDocument);
    std::string msgSigned = json_get_key_string(data, "signature");
    if(!msgSigned.empty())
    {
        signature = msgSigned;
        return true;
    }

    return false;
}

bool QtumLedger::beginGetKeyPool(const std::string &fingerprint, int type, const std::string& path, bool internal, int from, int to, std::string &)
{
    // Get the output type
    std::string descType;
    switch (type) {
    case (int)OutputType::P2SH_SEGWIT:
        descType = "sh_wit";
        break;
    case (int)OutputType::BECH32:
        descType = "wit";
        break;
    case (int)OutputType::LEGACY:
        descType = "legacy";
        break;
    default:
        break;
    }

    // Execute command line
    std::vector<std::string> arguments = d->arguments;
    arguments << "-f" << fingerprint << "getkeypool";
    if(descType != "")
        arguments << "--addr-type" << descType;
    if(path != "")
    {
        arguments << "--path" << path;
        if(internal)
        {
            arguments << "--internal";
        }
    }
    arguments << std::to_string(from) << std::to_string(to);
    d->process.start(d->toolPath, arguments);
    d->fStarted = true;

    return d->fStarted;
}

bool QtumLedger::endGetKeyPool(const std::string &, int, const std::string& , bool, int, int, std::string &desc)
{
    // Decode command line results
    bool ret = d->strStdout.find("desc")!=std::string::npos;
    desc = d->strStdout;
    return ret;
}


std::string QtumLedger::derivationPath(int type)
{
    std::string derivPath;
    if(d->ledgerMainPath)
    {
        switch (type) {
        case (int)OutputType::P2SH_SEGWIT:
            derivPath = "m/49'/88'/0'";
            break;
        case (int)OutputType::BECH32:
            derivPath = "m/84'/88'/0'";
            break;
        case (int)OutputType::LEGACY:
            derivPath = "m/44'/88'/0'";
            break;
        default:
            break;
        }
    }
    else
    {
        switch (type) {
        case (int)OutputType::P2SH_SEGWIT:
            derivPath = "m/49'/1'/0'";
            break;
        case (int)OutputType::BECH32:
            derivPath = "m/84'/1'/0'";
            break;
        case (int)OutputType::LEGACY:
            derivPath = "m/44'/1'/0'";
            break;
        default:
            break;
        }
    }

    return derivPath;
}
