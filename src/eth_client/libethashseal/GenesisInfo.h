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

/// Set the improvements activation blocks for Qtum
struct EVMConsensus
{
    EVMConsensus() {}
    EVMConsensus(int nHeight) :
        QIP6Height(nHeight),
        QIP7Height(nHeight),
        nMuirGlacierHeight(nHeight),
        nLondonHeight(nHeight)
    {}

    int QIP6Height = 0x7fffffff;
    int QIP7Height = 0x7fffffff;
    int nMuirGlacierHeight = 0x7fffffff;
    int nLondonHeight = 0x7fffffff;
};

std::string genesisInfoQtum(Network _n, EVMConsensus _consensus);

}
}
