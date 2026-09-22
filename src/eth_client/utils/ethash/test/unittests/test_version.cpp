// ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#include <ethash/version.h>

#include <gtest/gtest.h>

TEST(libethash, version)
{
    static_assert(ethash::version[0] != 0, "incorrect ethash::version");

    EXPECT_EQ(ETHASH_VERSION, TEST_PROJECT_VERSION);
    EXPECT_EQ(ethash::version, TEST_PROJECT_VERSION);
}
