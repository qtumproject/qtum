// Ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0. See the LICENSE file.

#include "../unittests/helpers.hpp"

#include <ethash/progpow.hpp>

#include <benchmark/benchmark.h>

static void progpow_hash(benchmark::State& state)
{
    // Get block number in millions.
    int block_number = static_cast<int>(state.range(0)) * 1000000;
    uint64_t nonce = 1;

    const auto& ctx = ethash::get_global_epoch_context(ethash::get_epoch_number(block_number));

    for (auto _ : state)
        progpow::hash(ctx, block_number++, {}, nonce++);
}
BENCHMARK(progpow_hash)->Unit(benchmark::kMicrosecond)->Arg(0)->Arg(10);
