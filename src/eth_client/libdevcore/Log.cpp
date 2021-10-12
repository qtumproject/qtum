// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2013-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
#include "Log.h"

#ifdef __APPLE__
#include <pthread.h>
#endif

namespace dev
{

int g_logVerbosity = 5;
Logger g_errorLogger(VerbosityError, "error");
Logger g_warnLogger(VerbosityWarning, "warn");
Logger g_noteLogger(VerbosityInfo, "info");
Logger g_debugLogger(VerbosityDebug, "debug");
Logger g_traceLogger(VerbosityTrace, "trace");

void setThreadName(std::string const&)
{}

std::string getThreadName()
{
    return "";
}

void simpleDebugOut(std::string const& _s, char const*)
{
    std::cerr << _s << std::endl << std::flush;
}

std::function<void(std::string const&, char const*)> g_logPost = simpleDebugOut;

bool isVmTraceEnabled()
{
    return VerbosityTrace < g_logVerbosity;
}

}  // namespace

