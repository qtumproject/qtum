// ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0. See the LICENSE file.

#include <ethash/ethash.hpp>

int main()
{
    uint8_t seed_bytes[32] = {0};
    ethash::hash256 seed = ethash::hash256_from_bytes(seed_bytes);
    return ethash::find_epoch_number(seed);
}
