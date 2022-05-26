/* ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
 * Copyright 2018 Pawel Bylica.
 * Licensed under the Apache License, Version 2.0. See the LICENSE file.
 */

#pragma once

#include "support/attributes.h"
#include <stdint.h>

/**
 * KISS PRNG by the spec from 1999.
 *
 * The implementation of KISS pseudo-random number generator
 * by the specification published on 21 Jan 1999 in
 * http://www.cse.yorku.ca/~oz/marsaglia-rng.html.
 * The KISS is not versioned so here we are using `kiss99` prefix to indicate
 * the version from 1999.
 *
 * The specification uses `unsigned long` type with the intention for 32-bit
 * values. Because in GCC/clang for 64-bit architectures `unsigned long` is
 * 64-bit size type, here the explicit `uint32_t` type is used.
 *
 * @defgroup kiss99 KISS99
 * @{
 */

/**
 * The KISS generator.
 */
class kiss99
{
    uint32_t z = 362436069;
    uint32_t w = 521288629;
    uint32_t jsr = 123456789;
    uint32_t jcong = 380116160;

public:
    /** Creates KISS generator state with default values provided by the specification. */
    kiss99() noexcept = default;

    /** Creates KISS generator state with provided init values.*/
    kiss99(uint32_t z, uint32_t w, uint32_t jsr, uint32_t jcong) noexcept
      : z{z}, w{w}, jsr{jsr}, jcong{jcong}
    {}

    /** Generates next number from the KISS generator. */
    NO_SANITIZE("unsigned-integer-overflow")
    uint32_t operator()() noexcept
    {
        z = 36969 * (z & 0xffff) + (z >> 16);
        w = 18000 * (w & 0xffff) + (w >> 16);

        jcong = 69069 * jcong + 1234567;

        jsr ^= (jsr << 17);
        jsr ^= (jsr >> 13);
        jsr ^= (jsr << 5);

        return (((z << 16) + w) ^ jcong) + jsr;
    }
};

/** @} */
