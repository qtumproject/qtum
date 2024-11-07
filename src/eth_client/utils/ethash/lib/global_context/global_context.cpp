// ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018-2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#include "../ethash/ethash-internal.hpp"
#include <ethash/global_context.h>

#include <memory>
#include <mutex>

#if !defined(__has_cpp_attribute)
#define __has_cpp_attribute(x) 0
#endif

#if __has_cpp_attribute(gnu::noinline)
#define ATTRIBUTE_NOINLINE [[gnu::noinline]]
#elif defined(_MSC_VER)
#define ATTRIBUTE_NOINLINE __declspec(noinline)
#else
#define ATTRIBUTE_NOINLINE
#endif

using namespace ethash;

namespace
{
std::mutex shared_context_mutex;
std::shared_ptr<epoch_context> shared_context;
thread_local std::shared_ptr<epoch_context> thread_local_context;

std::mutex shared_context_full_mutex;
std::shared_ptr<epoch_context_full> shared_context_full;
thread_local std::shared_ptr<epoch_context_full> thread_local_context_full;

/// Update thread local epoch context.
///
/// This function is on the slow path. It's separated to allow inlining the fast
/// path.
///
/// @todo: Redesign to guarantee deallocation before new allocation.
ATTRIBUTE_NOINLINE
void update_local_context(int epoch_number)
{
    // Release the shared pointer of the obsoleted context.
    thread_local_context.reset();

    // Local context invalid, check the shared context.
    std::lock_guard<std::mutex> lock{shared_context_mutex};

    if (!shared_context || shared_context->epoch_number != epoch_number)
    {
        // Release the shared pointer of the obsoleted context.
        shared_context.reset();

        // Build new context.
        shared_context = create_epoch_context(epoch_number);
    }

    thread_local_context = shared_context;
}

ATTRIBUTE_NOINLINE
void update_local_context_full(int epoch_number)
{
    // Release the shared pointer of the obsoleted context.
    thread_local_context_full.reset();

    // Local context invalid, check the shared context.
    std::lock_guard<std::mutex> lock{shared_context_full_mutex};

    if (!shared_context_full || shared_context_full->epoch_number != epoch_number)
    {
        // Release the shared pointer of the obsoleted context.
        shared_context_full.reset();

        // Build new context.
        shared_context_full = create_epoch_context_full(epoch_number);
    }

    thread_local_context_full = shared_context_full;
}
}  // namespace

const ethash_epoch_context* ethash_get_global_epoch_context(int epoch_number) noexcept
{
    // Check if local context matches epoch number.
    if (!thread_local_context || thread_local_context->epoch_number != epoch_number)
        update_local_context(epoch_number);

    return thread_local_context.get();
}

const ethash_epoch_context_full* ethash_get_global_epoch_context_full(int epoch_number) noexcept
{
    // Check if local context matches epoch number.
    if (!thread_local_context_full || thread_local_context_full->epoch_number != epoch_number)
        update_local_context_full(epoch_number);

    return thread_local_context_full.get();
}
