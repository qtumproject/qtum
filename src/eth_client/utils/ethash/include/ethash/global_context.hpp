// ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.
#pragma once

#include <ethash/global_context.h>

namespace ethash
{
using epoch_context = ethash_epoch_context;
using epoch_context_full = ethash_epoch_context_full;

/// Get global shared epoch context.
inline const epoch_context& get_global_epoch_context(int epoch_number) noexcept
{
    return *ethash_get_global_epoch_context(epoch_number);
}

/// Get global shared epoch context with full dataset initialized.
inline const epoch_context_full& get_global_epoch_context_full(int epoch_number) noexcept
{
    return *ethash_get_global_epoch_context_full(epoch_number);
}
}  // namespace ethash
