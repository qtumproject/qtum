// ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018-2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#include <ethash/ethash.hpp>
#include <ethash/version.h>

int main()
{
    static_assert(sizeof(ethash::version) >= 6, "incorrect ethash::version");

    uint8_t seed_bytes[32] = {0};
    ethash::hash256 seed = ethash::hash256_from_bytes(seed_bytes);
    return ethash::find_epoch_number(seed);
}
