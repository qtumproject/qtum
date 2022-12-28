// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/data.h>

namespace benchmark {
namespace data {

#include <bench/data/blockbench.raw.h>
const std::vector<uint8_t> blockbench{std::begin(blockbench_raw), std::end(blockbench_raw)};

} // namespace data
} // namespace benchmark
