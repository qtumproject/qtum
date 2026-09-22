// ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2021 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#pragma once

#include <ethash/hash_types.h>

extern "C" {

/// Converts difficulty to boundary.
///
/// The validity of the Ethash final hash is given as hash <= (2^256 / difficulty).
/// This function computes boundary = (2^256 / difficulty).
///
/// The function is for testing purposes. The division is done using Knuth's D algorithm
/// with 32-bit words for portability. Using 64-bit words and precomputing divisor's inversion
/// (see intx library) would be much more efficient. Especially for Mainnet difficulty values
/// having ~53 bits.
///
/// @param difficulty  The difficulty value as big-endian 256-bit number.
/// @return            The boundary value as big-endian 256-bit number.
ethash_hash256 ethash_difficulty_to_boundary(const ethash_hash256* difficulty) noexcept;
}
