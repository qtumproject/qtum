// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include "CommonIO.h"
#include "FixedHash.h"
#include <string>
#include <vector>

namespace dev
{
/// For better formatting it is recommended to limit thread name to max 4 characters.
void setThreadName(std::string const& _n);

/// Set the current thread's log name.
std::string getThreadName();

enum class LogTag
{
    None,
    Url,
    Error,
    Special
};

class LogOutputStreamBase
{
public:
    LogOutputStreamBase(char const* _id, unsigned _v, bool _autospacing):
        m_autospacing(_autospacing),
        m_verbosity(_v),
        m_id(_id)
    {}

    void comment(std::string const& _t)
    {
        m_sstr << _t;
        m_logTag = LogTag::None;
    }

    void append(unsigned long _t) { m_sstr << _t; }
    void append(long _t) { m_sstr << _t; }
    void append(unsigned int _t) { m_sstr << _t; }
    void append(int _t) { m_sstr << _t; }
    void append(bigint const& _t) { m_sstr << _t; }
    void append(u256 const& _t) { m_sstr << _t; }
    void append(u160 const& _t) { m_sstr << _t; }
    void append(double _t) { m_sstr << _t; }
    template <unsigned N> void append(FixedHash<N> const& _t) { m_sstr << "#" << _t.abridged(); }
    void append(h160 const& _t) { m_sstr << "@" << _t.abridged(); }
    void append(h256 const& _t) { m_sstr << "#" << _t.abridged(); }
    void append(h512 const& _t) { m_sstr << "##" << _t.abridged(); }
    void append(std::string const& _t) { m_sstr << "\"" + _t + "\""; }
    void append(bytes const& _t) { m_sstr << "%" << toHex(_t); }
    void append(bytesConstRef _t) { m_sstr << "%" << toHex(_t); }
    template <class T> void append(std::vector<T> const& _t)
    {
        m_sstr << "[";
        int n = 0;
        for (auto const& i: _t)
        {
            m_sstr << (n++ ? ", " : "");
            append(i);
        }
        m_sstr << "]";
    }
    template <class T> void append(std::set<T> const& _t)
    {
        m_sstr << "{";
        int n = 0;
        for (auto const& i: _t)
        {
            m_sstr << (n++ ? ", " : "");
            append(i);
        }
        m_sstr << "}";
    }
    template <class T, class U> void append(std::map<T, U> const& _t)
    {
        m_sstr << "{";
        int n = 0;
        for (auto const& i: _t)
        {
            m_sstr << (n++ ? ", " : "");
            append(i.first);
            m_sstr << (n++ ? ": " : "");
            append(i.second);
        }
        m_sstr << "}";
    }
    template <class T> void append(std::unordered_set<T> const& _t)
    {
        m_sstr << "{";
        int n = 0;
        for (auto const& i: _t)
        {
            m_sstr << (n++ ? ", " : "");
            append(i);
        }
        m_sstr << "}";
    }
    template <class T, class U> void append(std::unordered_map<T, U> const& _t)
    {
        m_sstr << "{";
        int n = 0;
        for (auto const& i: _t)
        {
            m_sstr << (n++ ? ", " : "");
            append(i.first);
            m_sstr << (n++ ? ": " : "");
            append(i.second);
        }
        m_sstr << "}";
    }
    template <class T, class U> void append(std::pair<T, U> const& _t)
    {
        m_sstr << "(";
        append(_t.first);
        m_sstr << ", ";
        append(_t.second);
        m_sstr << ")";
    }
    template <class T> void append(T const& _t)
    {
        m_sstr << toString(_t);
    }

protected:
    bool m_autospacing = false;
    unsigned m_verbosity = 0;
    char const* m_id;
    std::stringstream m_sstr;	///< The accrued log entry.
    LogTag m_logTag = LogTag::None;
};

class Logger
{
public:
    Logger(int _verbosity, std::string _channel):
        verbosity(_verbosity),
        channel(_channel)
    {}

    const char* name(){return channel.c_str();}

    int verbosity;
    std::string channel;
};

/// A simple log-output function that prints log messages to stdout.
void simpleDebugOut(std::string const&, char const*);

/// The logging system's current verbosity.
extern int g_logVerbosity;

/// The current method that the logging system uses to output the log messages. Defaults to simpleDebugOut().
extern std::function<void(std::string const&, char const*)> g_logPost;

/// Is vm trace enabled
bool isVmTraceEnabled();

/// Logging class, iostream-like, that can be shifted to.
class LogOutputStream: LogOutputStreamBase
{
public:
    /// Construct a new object.
    LogOutputStream(Logger _Id):
        LogOutputStreamBase(_Id.name(), _Id.verbosity, true),
        Id(_Id),
        _AutoSpacing(true)
    {}

    /// Destructor. Posts the accrued log entry to the g_logPost function.
    ~LogOutputStream() { if (Id.verbosity <= g_logVerbosity) g_logPost(m_sstr.str(), Id.name()); }

    LogOutputStream& operator<<(std::string const& _t) { if (Id.verbosity <= g_logVerbosity) { if (_AutoSpacing && m_sstr.str().size() && m_sstr.str().back() != ' ') m_sstr << " "; comment(_t); } return *this; }

    LogOutputStream& operator<<(LogTag _t) { m_logTag = _t; return *this; }

    /// Shift arbitrary data to the log. Spaces will be added between items as required.
    template <class T> LogOutputStream& operator<<(T const& _t){ if (Id.verbosity <= g_logVerbosity) { if (_AutoSpacing && m_sstr.str().size() && m_sstr.str().back() != ' ') m_sstr << " "; append(_t); } return *this; }
private:
    Logger Id;
    bool _AutoSpacing;
};

#define LOG_STREAM(SEVERITY, CHANNEL) dev::LogOutputStream(Logger(SEVERITY, CHANNEL))
#define LOG(LOGGER) dev::LogOutputStream(LOGGER)

enum Verbosity
{
    VerbositySilent = -1,
    VerbosityError = 0,
    VerbosityWarning = 1,
    VerbosityInfo = 2,
    VerbosityDebug = 3,
    VerbosityTrace = 4,
};

// Simple macro to log to any channel a message without creating a logger object
// e.g. clog(VerbosityInfo, "channel") << "message";
#define clog(SEVERITY, CHANNEL) LOG(Logger(SEVERITY, CHANNEL))

// Simple cout-like stream objects for accessing common log channels.
extern Logger g_errorLogger;
extern Logger g_warnLogger;
extern Logger g_noteLogger;
extern Logger g_debugLogger;
extern Logger g_traceLogger;

#define cerror LOG(dev::g_errorLogger)
#define cwarn LOG(dev::g_warnLogger)
#define cnote LOG(dev::g_noteLogger)
#define cdebug LOG(dev::g_debugLogger)
#define ctrace LOG(dev::g_traceLogger)

// Simple non-thread-safe logger with fixed severity and channel for each message
// For better formatting it is recommended to limit channel name to max 6 characters.
inline Logger createLogger(int _severity, std::string const& _channel)
{
    return Logger(_severity, _channel);
}
}  // namespace dev

