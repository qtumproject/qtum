// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/data.h>

namespace benchmark {
namespace data {

#include <bench/data/blockbench.raw.h>
const std::vector<uint8_t> blockbench{blockbench_raw, blockbench_raw + sizeof(blockbench_raw) / sizeof(blockbench_raw[0])};

} // namespace data
} // namespace benchmark
