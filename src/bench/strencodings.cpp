// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data/blockbench.raw.h>
#include <span.h>
#include <util/strencodings.h>

#include <vector>

static void HexStrBench(benchmark::Bench& bench)
{
    auto const& data = benchmark::data::blockbench;
    bench.batch(data.size()).unit("byte").run([&] {
        auto hex = HexStr(data);
        ankerl::nanobench::doNotOptimizeAway(hex);
    });
}

BENCHMARK(HexStrBench);
