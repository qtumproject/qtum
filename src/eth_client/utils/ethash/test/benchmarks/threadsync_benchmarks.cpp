// Ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0. See the LICENSE file.

#include "threadsync_utils.hpp"

#include <benchmark/benchmark.h>


template<bool verify_fn(int, int)>
static void threadsync_fake_cache(benchmark::State& state)
{
    int id = 17;
    int value = 500;
    verify_fn(id, value);

    for (auto _ : state)
    {
        auto handle = verify_fn(id, value);
        benchmark::DoNotOptimize(handle);
    }
}
BENCHMARK_TEMPLATE(threadsync_fake_cache, verify_fake_cache_nosync)->ThreadRange(1, 8);
BENCHMARK_TEMPLATE(threadsync_fake_cache, verify_fake_cache_mutex)->ThreadRange(1, 8);
BENCHMARK_TEMPLATE(threadsync_fake_cache, verify_fake_cache_thread_local)->ThreadRange(1, 8);
