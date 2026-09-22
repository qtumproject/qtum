// ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#pragma once

#include "../../lib/ethash/endianness.hpp"
#include <ethash/ethash.hpp>
#include <string>

template <typename Hash>
inline std::string to_hex(const Hash& h)
{
    static const auto hex_chars = "0123456789abcdef";
    std::string str;
    str.reserve(sizeof(h) * 2);
    for (auto b : h.bytes)
    {
        str.push_back(hex_chars[uint8_t(b) >> 4]);
        str.push_back(hex_chars[uint8_t(b) & 0xf]);
    }
    return str;
}

inline ethash::hash256 to_hash256(const std::string& hex)
{
    auto parse_digit = [](char d) -> int { return d <= '9' ? (d - '0') : (d - 'a' + 10); };

    ethash::hash256 hash = {};
    for (size_t i = 1; i < hex.size(); i += 2)
    {
        int h = parse_digit(hex[i - 1]);
        int l = parse_digit(hex[i]);
        hash.bytes[i / 2] = uint8_t((h << 4) | l);
    }
    return hash;
}

/// Comparison operator for hash256 to be used in unit tests.
inline bool operator==(const ethash::hash256& a, const ethash::hash256& b) noexcept
{
    return std::memcmp(a.bytes, b.bytes, sizeof(a)) == 0;
}

inline bool operator!=(const ethash::hash256& a, const ethash::hash256& b) noexcept
{
    return !(a == b);
}

[[clang::no_sanitize("unsigned-integer-overflow")]] inline ethash::hash256 inc(
    const ethash::hash256& x) noexcept
{
    ethash::hash256 z{};
    bool carry = true;
    for (int i = 3; i >= 0; --i)
    {
        const auto t = ethash::be::uint64(x.word64s[i]);
        const auto s = t + carry;
        carry = s < t;
        z.word64s[i] = ethash::be::uint64(s);
    }
    return z;
}

[[clang::no_sanitize("unsigned-integer-overflow")]] inline ethash::hash256 dec(
    const ethash::hash256& x) noexcept
{
    ethash::hash256 z{};
    bool borrow = true;
    for (int i = 3; i >= 0; --i)
    {
        const auto t = ethash::be::uint64(x.word64s[i]);
        const auto d = t - borrow;
        borrow = d > t;
        z.word64s[i] = ethash::be::uint64(d);
    }
    return z;
}

inline const ethash::epoch_context& get_ethash_epoch_context_0() noexcept
{
    static ethash::epoch_context_ptr context = ethash::create_epoch_context(0);
    return *context;
}
