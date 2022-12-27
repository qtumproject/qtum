/* ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
 * Copyright 2018 Pawel Bylica.
 * Licensed under the Apache License, Version 2.0. See the LICENSE file.
 */

#include <ethash/keccak.h>

#include "support/attributes.h"
#include <string.h>

#if _WIN32
/* On Windows assume little endian. */
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#define __BYTE_ORDER __LITTLE_ENDIAN
#elif __APPLE__
#include <machine/endian.h>
#else
#include <endian.h>
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define to_le64(X) X
#else
#define to_le64(X) __builtin_bswap64(X)
#endif


/** Loads 64-bit integer from given memory location as little-endian number. */
static INLINE ALWAYS_INLINE uint64_t load_le(const uint8_t* data)
{
    /* memcpy is the best way of expressing the intention. Every compiler will
       optimize is to single load instruction if the target architecture
       supports unaligned memory access (GCC and clang even in O0).
       This is great trick because we are violating C/C++ memory alignment
       restrictions with no performance penalty. */
    uint64_t word;
    memcpy(&word, data, sizeof(word));
    return to_le64(word);
}

static INLINE ALWAYS_INLINE void keccak(
    uint64_t* out, size_t bits, const uint8_t* data, size_t size)
{
    static const size_t word_size = sizeof(uint64_t);
    const size_t hash_size = bits / 8;
    const size_t block_size = (1600 - bits * 2) / 8;

    size_t i;
    uint64_t* state_iter;
    uint64_t last_word = 0;
    uint8_t* last_word_iter = (uint8_t*)&last_word;

    uint64_t state[25] = {0};

    while (size >= block_size)
    {
        for (i = 0; i < (block_size / word_size); ++i)
        {
            state[i] ^= load_le(data);
            data += word_size;
        }

        ethash_keccakf1600(state);

        size -= block_size;
    }

    state_iter = state;

    while (size >= word_size)
    {
        *state_iter ^= load_le(data);
        ++state_iter;
        data += word_size;
        size -= word_size;
    }

    while (size > 0)
    {
        *last_word_iter = *data;
        ++last_word_iter;
        ++data;
        --size;
    }
    *last_word_iter = 0x01;
    *state_iter ^= to_le64(last_word);

    state[(block_size / word_size) - 1] ^= 0x8000000000000000;

    ethash_keccakf1600(state);

    for (i = 0; i < (hash_size / word_size); ++i)
        out[i] = to_le64(state[i]);
}

union ethash_hash256 ethash_keccak256(const uint8_t* data, size_t size)
{
    union ethash_hash256 hash;
    keccak(hash.word64s, 256, data, size);
    return hash;
}

union ethash_hash256 ethash_keccak256_32(const uint8_t data[32])
{
    union ethash_hash256 hash;
    keccak(hash.word64s, 256, data, 32);
    return hash;
}

union ethash_hash512 ethash_keccak512(const uint8_t* data, size_t size)
{
    union ethash_hash512 hash;
    keccak(hash.word64s, 512, data, size);
    return hash;
}

union ethash_hash512 ethash_keccak512_64(const uint8_t data[64])
{
    union ethash_hash512 hash;
    keccak(hash.word64s, 512, data, 64);
    return hash;
}
