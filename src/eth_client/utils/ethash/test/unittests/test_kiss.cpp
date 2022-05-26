// ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0. See the LICENSE file.

#include <ethash/bit_manipulation.h>
#include <ethash/kiss99.hpp>

#include <gtest/gtest.h>


// The KISS99 reference implementation from http://www.cse.yorku.ca/~oz/marsaglia-rng.html.
#define znew (z = 36969 * (z & 65535) + (z >> 16))
#define wnew (w = 18000 * (w & 65535) + (w >> 16))
#define MWC ((znew << 16) + wnew)
#define SHR3 (jsr ^= (jsr << 17), jsr ^= (jsr >> 13), jsr ^= (jsr << 5))
#define CONG (jcong = 69069 * jcong + 1234567)
#define KISS ((MWC ^ CONG) + SHR3)
static unsigned z = 362436069, w = 521288629, jsr = 123456789, jcong = 380116160;

NO_SANITIZE("unsigned-integer-overflow")
static unsigned kiss_reference()
{
    return KISS;
}


TEST(kiss99, generate)
{
    kiss99 rng;

    EXPECT_EQ(rng(), 769445856);
    EXPECT_EQ(rng(), 742012328);
    EXPECT_EQ(rng(), 2121196314);
    EXPECT_EQ(rng(), 2805620942);

    for (int i = 0; i < 100000 - 5; ++i)
        rng();

    // The 100000th number.
    EXPECT_EQ(rng(), 941074834);
}

TEST(kiss99, compare_with_reference)
{
    kiss99 rng;

    for (int i = 0; i < 100000; ++i)
        EXPECT_EQ(rng(), kiss_reference());
}