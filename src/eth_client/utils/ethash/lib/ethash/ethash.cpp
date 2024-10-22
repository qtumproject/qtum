// ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018-2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#include "ethash-internal.hpp"

#include "primes.h"
#include <ethash/keccak.hpp>
#include <cstdlib>
#include <cstring>

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
/// The core transformation of the FNV-1 hash function.
/// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function#FNV-1_hash.
[[clang::no_sanitize("unsigned-integer-overflow")]] inline uint32_t fnv1(
    uint32_t u, uint32_t v) noexcept
{
    static const uint32_t fnv_prime = 0x01000193;
    return (u * fnv_prime) ^ v;
}


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
    for (int i = 0; i <= max_epoch_number; ++i)
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

namespace
{
void build_light_cache(hash512 cache[], int num_items, const hash256& seed) noexcept
{
    hash512 item = keccak512(seed.bytes, sizeof(seed));
    cache[0] = item;
    for (int i = 1; i < num_items; ++i)
    {
        item = keccak512(item);
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

            cache[i] = keccak512(bitwise_xor(cache[v], cache[w]));
        }
    }
}

epoch_context_full* create_epoch_context(int epoch_number, bool full) noexcept
{
    static_assert(sizeof(epoch_context_full) < sizeof(hash512), "epoch_context too big");
    static constexpr size_t context_alloc_size = sizeof(hash512);

    if (epoch_number < 0 || epoch_number > max_epoch_number)
        return nullptr;

    const int light_cache_num_items = calculate_light_cache_num_items(epoch_number);
    const int full_dataset_num_items = calculate_full_dataset_num_items(epoch_number);
    const size_t light_cache_size = get_light_cache_size(light_cache_num_items);
    const size_t full_dataset_size =
        full ? static_cast<size_t>(full_dataset_num_items) * sizeof(hash1024) : 0;

    const size_t alloc_size = context_alloc_size + light_cache_size + full_dataset_size;

    char* const alloc_data = static_cast<char*>(std::calloc(1, alloc_size));
    if (!alloc_data)
        return nullptr;  // Signal out-of-memory by returning null pointer.

    hash512* const light_cache = reinterpret_cast<hash512*>(alloc_data + context_alloc_size);
    const hash256 epoch_seed = calculate_epoch_seed(epoch_number);
    build_light_cache(light_cache, light_cache_num_items, epoch_seed);

    hash1024* const full_dataset =
        full ? reinterpret_cast<hash1024*>(alloc_data + context_alloc_size + light_cache_size) :
               nullptr;

    epoch_context_full* const context = new (alloc_data) epoch_context_full{
        epoch_number,
        light_cache_num_items,
        light_cache,
        full_dataset_num_items,
        full_dataset,
    };

    return context;
}
}  // namespace

/// Calculates a full dataset item.
///
/// This consist of two 512-bit items defined by the Ethash specification, but these items
/// are never needed separately. Here the computation is done interleaved for better performance.
hash1024 calculate_dataset_item_1024(const epoch_context& context, uint32_t index) noexcept
{
    const uint32_t num_cache_items = static_cast<uint32_t>(context.light_cache_num_items);
    const hash512* const cache = context.light_cache;

    const uint32_t seed0 = index * 2;
    const uint32_t seed1 = seed0 + 1;

    hash512 mix0 = cache[seed0 % num_cache_items];
    hash512 mix1 = cache[seed1 % num_cache_items];

    mix0.word32s[0] ^= le::uint32(seed0);
    mix1.word32s[0] ^= le::uint32(seed1);

    mix0 = le::uint32s(keccak512(mix0));
    mix1 = le::uint32s(keccak512(mix1));

    for (uint32_t j = 0; j < full_dataset_item_parents; ++j)
    {
        constexpr size_t num_words = sizeof(mix0) / sizeof(uint32_t);
        const uint32_t t0 = fnv1(seed0 ^ j, mix0.word32s[j % num_words]);
        const uint32_t t1 = fnv1(seed1 ^ j, mix1.word32s[j % num_words]);
        mix0 = fnv1(mix0, le::uint32s(cache[t0 % num_cache_items]));
        mix1 = fnv1(mix1, le::uint32s(cache[t1 % num_cache_items]));
    }

    return hash1024{{keccak512(le::uint32s(mix0)), keccak512(le::uint32s(mix1))}};
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

result hash(const epoch_context_full& context, const hash256& header_hash, uint64_t nonce) noexcept
{
    static const auto lazy_lookup = [](const epoch_context& ctx, uint32_t index) noexcept {
        hash1024& item = static_cast<const epoch_context_full&>(ctx).full_dataset[index];
        if (item.word64s[0] == 0)
            item = calculate_dataset_item_1024(ctx, index);
        return item;
    };

    const hash512 seed = hash_seed(header_hash, nonce);
    const hash256 mix_hash = hash_kernel(context, seed, lazy_lookup);
    return {hash_final(seed, mix_hash), mix_hash};
}

search_result search_light(const epoch_context& context, const hash256& header_hash,
    const hash256& boundary, uint64_t start_nonce, size_t iterations) noexcept
{
    const uint64_t end_nonce = start_nonce + iterations;
    for (uint64_t nonce = start_nonce; nonce < end_nonce; ++nonce)
    {
        result r = hash(context, header_hash, nonce);
        if (less_equal(r.final_hash, boundary))
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
        if (less_equal(r.final_hash, boundary))
            return {r, nonce};
    }
    return {};
}


struct uint128
{
    uint64_t lo;
    uint64_t hi;
};

/// Full unsigned multiplication 64 x 64 -> 128.
[[clang::no_sanitize("unsigned-integer-overflow", "unsigned-shift-base")]] inline uint128 umul(
    uint64_t x, uint64_t y) noexcept
{
#ifdef __SIZEOF_INT128__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"  // Usage of __int128 triggers a pedantic warning.
    using builtin_uint128 = unsigned __int128;
#pragma GCC diagnostic pop

    const auto p = builtin_uint128{x} * builtin_uint128{y};
    const auto hi = static_cast<uint64_t>(p >> 64);
    const auto lo = static_cast<uint64_t>(p);
#else
    uint64_t xl = x & 0xffffffff;
    uint64_t xh = x >> 32;
    uint64_t yl = y & 0xffffffff;
    uint64_t yh = y >> 32;

    uint64_t t0 = xl * yl;
    uint64_t t1 = xh * yl;
    uint64_t t2 = xl * yh;
    uint64_t t3 = xh * yh;

    uint64_t u1 = t1 + (t0 >> 32);
    uint64_t u2 = t2 + (u1 & 0xffffffff);

    uint64_t lo = (u2 << 32) | (t0 & 0xffffffff);
    uint64_t hi = t3 + (u2 >> 32) + (u1 >> 32);
#endif
    return {lo, hi};
}

[[clang::no_sanitize("unsigned-integer-overflow")]] bool check_against_difficulty(
    const hash256& final_hash, const hash256& difficulty) noexcept
{
    constexpr size_t num_words = sizeof(hash256) / sizeof(uint64_t);

    // Convert difficulty to little-endian array of native 64-bit words.
    uint64_t d[num_words];
    for (size_t i = 0; i < num_words; ++i)
        d[i] = be::uint64(difficulty.word64s[num_words - 1 - i]);

    // Convert hash to little-endian array of native 64-bit words.
    uint64_t h[num_words];
    for (size_t i = 0; i < num_words; ++i)
        h[i] = be::uint64(final_hash.word64s[num_words - 1 - i]);

    // Compute p = h * d.
    uint64_t p[2 * num_words];

    {  // First round for d[0]. Fills p[0:4].
        uint64_t k = 0;
        for (size_t i = 0; i < num_words; ++i)
        {
            const auto r = umul(h[i], d[0]);
            p[i] = r.lo + k;
            k = r.hi + (p[i] < k);
        }
        p[4] = k;
    }

    // Rounds 1-3. The d[j] is likely 0 so omit the round in such case.
    for (size_t j = 1; j < num_words; ++j)
    {
        uint64_t k = 0;
        if (d[j] != 0)
        {
            for (size_t i = 0; i < num_words; ++i)
            {
                const auto r = umul(h[i], d[j]);
                const auto t = p[j + i] + r.lo;
                p[i + j] = t + k;
                k = r.hi + (t < r.lo) + (p[j + i] < k);
            }
        }
        p[j + 4] = k;  // Always init p[].
    }

    // Check if p <= 2^256.
    // In most cases the p < 2^256 (i.e. top 4 words are zero) should be enough.
    const auto high_zero = (p[7] | p[6] | p[5] | p[4]) == 0;
    if (high_zero)
        return true;

    // Lastly, check p == 2^256.
    const auto low_zero = (p[3] | p[2] | p[1] | p[0]) == 0;
    const auto high_one = (((p[7] | p[6] | p[5]) == 0) & (p[4] == 1)) != 0;
    return low_zero && high_one;
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
    constexpr int item_size = sizeof(hash512);
    constexpr int num_items_init = light_cache_init_size / item_size;
    constexpr int num_items_growth = light_cache_growth / item_size;
    static_assert(
        light_cache_init_size % item_size == 0, "light_cache_init_size not multiple of item size");
    static_assert(
        light_cache_growth % item_size == 0, "light_cache_growth not multiple of item size");

    if (epoch_number < 0 || epoch_number > max_epoch_number)
        return 0;

    int num_items_upper_bound = num_items_init + epoch_number * num_items_growth;
    int num_items = ethash_find_largest_prime(num_items_upper_bound);
    return num_items;
}

int ethash_calculate_full_dataset_num_items(int epoch_number) noexcept
{
    constexpr int item_size = sizeof(hash1024);
    constexpr int num_items_init = full_dataset_init_size / item_size;
    constexpr int num_items_growth = full_dataset_growth / item_size;
    static_assert(full_dataset_init_size % item_size == 0,
        "full_dataset_init_size not multiple of item size");
    static_assert(
        full_dataset_growth % item_size == 0, "full_dataset_growth not multiple of item size");

    if (epoch_number < 0 || epoch_number > max_epoch_number)
        return 0;

    int num_items_upper_bound = num_items_init + epoch_number * num_items_growth;
    int num_items = ethash_find_largest_prime(num_items_upper_bound);
    return num_items;
}

epoch_context* ethash_create_epoch_context(int epoch_number) noexcept
{
    return create_epoch_context(epoch_number, false);
}

epoch_context_full* ethash_create_epoch_context_full(int epoch_number) noexcept
{
    return create_epoch_context(epoch_number, true);
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

ethash_result ethash_hash(
    const epoch_context* context, const hash256* header_hash, uint64_t nonce) noexcept
{
    const hash512 seed = hash_seed(*header_hash, nonce);
    const hash256 mix_hash = hash_kernel(*context, seed, calculate_dataset_item_1024);
    return {hash_final(seed, mix_hash), mix_hash};
}


ethash_errc ethash_verify_final_hash_against_difficulty(const hash256* header_hash,
    const hash256* mix_hash, uint64_t nonce, const hash256* difficulty) noexcept
{
    return check_against_difficulty(
               hash_final(hash_seed(*header_hash, nonce), *mix_hash), *difficulty) ?
               ETHASH_SUCCESS :
               ETHASH_INVALID_FINAL_HASH;
}

ethash_errc ethash_verify_against_boundary(const epoch_context* context, const hash256* header_hash,
    const hash256* mix_hash, uint64_t nonce, const hash256* boundary) noexcept
{
    const hash512 seed = hash_seed(*header_hash, nonce);
    if (!less_equal(hash_final(seed, *mix_hash), *boundary))
        return ETHASH_INVALID_FINAL_HASH;

    const hash256 expected_mix_hash = hash_kernel(*context, seed, calculate_dataset_item_1024);
    return equal(expected_mix_hash, *mix_hash) ? ETHASH_SUCCESS : ETHASH_INVALID_MIX_HASH;
}

ethash_errc ethash_verify_against_difficulty(const epoch_context* context,
    const hash256* header_hash, const hash256* mix_hash, uint64_t nonce,
    const hash256* difficulty) noexcept
{
    const hash512 seed = hash_seed(*header_hash, nonce);
    if (!check_against_difficulty(hash_final(seed, *mix_hash), *difficulty))
        return ETHASH_INVALID_FINAL_HASH;

    const hash256 expected_mix_hash = hash_kernel(*context, seed, calculate_dataset_item_1024);
    return equal(expected_mix_hash, *mix_hash) ? ETHASH_SUCCESS : ETHASH_INVALID_MIX_HASH;
}

}  // extern "C"
