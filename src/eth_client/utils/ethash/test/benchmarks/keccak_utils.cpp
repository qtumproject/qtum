// Ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0. See the LICENSE file.

#include <cstdint>
#include <cstring>

#define fix_endianness(X) X
#define fix_endianness64(X) X


void fake_keccakf1600(uint64_t* state) noexcept;


namespace
{
inline void xor_into_state(uint64_t* state, const uint64_t* block, size_t num_words) noexcept
{
    for (size_t i = 0; i < num_words; ++i)
        state[i] ^= fix_endianness(block[i]);
}

inline void xor_into_state(uint8_t* state, const uint8_t* block, size_t size)
{
    for (size_t i = 0; i < size; ++i)
        state[i] ^= block[i];
}

template <size_t bits>
inline void keccak_default_aligned(uint64_t* out, const uint64_t* data, size_t size) noexcept
{
    static constexpr size_t block_size = (1600 - bits * 2) / 8;
    static constexpr size_t block_words = block_size / sizeof(uint64_t);

    uint64_t state[25] = {};

    while (size >= block_words)
    {
        xor_into_state(state, data, block_words);
        fake_keccakf1600(state);
        data += block_words;
        size -= block_words;
    }

    // Final block:
    uint64_t block[block_words] = {};
    // Weirdly, GCC and clang are able to optimize memcpy better than for loop.
    std::memcpy(block, data, size * sizeof(uint64_t));

    // Padding:
    auto block_bytes = reinterpret_cast<unsigned char*>(block);
    block_bytes[size * sizeof(uint64_t)] = 0x01;
    block_bytes[block_size - 1] |= 0x80;

    xor_into_state(state, block, block_words);
    fake_keccakf1600(state);

    std::memcpy(out, state, bits / 8);
    //    return fix_endianness64(hash);
}


template <size_t bits>
inline void keccak_default(uint64_t* out, const uint8_t* data, size_t size) noexcept
{
    static constexpr size_t block_size = (1600 - bits * 2) / 8;
    static constexpr size_t block_words = block_size / sizeof(uint64_t);

    bool aligned = (uintptr_t)data % 8 == 0;

    uint64_t state[25] = {};

    uint64_t block[block_words];

    uint64_t* p;

    while (size >= block_size)
    {
        if (!aligned)
        {
            std::memcpy(block, data, block_size);
            p = block;
        }
        else
            p = (uint64_t*)data;
        xor_into_state(state, p, block_words);
        fake_keccakf1600(state);
        data += block_size;
        size -= block_size;
    }


    auto state_bytes = reinterpret_cast<uint8_t*>(state);
    xor_into_state(state_bytes, data, size);

    state_bytes[size] ^= 0x01;
    state_bytes[block_size - 1] ^= 0x80;

    fake_keccakf1600(state);

    std::memcpy(out, state, bits / 8);
    //    return fix_endianness64(hash);
}

/// Loads 64-bit integer from given memory location.
inline uint64_t load_le(const uint8_t *data) noexcept
{
    // memcpy is the best way of expressing the intention. Every compiler will
    // optimize is to single load instruction if the target architecture
    // supports unaligned memory access (GCC and clang even in O0).
    // This is great trick because we are violating C/C++ memory alignment
    // restrictions with no performance penalty.
    uint64_t word;
    memcpy(&word, data, sizeof(word));
    return word;
}

inline void keccak_fastest(uint64_t* out, const uint8_t* data, size_t size)
{
    static constexpr size_t block_size = (1600 - 256 * 2) / 8;

    union
    {
        uint64_t words[25];
        uint8_t bytes[200];
    } state = {{0}};


    while (size >= block_size)
    {
        for (size_t i = 0; i < (block_size / sizeof(uint64_t)); ++i)
            state.words[i] ^= fix_endianness(load_le(data + i * sizeof(uint64_t)));

        fake_keccakf1600(state.words);

        data += block_size;
        size -= block_size;
    }

    uint8_t* p_state_bytes = state.bytes;

    while (size >= sizeof(uint64_t))
    {
        uint64_t* p_state_word = (uint64_t*)p_state_bytes;
        *p_state_word ^= load_le(data);
        data += sizeof(uint64_t);
        size -= sizeof(uint64_t);
        p_state_bytes += sizeof(uint64_t);
    }

    for (size_t i = 0; i < size; ++i)
        p_state_bytes[i] ^= data[i];


    p_state_bytes[size] ^= 0x01;
    state.bytes[block_size - 1] ^= 0x80;

    fake_keccakf1600(state.words);

    for (int i = 0; i < 4; ++i)
        out[i] = state.words[i];
}
}  // namespace

void fake_keccak256_default_aligned(uint64_t* out, const uint8_t* data, size_t size) noexcept
{
    keccak_default_aligned<256>(out, (uint64_t*)data, size / 8);
}

void fake_keccak256_default(uint64_t* out, const uint8_t* data, size_t size) noexcept
{
    keccak_default<256>(out, data, size);
}

void fake_keccak256_fastest(uint64_t* out, const uint8_t* data, size_t size) noexcept
{
    keccak_fastest(out, data, size);
}

void fake_keccak256_fastest_word4(uint64_t out[4], const uint64_t data[4]) noexcept
{
    keccak_fastest(out, (const uint8_t*)data, 32);
}
