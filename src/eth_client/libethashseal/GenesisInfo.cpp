
// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
#include "GenesisInfo.h"
using namespace dev;

//Client configurations
#include "genesis/qtumMainNetwork.cpp"

//Test configurations
#include "genesis/test/qtumTestNetwork.cpp"

static dev::h256 const c_genesisDefaultStateRoot;

std::string const& dev::eth::genesisInfo(Network _n)
{
    switch (_n)
    {
    //Client genesis
    case Network::qtumMainNetwork: return c_genesisInfoQtumMainNetwork;

    //Test genesis
    case Network::qtumTestNetwork: return c_genesisInfoQtumTestNetwork;

    default:
        throw std::invalid_argument("Invalid network value");
    }
}

h256 const& dev::eth::genesisStateRoot(Network _n)
{
    switch (_n)
    {
    case Network::qtumMainNetwork: return c_genesisStateRootQtumMainNetwork;
    case Network::qtumTestNetwork: return c_genesisStateRootQtumTestNetwork;
    default:
        throw std::invalid_argument("Invalid network value");
    }
}
