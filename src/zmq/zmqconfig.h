// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ZMQ_ZMQCONFIG_H
#define BITCOIN_ZMQ_ZMQCONFIG_H

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include <stdarg.h>
#include <string>

#if ENABLE_ZMQ
#include <zmq.h>

static const std::string DEFAULT_ZMQPUBHASHBLOCK = "tcp://127.0.0.1:29000";
static const std::string DEFAULT_ZMQPUBRAWBLOCK = "tcp://127.0.0.1:29000";
static const std::string DEFAULT_ZMQPUBRAWTX = "tcp://127.0.0.1:29000";

#endif

#include "primitives/block.h"
#include "primitives/transaction.h"

void zmqError(const char *str);

#endif // BITCOIN_ZMQ_ZMQCONFIG_H
