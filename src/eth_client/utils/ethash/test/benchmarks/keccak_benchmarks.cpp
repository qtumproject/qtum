// Ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// SPDX-License-Identifier: Apache-2.0

#include "keccak_utils.hpp"
#include <benchmark/benchmark.h>
#include <ethash/keccak.h>


void fake_keccakf1600(uint64_t* state) noexcept  // NOLINT(readability-non-const-parameter)
{
    // Do nothing to measure performance of the code outside the keccakf function.
    (void)state;
}


static void keccak256(benchmark::State& state)
{
    const auto data_size = static_cast<size_t>(state.range(0));
    std::vector<uint8_t> data(data_size, 0xde);

    for (auto _ : state)
    {
        auto h = ethash_keccak256(data.data(), data.size());
        benchmark::DoNotOptimize(h.bytes);
    }
}
BENCHMARK(keccak256)->Arg(0)->Arg(32)->Arg(64)->Arg(135)->Arg(136);


static void keccak512(benchmark::State& state)
{
    const auto data_size = static_cast<size_t>(state.range(0));
    std::vector<uint8_t> data(data_size, 0xde);

    for (auto _ : state)
    {
        auto h = ethash_keccak512(data.data(), data.size());
        benchmark::DoNotOptimize(h.bytes);
    }
}
BENCHMARK(keccak512)->Arg(0)->Arg(32)->Arg(64)->Arg(71)->Arg(143)->Arg(144);


#define FAKE_KECCAK_ARGS ->Arg(128)->Arg(17 * 8)->Arg(4096)->Arg(16 * 1024)

template <void keccak_fn(uint64_t*, const uint8_t*, size_t)>
static void fake_keccak256(benchmark::State& state)
{
    std::vector<uint8_t> data(static_cast<size_t>(state.range(0)), 0xaa);

    for (auto _ : state)
    {
        uint64_t out[4];
        keccak_fn(out, data.data(), data.size());
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK_TEMPLATE(fake_keccak256, fake_keccak256_default_aligned) FAKE_KECCAK_ARGS;
BENCHMARK_TEMPLATE(fake_keccak256, fake_keccak256_default) FAKE_KECCAK_ARGS;
BENCHMARK_TEMPLATE(fake_keccak256, fake_keccak256_fastest) FAKE_KECCAK_ARGS;


template <void keccak_fn(uint64_t*, const uint8_t*, size_t)>
static void fake_keccak256_unaligned(benchmark::State& state)
{
    const auto size = static_cast<size_t>(state.range(0));
    std::vector<uint8_t> data(size + 1, 0xaa);

    for (auto _ : state)
    {
        uint64_t out[4];
        keccak_fn(out, data.data() + 1, size);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK_TEMPLATE(fake_keccak256_unaligned, fake_keccak256_default) FAKE_KECCAK_ARGS;
BENCHMARK_TEMPLATE(fake_keccak256_unaligned, fake_keccak256_fastest) FAKE_KECCAK_ARGS;


static void fake_keccak256_word4(benchmark::State& state)
{
    const uint64_t input[4] = {1, 2, 3, 4};

    for (auto _ : state)
    {
        uint64_t out[4];
        fake_keccak256_fastest_word4(out, input);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(fake_keccak256_word4);
