/* ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
 * Copyright 2019 Pawel Bylica.
 * Licensed under the Apache License, Version 2.0.
 */

#pragma once

/** The ethash library version. */
#define ETHASH_VERSION "1.1.0"

#ifdef __cplusplus
namespace ethash
{
/// The ethash library version.
constexpr auto version = ETHASH_VERSION;

}  // namespace ethash
#endif
