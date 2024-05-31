// ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0. See the LICENSE file.

#include "ethash-internal.hpp"

#include "bit_manipulation.h"
#include "endianness.hpp"
#include "primes.h"
#include "support/attributes.h"
#include <ethash/keccak.hpp>
#include <ethash/progpow.hpp>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace ethash
{
// Internal constants:
constexpr static int light_cache_init_size = 1 << 24;
constexpr static int light_cache_growth = 1 << 17;
constexpr static int light_cache_rounds = 3;
constexpr static int full_dataset_init_size = 1 << 30;
constexpr static int full_dataset_growth = 1 << 23;
constexpr static int full_dataset_item_parents = 256;

// Verify constants:
static_assert(sizeof(hash512) == ETHASH_LIGHT_CACHE_ITEM_SIZE, "");
static_assert(sizeof(hash1024) == ETHASH_FULL_DATASET_ITEM_SIZE, "");
static_assert(light_cache_item_size == ETHASH_LIGHT_CACHE_ITEM_SIZE, "");
static_assert(full_dataset_item_size == ETHASH_FULL_DATASET_ITEM_SIZE, "");


namespace
{
using ::fnv1;

inline hash512 fnv1(const hash512& u, const hash512& v) noexcept
{
    hash512 r;
    for (size_t i = 0; i < sizeof(r) / sizeof(r.word32s[0]); ++i)
        r.word32s[i] = fnv1(u.word32s[i], v.word32s[i]);
    return r;
}

inline hash512 bitwise_xor(const hash512& x, const hash512& y) noexcept
{
    hash512 z;
    for (size_t i = 0; i < sizeof(z) / sizeof(z.word64s[0]); ++i)
        z.word64s[i] = x.word64s[i] ^ y.word64s[i];
    return z;
}
}  // namespace

int find_epoch_number(const hash256& seed) noexcept
{
    static constexpr int num_tries = 30000;  // Divisible by 16.

    // Thread-local cache of the last search.
    static thread_local int cached_epoch_number = 0;
    static thread_local hash256 cached_seed = {};

    // Load from memory once (memory will be clobbered by keccak256()).
    const uint32_t seed_part = seed.word32s[0];
    const int e = cached_epoch_number;
    hash256 s = cached_seed;

    if (s.word32s[0] == seed_part)
        return e;

    // Try the next seed, will match for sequential epoch access.
    s = keccak256(s);
    if (s.word32s[0] == seed_part)
    {
        cached_seed = s;
        cached_epoch_number = e + 1;
        return e + 1;
    }

    // Search for matching seed starting from epoch 0.
    s = {};
    for (int i = 0; i < num_tries; ++i)
    {
        if (s.word32s[0] == seed_part)
        {
            cached_seed = s;
            cached_epoch_number = i;
            return i;
        }

        s = keccak256(s);
    }

    return -1;
}

namespace generic
{
void build_light_cache(
    hash_fn_512 hash_fn, hash512 cache[], int num_items, const hash256& seed) noexcept
{
    hash512 item = hash_fn(seed.bytes, sizeof(seed));
    cache[0] = item;
    for (int i = 1; i < num_items; ++i)
    {
        item = hash_fn(item.bytes, sizeof(item));
        cache[i] = item;
    }

    for (int q = 0; q < light_cache_rounds; ++q)
    {
        for (int i = 0; i < num_items; ++i)
        {
            const uint32_t index_limit = static_cast<uint32_t>(num_items);

            // Fist index: 4 first bytes of the item as little-endian integer.
            const uint32_t t = le::uint32(cache[i].word32s[0]);
            const uint32_t v = t % index_limit;

            // Second index.
            const uint32_t w = static_cast<uint32_t>(num_items + (i - 1)) % index_limit;

            const hash512 x = bitwise_xor(cache[v], cache[w]);
            cache[i] = hash_fn(x.bytes, sizeof(x));
        }
    }
}

epoch_context_full* create_epoch_context(
    build_light_cache_fn build_fn, int epoch_number, bool full) noexcept
{
    static_assert(sizeof(epoch_context_full) < sizeof(hash512), "epoch_context too big");
    static constexpr size_t context_alloc_size = sizeof(hash512);

    const int light_cache_num_items = calculate_light_cache_num_items(epoch_number);
    const int full_dataset_num_items = calculate_full_dataset_num_items(epoch_number);
    const size_t light_cache_size = get_light_cache_size(light_cache_num_items);
    const size_t full_dataset_size =
        full ? static_cast<size_t>(full_dataset_num_items) * sizeof(hash1024) :
               progpow::l1_cache_size;

    const size_t alloc_size = context_alloc_size + light_cache_size + full_dataset_size;

    char* const alloc_data = static_cast<char*>(std::calloc(1, alloc_size));
    if (!alloc_data)
        return nullptr;  // Signal out-of-memory by returning null pointer.

    hash512* const light_cache = reinterpret_cast<hash512*>(alloc_data + context_alloc_size);
    const hash256 epoch_seed = calculate_epoch_seed(epoch_number);
    build_fn(light_cache, light_cache_num_items, epoch_seed);

    uint32_t* const l1_cache =
        reinterpret_cast<uint32_t*>(alloc_data + context_alloc_size + light_cache_size);

    hash1024* full_dataset = full ? reinterpret_cast<hash1024*>(l1_cache) : nullptr;

    epoch_context_full* const context = new (alloc_data) epoch_context_full{
        epoch_number,
        light_cache_num_items,
        light_cache,
        l1_cache,
        full_dataset_num_items,
        full_dataset,
    };

    auto* full_dataset_2048 = reinterpret_cast<hash2048*>(l1_cache);
    for (uint32_t i = 0; i < progpow::l1_cache_size / sizeof(full_dataset_2048[0]); ++i)
        full_dataset_2048[i] = calculate_dataset_item_2048(*context, i);
    return context;
}
}  // namespace generic

void build_light_cache(hash512 cache[], int num_items, const hash256& seed) noexcept
{
    return generic::build_light_cache(keccak512, cache, num_items, seed);
}

struct item_state
{
    const hash512* const cache;
    const int64_t num_cache_items;
    const uint32_t seed;

    hash512 mix;

    ALWAYS_INLINE item_state(const epoch_context& context, int64_t index) noexcept
      : cache{context.light_cache},
        num_cache_items{context.light_cache_num_items},
        seed{static_cast<uint32_t>(index)}
    {
        mix = cache[index % num_cache_items];
        mix.word32s[0] ^= le::uint32(seed);
        mix = le::uint32s(keccak512(mix));
    }

    ALWAYS_INLINE void update(uint32_t round) noexcept
    {
        static constexpr size_t num_words = sizeof(mix) / sizeof(uint32_t);
        const uint32_t t = fnv1(seed ^ round, mix.word32s[round % num_words]);
        const int64_t parent_index = t % num_cache_items;
        mix = fnv1(mix, le::uint32s(cache[parent_index]));
    }

    ALWAYS_INLINE hash512 final() noexcept { return keccak512(le::uint32s(mix)); }
};

hash512 calculate_dataset_item_512(const epoch_context& context, int64_t index) noexcept
{
    item_state item0{context, index};
    for (uint32_t j = 0; j < full_dataset_item_parents; ++j)
        item0.update(j);
    return item0.final();
}

/// Calculates a full dataset item
///
/// This consist of two 512-bit items produced by calculate_dataset_item_partial().
/// Here the computation is done interleaved for better performance.
hash1024 calculate_dataset_item_1024(const epoch_context& context, uint32_t index) noexcept
{
    item_state item0{context, int64_t(index) * 2};
    item_state item1{context, int64_t(index) * 2 + 1};

    for (uint32_t j = 0; j < full_dataset_item_parents; ++j)
    {
        item0.update(j);
        item1.update(j);
    }

    return hash1024{{item0.final(), item1.final()}};
}

hash2048 calculate_dataset_item_2048(const epoch_context& context, uint32_t index) noexcept
{
    item_state item0{context, int64_t(index) * 4};
    item_state item1{context, int64_t(index) * 4 + 1};
    item_state item2{context, int64_t(index) * 4 + 2};
    item_state item3{context, int64_t(index) * 4 + 3};

    for (uint32_t j = 0; j < full_dataset_item_parents; ++j)
    {
        item0.update(j);
        item1.update(j);
        item2.update(j);
        item3.update(j);
    }

    return hash2048{{item0.final(), item1.final(), item2.final(), item3.final()}};
}

namespace
{
using lookup_fn = hash1024 (*)(const epoch_context&, uint32_t);

inline hash512 hash_seed(const hash256& header_hash, uint64_t nonce) noexcept
{
    nonce = le::uint64(nonce);
    uint8_t init_data[sizeof(header_hash) + sizeof(nonce)];
    std::memcpy(&init_data[0], &header_hash, sizeof(header_hash));
    std::memcpy(&init_data[sizeof(header_hash)], &nonce, sizeof(nonce));

    return keccak512(init_data, sizeof(init_data));
}

inline hash256 hash_final(const hash512& seed, const hash256& mix_hash)
{
    uint8_t final_data[sizeof(seed) + sizeof(mix_hash)];
    std::memcpy(&final_data[0], seed.bytes, sizeof(seed));
    std::memcpy(&final_data[sizeof(seed)], mix_hash.bytes, sizeof(mix_hash));
    return keccak256(final_data, sizeof(final_data));
}

inline hash256 hash_kernel(
    const epoch_context& context, const hash512& seed, lookup_fn lookup) noexcept
{
    static constexpr size_t num_words = sizeof(hash1024) / sizeof(uint32_t);
    const uint32_t index_limit = static_cast<uint32_t>(context.full_dataset_num_items);
    const uint32_t seed_init = le::uint32(seed.word32s[0]);

    hash1024 mix{{le::uint32s(seed), le::uint32s(seed)}};

    for (uint32_t i = 0; i < num_dataset_accesses; ++i)
    {
        const uint32_t p = fnv1(i ^ seed_init, mix.word32s[i % num_words]) % index_limit;
        const hash1024 newdata = le::uint32s(lookup(context, p));

        for (size_t j = 0; j < num_words; ++j)
            mix.word32s[j] = fnv1(mix.word32s[j], newdata.word32s[j]);
    }

    hash256 mix_hash;
    for (size_t i = 0; i < num_words; i += 4)
    {
        const uint32_t h1 = fnv1(mix.word32s[i], mix.word32s[i + 1]);
        const uint32_t h2 = fnv1(h1, mix.word32s[i + 2]);
        const uint32_t h3 = fnv1(h2, mix.word32s[i + 3]);
        mix_hash.word32s[i / 4] = h3;
    }

    return le::uint32s(mix_hash);
}
}  // namespace

result hash(const epoch_context& context, const hash256& header_hash, uint64_t nonce) noexcept
{
    const hash512 seed = hash_seed(header_hash, nonce);
    const hash256 mix_hash = hash_kernel(context, seed, calculate_dataset_item_1024);
    return {hash_final(seed, mix_hash), mix_hash};
}

result hash(const epoch_context_full& context, const hash256& header_hash, uint64_t nonce) noexcept
{
    static const auto lazy_lookup = [](const epoch_context& context, uint32_t index) noexcept
    {
        auto full_dataset = static_cast<const epoch_context_full&>(context).full_dataset;
        hash1024& item = full_dataset[index];
        if (item.word64s[0] == 0)
        {
            // TODO: Copy elision here makes it thread-safe?
            item = calculate_dataset_item_1024(context, index);
        }

        return item;
    };

    const hash512 seed = hash_seed(header_hash, nonce);
    const hash256 mix_hash = hash_kernel(context, seed, lazy_lookup);
    return {hash_final(seed, mix_hash), mix_hash};
}

bool verify_final_hash(const hash256& header_hash, const hash256& mix_hash, uint64_t nonce,
    const hash256& boundary) noexcept
{
    const hash512 seed = hash_seed(header_hash, nonce);
    return is_less_or_equal(hash_final(seed, mix_hash), boundary);
}

bool verify(const epoch_context& context, const hash256& header_hash, const hash256& mix_hash,
    uint64_t nonce, const hash256& boundary) noexcept
{
    const hash512 seed = hash_seed(header_hash, nonce);
    if (!is_less_or_equal(hash_final(seed, mix_hash), boundary))
        return false;

    const hash256 expected_mix_hash = hash_kernel(context, seed, calculate_dataset_item_1024);
    return is_equal(expected_mix_hash, mix_hash);
}

search_result search_light(const epoch_context& context, const hash256& header_hash,
    const hash256& boundary, uint64_t start_nonce, size_t iterations) noexcept
{
    const uint64_t end_nonce = start_nonce + iterations;
    for (uint64_t nonce = start_nonce; nonce < end_nonce; ++nonce)
    {
        result r = hash(context, header_hash, nonce);
        if (is_less_or_equal(r.final_hash, boundary))
            return {r, nonce};
    }
    return {};
}

search_result search(const epoch_context_full& context, const hash256& header_hash,
    const hash256& boundary, uint64_t start_nonce, size_t iterations) noexcept
{
    const uint64_t end_nonce = start_nonce + iterations;
    for (uint64_t nonce = start_nonce; nonce < end_nonce; ++nonce)
    {
        result r = hash(context, header_hash, nonce);
        if (is_less_or_equal(r.final_hash, boundary))
            return {r, nonce};
    }
    return {};
}
}  // namespace ethash

using namespace ethash;

extern "C" {

ethash_hash256 ethash_calculate_epoch_seed(int epoch_number) noexcept
{
    ethash_hash256 epoch_seed = {};
    for (int i = 0; i < epoch_number; ++i)
        epoch_seed = ethash_keccak256_32(epoch_seed.bytes);
    return epoch_seed;
}

int ethash_calculate_light_cache_num_items(int epoch_number) noexcept
{
    static constexpr int item_size = sizeof(hash512);
    static constexpr int num_items_init = light_cache_init_size / item_size;
    static constexpr int num_items_growth = light_cache_growth / item_size;
    static_assert(
        light_cache_init_size % item_size == 0, "light_cache_init_size not multiple of item size");
    static_assert(
        light_cache_growth % item_size == 0, "light_cache_growth not multiple of item size");

    int num_items_upper_bound = num_items_init + epoch_number * num_items_growth;
    int num_items = ethash_find_largest_prime(num_items_upper_bound);
    return num_items;
}

int ethash_calculate_full_dataset_num_items(int epoch_number) noexcept
{
    static constexpr int item_size = sizeof(hash1024);
    static constexpr int num_items_init = full_dataset_init_size / item_size;
    static constexpr int num_items_growth = full_dataset_growth / item_size;
    static_assert(full_dataset_init_size % item_size == 0,
        "full_dataset_init_size not multiple of item size");
    static_assert(
        full_dataset_growth % item_size == 0, "full_dataset_growth not multiple of item size");

    int num_items_upper_bound = num_items_init + epoch_number * num_items_growth;
    int num_items = ethash_find_largest_prime(num_items_upper_bound);
    return num_items;
}

epoch_context* ethash_create_epoch_context(int epoch_number) noexcept
{
    return generic::create_epoch_context(build_light_cache, epoch_number, false);
}

epoch_context_full* ethash_create_epoch_context_full(int epoch_number) noexcept
{
    return generic::create_epoch_context(build_light_cache, epoch_number, true);
}

void ethash_destroy_epoch_context_full(epoch_context_full* context) noexcept
{
    ethash_destroy_epoch_context(context);
}

void ethash_destroy_epoch_context(epoch_context* context) noexcept
{
    context->~epoch_context();
    std::free(context);
}

}  // extern "C"
