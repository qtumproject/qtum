// ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

/// @file
/// Contains declarations of internal ethash functions to allow them to be
/// unit-tested.

#pragma once

#include "endianness.hpp"
#include <ethash/ethash.hpp>


extern "C" struct ethash_epoch_context_full : ethash_epoch_context
{
    ethash_hash1024* full_dataset;

    constexpr ethash_epoch_context_full(int epoch, int light_num_items, const ethash_hash512* light,
        int dataset_num_items, ethash_hash1024* dataset) noexcept
      : ethash_epoch_context{epoch, light_num_items, light, dataset_num_items},
        full_dataset{dataset}
    {}
};

namespace ethash
{
/// Returns true if a <= b in byte-wise comparison (i.e. as big-endian numbers).
inline bool less_equal(const hash256& a, const hash256& b) noexcept
{
    for (size_t i = 0; i < (sizeof(a) / sizeof(a.word64s[0])); ++i)
    {
        if (be::uint64(a.word64s[i]) > be::uint64(b.word64s[i]))
            return false;
        if (be::uint64(a.word64s[i]) < be::uint64(b.word64s[i]))
            return true;
    }
    return true;
}

/// Returns true if a == b in byte-wise comparison.
inline bool equal(const hash256& a, const hash256& b) noexcept
{
    return (a.word64s[0] == b.word64s[0]) & (a.word64s[1] == b.word64s[1]) &
           (a.word64s[2] == b.word64s[2]) & (a.word64s[3] == b.word64s[3]);
}

bool check_against_difficulty(const hash256& final_hash, const hash256& difficulty) noexcept;

hash1024 calculate_dataset_item_1024(const epoch_context& context, uint32_t index) noexcept;

}  // namespace ethash
