// Ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <cstdint>

void fake_keccak256_default(uint64_t* out, const uint8_t* data, size_t size) noexcept;
void fake_keccak256_default_aligned(uint64_t* out, const uint8_t* data, size_t size) noexcept;
void fake_keccak256_fastest(uint64_t *out, const uint8_t *data, size_t size) noexcept;
void fake_keccak256_fastest_word4(uint64_t out[4], const uint64_t data[4]) noexcept;
void fake_keccakf1600(uint64_t* state) noexcept;
