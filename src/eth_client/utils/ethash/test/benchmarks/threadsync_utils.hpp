// Ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0. See the LICENSE file.

#pragma once

#include <memory>

struct fake_cache;

std::shared_ptr<fake_cache> build_fake_cache(int id) noexcept;

bool verify_fake_cache_nosync(int id, int value) noexcept;

bool verify_fake_cache_mutex(int id, int value) noexcept;

bool verify_fake_cache_thread_local(int id, int value) noexcept;
