// ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0. See the LICENSE file.

#include <ethash/endianness.hpp>
#include <ethash/progpow.hpp>

#include "helpers.hpp"
#include "progpow_test_vectors.hpp"

#include <gtest/gtest.h>
#include <array>


TEST(progpow, l1_cache)
{
    auto& context = get_ethash_epoch_context_0();

    constexpr auto test_size = 20;
    std::array<uint32_t, test_size> cache_slice;
    for (size_t i = 0; i < cache_slice.size(); ++i)
        cache_slice[i] = ethash::le::uint32(context.l1_cache[i]);

    const std::array<uint32_t, test_size> expected{
        {690150178, 1181503948, 2248155602, 2118233073, 2193871115, 1791778428, 1067701239,
            724807309, 530799275, 3480325829, 3899029234, 1998124059, 2541974622, 1100859971,
            1297211151, 3268320000, 2217813733, 2690422980, 3172863319, 2651064309}};
    EXPECT_EQ(cache_slice, expected);
}

TEST(progpow, hash_empty)
{
    auto& context = get_ethash_epoch_context_0();

    const auto result = progpow::hash(context, 0, {}, 0);
    const auto mix_hex = "faeb1be51075b03a4ff44b335067951ead07a3b078539ace76fd56fc410557a3";
    const auto final_hex = "63155f732f2bf556967f906155b510c917e48e99685ead76ea83f4eca03ab12b";
    EXPECT_EQ(to_hex(result.mix_hash), mix_hex);
    EXPECT_EQ(to_hex(result.final_hash), final_hex);
}

TEST(progpow, hash_30000)
{
    const int block_number = 30000;
    const auto header =
        to_hash256("ffeeddccbbaa9988776655443322110000112233445566778899aabbccddeeff");
    const uint64_t nonce = 0x123456789abcdef0;

    auto context = ethash::create_epoch_context(ethash::get_epoch_number(block_number));

    const auto result = progpow::hash(*context, block_number, header, nonce);
    const auto mix_hex = "11f19805c58ab46610ff9c719dcf0a5f18fa2f1605798eef770c47219274767d";
    const auto final_hex = "5b7ccd472dbefdd95b895cac8ece67ff0deb5a6bd2ecc6e162383d00c3728ece";
    EXPECT_EQ(to_hex(result.mix_hash), mix_hex);
    EXPECT_EQ(to_hex(result.final_hash), final_hex);
}

TEST(progpow, hash_and_verify)
{
    ethash::epoch_context_ptr context{nullptr, nullptr};

    for (auto& t : progpow_hash_test_cases)
    {
        const auto epoch_number = ethash::get_epoch_number(t.block_number);
        if (!context || context->epoch_number != epoch_number)
            context = ethash::create_epoch_context(epoch_number);

        const auto header_hash = to_hash256(t.header_hash_hex);
        const auto nonce = std::stoull(t.nonce_hex, nullptr, 16);
        const auto result = progpow::hash(*context, t.block_number, header_hash, nonce);
        EXPECT_EQ(to_hex(result.mix_hash), t.mix_hash_hex);
        EXPECT_EQ(to_hex(result.final_hash), t.final_hash_hex);

        auto success = progpow::verify(
            *context, t.block_number, header_hash, result.mix_hash, nonce, result.final_hash);
        EXPECT_TRUE(success);

        auto lower_boundary = result.final_hash;
        --lower_boundary.bytes[31];
        auto final_failure = progpow::verify(
            *context, t.block_number, header_hash, result.mix_hash, nonce, lower_boundary);
        EXPECT_FALSE(final_failure);

        auto different_mix = result.mix_hash;
        ++different_mix.bytes[7];
        auto mix_failure = progpow::verify(
            *context, t.block_number, header_hash, different_mix, nonce, result.final_hash);
        EXPECT_FALSE(mix_failure);
    }
}

TEST(progpow, search)
{
    auto ctxp = ethash::create_epoch_context_full(0);
    auto& ctx = *ctxp;
    auto& ctxl = reinterpret_cast<const ethash::epoch_context&>(ctx);

    auto boundary = to_hash256("00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    auto sr = progpow::search(ctx, 0, {}, boundary, 0, 100);
    auto srl = progpow::search_light(ctxl, 0, {}, boundary, 0, 100);

    EXPECT_EQ(sr.mix_hash, ethash::hash256{});
    EXPECT_EQ(sr.final_hash, ethash::hash256{});
    EXPECT_EQ(sr.nonce, 0x0);
    EXPECT_EQ(sr.mix_hash, srl.mix_hash);
    EXPECT_EQ(sr.final_hash, srl.final_hash);
    EXPECT_EQ(sr.nonce, srl.nonce);

    sr = progpow::search(ctx, 0, {}, boundary, 100, 100);
    srl = progpow::search_light(ctxl, 0, {}, boundary, 100, 100);

    EXPECT_NE(sr.mix_hash, ethash::hash256{});
    EXPECT_NE(sr.final_hash, ethash::hash256{});
    EXPECT_EQ(sr.nonce, 185);
    EXPECT_EQ(sr.mix_hash, srl.mix_hash);
    EXPECT_EQ(sr.final_hash, srl.final_hash);
    EXPECT_EQ(sr.nonce, srl.nonce);

    auto r = progpow::hash(ctx, 0, {}, 185);
    EXPECT_EQ(sr.final_hash, r.final_hash);
    EXPECT_EQ(sr.mix_hash, r.mix_hash);
}

#if ETHASH_TEST_GENERATION
TEST(progpow, generate_hash_test_cases)
{
    constexpr auto num_epochs = 2;

    using namespace progpow;
    hash256 h{};
    for (int e = 0; e < num_epochs; ++e)
    {
        auto context = ethash::create_epoch_context(e);
        auto block_numbers = {
            e * epoch_length,
            e * epoch_length + period_length - 1,
            e * epoch_length + period_length,
            e * epoch_length + 2 * period_length - 1,
            (e + 1) * epoch_length - period_length,
            (e + 1) * epoch_length - 1,
        };
        for (auto b : block_numbers)
        {
            auto i = uint64_t(b);
            auto nonce = i * i * i * 977 + i * i * 997 + i * 1009;
            auto r = hash(*context, b, h, nonce);

            std::cout << "{" << b << ", \"" << to_hex(h) << "\", \"" << std::hex
                      << std::setfill('0') << std::setw(16) << nonce << std::dec << "\", \""
                      << to_hex(r.mix_hash) << "\", \"" << to_hex(r.final_hash) << "\"},\n";

            h = r.final_hash;
        }
    }
}
#endif