/* ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
 * Copyright 2018-2019 Pawel Bylica.
 * Licensed under the Apache License, Version 2.0.
 */

#pragma once

#include <ethash/hash_types.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef __has_cpp_attribute
#define __has_cpp_attribute(X) 0
#endif

#ifndef __has_attribute
#define __has_attribute(X) 0
#endif

#ifndef __cplusplus
#define noexcept  // Ignore noexcept in C code.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The Ethash algorithm revision implemented as specified in the Ethash spec
 * https://github.com/ethereum/wiki/wiki/Ethash.
 */
#define ETHASH_REVISION "23"

#define ETHASH_EPOCH_LENGTH 30000
#define ETHASH_LIGHT_CACHE_ITEM_SIZE 64
#define ETHASH_FULL_DATASET_ITEM_SIZE 128
#define ETHASH_NUM_DATASET_ACCESSES 64

/// The maximum epoch number supported by this implementation.
///
/// The value represents the last epoch where the light cache size fits 4GB size limit.
/// It also allows to make some assumptions about the max values of datasets indices.
/// The DAG size in the last epoch is 252GB and the block number is above 979M
/// which gives over 60 years of blocks assuming 2s block times.
#define ETHASH_MAX_EPOCH_NUMBER 32639

/** Ethash error codes. */
enum ethash_errc
{
    ETHASH_SUCCESS = 0,
    ETHASH_INVALID_FINAL_HASH = 1,
    ETHASH_INVALID_MIX_HASH = 2
};
typedef enum ethash_errc ethash_errc;


struct ethash_epoch_context
{
    const int epoch_number;
    const int light_cache_num_items;
    const union ethash_hash512* const light_cache;
    const int full_dataset_num_items;
};


struct ethash_epoch_context_full;


struct ethash_result
{
    union ethash_hash256 final_hash;
    union ethash_hash256 mix_hash;
};


/**
 * Calculates the number of items in the light cache for given epoch.
 *
 * This function will search for a prime number matching the criteria given
 * by the Ethash so the execution time is not constant. It takes ~ 0.01 ms.
 *
 * @param epoch_number  The epoch number.
 * @return              The number items in the light cache.
 */
int ethash_calculate_light_cache_num_items(int epoch_number) noexcept;


/**
 * Calculates the number of items in the full dataset for given epoch.
 *
 * This function will search for a prime number matching the criteria given
 * by the Ethash so the execution time is not constant. It takes ~ 0.05 ms.
 *
 * @param epoch_number  The epoch number.
 * @return              The number items in the full dataset.
 */
int ethash_calculate_full_dataset_num_items(int epoch_number) noexcept;

/**
 * Calculates the epoch seed hash.
 * @param epoch_number  The epoch number.
 * @return              The epoch seed hash.
 */
union ethash_hash256 ethash_calculate_epoch_seed(int epoch_number) noexcept;


struct ethash_epoch_context* ethash_create_epoch_context(int epoch_number) noexcept;

/**
 * Creates the epoch context with the full dataset initialized.
 *
 * The memory for the full dataset is only allocated and marked as "not-generated".
 * The items of the full dataset are generated on the fly when hit for the first time.
 *
 * The memory allocated in the context MUST be freed with ethash_destroy_epoch_context_full().
 *
 * @param epoch_number  The epoch number.
 * @return  Pointer to the context or null in case of memory allocation failure.
 */
struct ethash_epoch_context_full* ethash_create_epoch_context_full(int epoch_number) noexcept;

void ethash_destroy_epoch_context(struct ethash_epoch_context* context) noexcept;

void ethash_destroy_epoch_context_full(struct ethash_epoch_context_full* context) noexcept;


struct ethash_result ethash_hash(const struct ethash_epoch_context* context,
    const union ethash_hash256* header_hash, uint64_t nonce) noexcept;

/**
 * Verify Ethash validity of a header hash against given boundary.
 *
 * This checks if final hash header_hash is within difficulty boundary header_hash <= boundary,
 * where boundary = 2^256 / difficulty.
 * It also checks if the Ethash result produced out of (header_hash, nonce) matches the provided
 * mix_hash.
 *
 * In most use-cases users should have access to the difficulty value directly therefore
 * usage of ethash_verify_against_difficulty() is recommended instead.
 *
 * @return  Error code: ::ETHASH_SUCCESS if valid, ::ETHASH_INVALID_FINAL_HASH if the final hash is
 *          not within provided boundary, ::ETHASH_INVALID_MIX_HASH if the provided mix hash
 *          mismatches the computed one.
 */
ethash_errc ethash_verify_against_boundary(const struct ethash_epoch_context* context,
    const union ethash_hash256* header_hash, const union ethash_hash256* mix_hash, uint64_t nonce,
    const union ethash_hash256* boundary) noexcept;

/**
 * Verify Ethash validity of a header hash against given difficulty.
 *
 * This checks if final hash header_hash satisfies difficulty requirement
 * header_hash <= (2^256 / difficulty). The checks is implemented by multiplication
 * header_hash * difficulty <= 2^256 therefore big-number division is not needed.
 * It also checks if the Ethash result produced out of (header_hash, nonce) matches the provided
 * mix_hash.
 *
 * @param difficulty  The difficulty number as big-endian 256-bit value. The encoding matches the
 *                    way the difficulty is stored in block headers.
 *
 * @return  Error code: ::ETHASH_SUCCESS if valid, ::ETHASH_INVALID_FINAL_HASH if the final hash
 *          does not satisfy difficulty, ::ETHASH_INVALID_MIX_HASH if the provided mix hash
 *          mismatches the computed one.
 */
ethash_errc ethash_verify_against_difficulty(const struct ethash_epoch_context* context,
    const union ethash_hash256* header_hash, const union ethash_hash256* mix_hash, uint64_t nonce,
    const union ethash_hash256* difficulty) noexcept;


/**
 * Verify only the final hash. This can be performed quickly without accessing Ethash context.
 *
 * @return  Error code: ::ETHASH_SUCCESS if valid, ::ETHASH_INVALID_FINAL_HASH if the final hash
 *          does not satisfy difficulty.
 */
ethash_errc ethash_verify_final_hash_against_difficulty(const union ethash_hash256* header_hash,
    const union ethash_hash256* mix_hash, uint64_t nonce,
    const union ethash_hash256* difficulty) noexcept;

#ifdef __cplusplus
}
#endif
