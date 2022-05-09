// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0. See the LICENSE file.

/// @file
/// This file contains helper functions to handle big-endian architectures.
/// The Ethash algorithm is naturally defined for little-endian architectures
/// so for those the helpers are just no-op empty functions.
/// For big-endian architectures we need 32-bit and 64-bit byte swapping in
/// some places.

#pragma once

#include <ethash/ethash.hpp>

#if _WIN32

#include <stdlib.h>

#define bswap32 _byteswap_ulong
#define bswap64 _byteswap_uint64

// On Windows assume little endian.
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#define __BYTE_ORDER __LITTLE_ENDIAN

#elif __APPLE__

#include <machine/endian.h>

#define bswap32 __builtin_bswap32
#define bswap64 __builtin_bswap64

#else

#include <endian.h>

#define bswap32 __builtin_bswap32
#define bswap64 __builtin_bswap64

#endif

namespace ethash
{
#if __BYTE_ORDER == __LITTLE_ENDIAN

struct le
{
    static uint32_t uint32(uint32_t x) noexcept { return x; }
    static uint64_t uint64(uint64_t x) noexcept { return x; }

    static const hash1024& uint32s(const hash1024& h) noexcept { return h; }
    static const hash512& uint32s(const hash512& h) noexcept { return h; }
    static const hash256& uint32s(const hash256& h) noexcept { return h; }
};

struct be
{
    static uint64_t uint64(uint64_t x) noexcept { return bswap64(x); }
};


#elif __BYTE_ORDER == __BIG_ENDIAN

struct le
{
    static uint32_t uint32(uint32_t x) noexcept { return bswap32(x); }
    static uint64_t uint64(uint64_t x) noexcept { return bswap64(x); }

    static hash1024 uint32s(hash1024 h) noexcept
    {
        for (auto& w : h.word32s)
            w = uint32(w);
        return h;
    }

    static hash512 uint32s(hash512 h) noexcept
    {
        for (auto& w : h.word32s)
            w = uint32(w);
        return h;
    }

    static hash256 uint32s(hash256 h) noexcept
    {
        for (auto& w : h.word32s)
            w = uint32(w);
        return h;
    }
};

struct be
{
    static uint64_t uint64(uint64_t x) noexcept { return x; }
};

#endif
}  // namespace ethash