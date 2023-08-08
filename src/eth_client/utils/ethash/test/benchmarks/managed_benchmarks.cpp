// ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0. See the LICENSE file.

#include <ethash/ethash.hpp>

#include <benchmark/benchmark.h>

static void get_epoch_context(benchmark::State& state)
{
    const auto e = static_cast<int>(state.range(0));

    ethash::get_global_epoch_context(0);

    for (auto _ : state)
    {
        auto& ctx = ethash::get_global_epoch_context(e);
        benchmark::DoNotOptimize(&ctx);
    }
}
BENCHMARK(get_epoch_context)->Arg(0)->ThreadRange(1, 8);
