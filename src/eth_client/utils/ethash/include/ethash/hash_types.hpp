// ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018-2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#pragma once

#include <ethash/hash_types.h>

namespace ethash
{
using hash256 = ethash_hash256;
using hash512 = ethash_hash512;
using hash1024 = ethash_hash1024;
}  // namespace ethash
