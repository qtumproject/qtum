// Ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0. See the LICENSE file.

#include "threadsync_utils.hpp"

#include <cstdlib>
#include <memory>
#include <mutex>

struct fake_cache : std::enable_shared_from_this<fake_cache>
{
    int id = -1;
    int payload = 0;
};

std::shared_ptr<fake_cache> build_fake_cache(int id) noexcept
{
    auto handle = std::make_shared<fake_cache>();
    handle->id = id;
    handle->payload = id * id;
    return handle;
}

std::shared_ptr<fake_cache> build_sentinel() noexcept
{
    static thread_local fake_cache sentinel;
    return std::shared_ptr<fake_cache>(&sentinel, [](fake_cache*){});
}

std::shared_ptr<fake_cache> build_sentinel2() noexcept
{
    return std::make_shared<fake_cache>();
}

namespace
{
std::shared_ptr<fake_cache> handle_nosync = build_sentinel();
}

bool verify_fake_cache_nosync(int id, int value) noexcept
{
    if (handle_nosync->id != id)
        handle_nosync = build_fake_cache(id);
    return handle_nosync->payload == value;
}

namespace
{
std::shared_ptr<fake_cache> handle_mutex;
std::mutex mutex;
}  // namespace

bool verify_fake_cache_mutex(int id, int value) noexcept
{
    std::lock_guard<std::mutex> lock{mutex};

    if (!handle_mutex || handle_mutex->id != id)
        handle_mutex = build_fake_cache(id);
    return handle_mutex->payload == value;
}

namespace
{
thread_local std::shared_ptr<fake_cache> handle_thread_local = build_sentinel();
std::mutex build_mutex;
}  // namespace

bool verify_fake_cache_thread_local(int id, int value) noexcept
{
    if (handle_thread_local->id != id)
    {
        std::lock_guard<std::mutex> lock{build_mutex};
        handle_thread_local = build_fake_cache(id);
    }
    return handle_thread_local->payload == value;
}
