// ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0. See the LICENSE file.

#include "helpers.hpp"
#include "test_cases.hpp"

#include <ethash/ethash-internal.hpp>

#include <gtest/gtest.h>

#include <array>
#include <future>

using namespace ethash;

TEST(managed_multithreaded, hash_all)
{
    constexpr size_t num_treads = 8;

    std::array<std::future<void>, num_treads> futures;
    for (auto& f : futures)
    {
        f = std::async(std::launch::async, [] {
            for (const auto& t : hash_test_cases)
            {
                const hash256 header_hash = to_hash256(t.header_hash_hex);
                const uint64_t nonce = std::stoull(t.nonce_hex, nullptr, 16);
                const int epoch_number = get_epoch_number(t.block_number);
                auto& context = get_global_epoch_context(epoch_number);
                const result res = hash(context, header_hash, nonce);
                EXPECT_EQ(to_hex(res.mix_hash), t.mix_hash_hex);
                EXPECT_EQ(to_hex(res.final_hash), t.final_hash_hex);
            }
        });
    }
    for (auto& f : futures)
        f.wait();
}

TEST(managed_multithreaded, hash_parallel)
{
    std::vector<std::future<bool>> futures;

    for (const auto& t : hash_test_cases)
    {
        futures.emplace_back(std::async(std::launch::async, [&t] {
            const hash256 header_hash = to_hash256(t.header_hash_hex);
            const uint64_t nonce = std::stoull(t.nonce_hex, nullptr, 16);
            const int epoch_number = get_epoch_number(t.block_number);
            auto& context = get_global_epoch_context(epoch_number);
            const result res = hash(context, header_hash, nonce);
            return (to_hex(res.mix_hash) == t.mix_hash_hex) &&
                   (to_hex(res.final_hash) == t.final_hash_hex);
        }));
    }

    for (auto& f : futures)
        EXPECT_TRUE(f.get());
}

TEST(managed_multithreaded, verify_all)
{
    constexpr size_t num_treads = 8;

    std::array<std::future<void>, num_treads> futures;
    for (auto& f : futures)
    {
        f = std::async(std::launch::async, [] {
            for (const auto& t : hash_test_cases)
            {
                const hash256 header_hash = to_hash256(t.header_hash_hex);
                const hash256 mix_hash = to_hash256(t.mix_hash_hex);
                const hash256 final_hash = to_hash256(t.final_hash_hex);
                const uint64_t nonce = std::stoull(t.nonce_hex, nullptr, 16);
                const hash256 boundary = final_hash;
                const int epoch_number = get_epoch_number(t.block_number);
                auto& context = get_global_epoch_context(epoch_number);
                const bool valid = verify(context, header_hash, mix_hash, nonce, boundary);
                EXPECT_TRUE(valid);
            }
        });
    }
    for (auto& f : futures)
        f.wait();
}

TEST(managed_multithreaded, verify_parallel)
{
    std::vector<std::future<bool>> futures;

    for (const auto& t : hash_test_cases)
    {
        futures.emplace_back(std::async(std::launch::async, [&t] {
            const hash256 header_hash = to_hash256(t.header_hash_hex);
            const hash256 mix_hash = to_hash256(t.mix_hash_hex);
            const hash256 final_hash = to_hash256(t.final_hash_hex);
            const uint64_t nonce = std::stoull(t.nonce_hex, nullptr, 16);
            const hash256 boundary = final_hash;
            const int epoch_number = get_epoch_number(t.block_number);
            auto& context = get_global_epoch_context(epoch_number);
            return verify(context, header_hash, mix_hash, nonce, boundary);
        }));
    }

    for (auto& f : futures)
        EXPECT_TRUE(f.get());
}

TEST(managed_multithreaded, get_epoch_context_random)
{
    static constexpr int num_threads = 4;

    std::vector<std::future<int>> futures;

    for (int i = 0; i < num_threads; ++i)
    {
        futures.emplace_back(std::async(std::launch::async, [] {
            int sum = 0;
            for (int j = 0; j < 10; ++j)
            {
                // Epoch number sequence: 0, 0, 1, 1, 0, 0, 1, 1, ...
                int epoch_number = (j / 2) % 2;
                get_global_epoch_context(epoch_number);
                sum += epoch_number;

                // Add sleep, otherwise this thread will starve others by quickly
                // reacquiring the mutex in get_epoch_number().
                std::this_thread::sleep_for(std::chrono::nanoseconds(1));
            }
            return sum;
        }));
    }

    for (auto& f : futures)
        EXPECT_GT(f.get(), 0);
}

TEST(managed_multithreaded, get_epoch_context_full)
{
    static constexpr int num_threads = 4;

    std::vector<std::future<bool>> futures;

    for (int i = 0; i < num_threads; ++i)
    {
        futures.emplace_back(std::async(std::launch::async, [] {
            hash1024* full_dataset1 = get_global_epoch_context_full(7).full_dataset;
            hash1024* full_dataset2 = get_global_epoch_context_full(7).full_dataset;
            return (full_dataset1 == full_dataset2) && (full_dataset1 != nullptr);
        }));
    }

    for (auto& f : futures)
        EXPECT_TRUE(f.get());
}
