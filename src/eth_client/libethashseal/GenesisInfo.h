// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include <string>
#include <libdevcore/FixedHash.h>
#include <libethcore/Common.h>

namespace dev
{
namespace eth
{

/// The network id.
enum class Network
{
    qtumMainNetwork = 1,
    qtumTestNetwork = 2
};

std::string const& genesisInfo(Network _n);
h256 const& genesisStateRoot(Network _n);

}
}
