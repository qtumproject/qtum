// ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0. See the LICENSE file.

#include <ethash/bit_manipulation.h>

#include <gtest/gtest.h>

TEST(bit_manipulation, rotl32)
{
    EXPECT_EQ(rotl32(0, 0), 0);
    EXPECT_EQ(rotl32(0, 4321), 0);

    EXPECT_EQ(rotl32(1, 0), 1);
    EXPECT_EQ(rotl32(1, 1), 2);
    EXPECT_EQ(rotl32(1, 2), 4);
    EXPECT_EQ(rotl32(1, 31), 1 << 31);
    EXPECT_EQ(rotl32(1, 32), 1);
    EXPECT_EQ(rotl32(1, 33), 2);

    EXPECT_EQ(rotl32(3, 0), 3);
    EXPECT_EQ(rotl32(3, 1), 6);
    EXPECT_EQ(rotl32(3, 30), 3 << 30);
    EXPECT_EQ(rotl32(3, 31), (1 << 31) | 1);
    EXPECT_EQ(rotl32(3, 32), 3);
    EXPECT_EQ(rotl32(3, 33), 6);
}

TEST(bit_manipulation, rotr32)
{
    EXPECT_EQ(rotr32(0, 0), 0);
    EXPECT_EQ(rotr32(0, 4321), 0);

    EXPECT_EQ(rotr32(1, 0), 1);
    EXPECT_EQ(rotr32(1, 1), 1 << 31);
    EXPECT_EQ(rotr32(1, 2), 1 << 30);
    EXPECT_EQ(rotr32(1, 30), 1 << 2);
    EXPECT_EQ(rotr32(1, 31), 1 << 1);
    EXPECT_EQ(rotr32(1, 32), 1);
    EXPECT_EQ(rotr32(1, 33), 1 << 31);

    EXPECT_EQ(rotr32(3, 0), 3);
    EXPECT_EQ(rotr32(3, 1), (1 << 31) | 1);
    EXPECT_EQ(rotr32(3, 2), (3 << 30));
    EXPECT_EQ(rotr32(3, 30), 12);
    EXPECT_EQ(rotr32(3, 31), 6);
    EXPECT_EQ(rotr32(3, 32), 3);
    EXPECT_EQ(rotr32(3, 33), (1 << 31) | 1);
}

TEST(bit_manipulation, clz32)
{
    EXPECT_EQ(clz32(0), 32);
    EXPECT_EQ(clz32(1), 31);
    EXPECT_EQ(clz32(1 << 1), 30);
    EXPECT_EQ(clz32(1 << 2), 29);
    EXPECT_EQ(clz32(1 << 30), 1);
    EXPECT_EQ(clz32(1u << 31), 0);
    EXPECT_EQ(clz32(4321), 19);
}

TEST(bit_manipulation, popcount32)
{
    EXPECT_EQ(popcount32(0), 0);
    EXPECT_EQ(popcount32(1), 1);
    EXPECT_EQ(popcount32(1 << 16), 1);
    EXPECT_EQ(popcount32(3), 2);
    EXPECT_EQ(popcount32(3 << 17), 2);
    EXPECT_EQ(popcount32(9 << 18), 2);
    EXPECT_EQ(popcount32(~0u), 32);
}

TEST(bit_manipulation, mul_hi32)
{
    EXPECT_EQ(mul_hi32(0, 0), 0);
    EXPECT_EQ(mul_hi32(0, 1), 0);
    EXPECT_EQ(mul_hi32(1, 0), 0);
    EXPECT_EQ(mul_hi32(1, 1), 0);

    EXPECT_EQ(mul_hi32(1 << 16, 1 << 16), 1);
    EXPECT_EQ(mul_hi32(1 << 16, (1 << 16) + 1), 1);

    EXPECT_EQ(mul_hi32(1 << 30, 1 << 30), 1 << 28);
    EXPECT_EQ(mul_hi32(1u << 31, 1u << 31), 1 << 30);

    EXPECT_EQ(mul_hi32(~0u, ~0u), ~0u - 1);
}
