// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0. See the LICENSE file.

/// @file
/// Shared test cases.

#pragma once

// Put in anonymous namespace to allow be include in multiple files
// but also make iteration over test cases easy with range-based for loop.
namespace
{
struct hash_test_case
{
    int block_number;
    const char* header_hash_hex;
    const char* nonce_hex;
    const char* mix_hash_hex;
    const char* final_hash_hex;
};

hash_test_case hash_test_cases[] = {
    {
        0,
        "2a8de2adf89af77358250bf908bf04ba94a6e8c3ba87775564a41d269a05e4ce",
        "4242424242424242",
        "58f759ede17a706c93f13030328bcea40c1d1341fb26f2facd21ceb0dae57017",
        "dd47fd2d98db51078356852d7c4014e6a5d6c387c35f40e2875b74a256ed7906",
    },
    {
        2,
        "100cbec5e5ef82991290d0d93d758f19082e71f234cf479192a8b94df6da6bfe",
        "307692cf71b12f6d",
        "e55d02c555a7969361cf74a9ec6211d8c14e4517930a00442f171bdb1698d175",
        "ab9b13423cface72cbec8424221651bc2e384ef0f7a560e038fc68c8d8684829",
    },
    {
        2683077,
        "0313d03c5ed78694c90ecb3d04190b82d5b222c75ba4cab83383dde4d11ed512",
        "8c5eaec000788d41",
        "93e85c97b34ccd8091e09ddb513fdc9e680fa8898d4a0737205e60af710a3dcb",
        "00000000000204882a6213f68fe89bc368df25c1ad999f82532a7433e99bc48e",
    },
    {
        5000000,
        "bc544c2baba832600013bd5d1983f592e9557d04b0fb5ef7a100434a5fc8d52a",
        "4617a20003ba3f25",
        "94cd4e844619ee20989578276a0a9046877d569d37ba076bf2e8e34f76189dea",
        "0000000000001a5b18ae31b7417b7c045bc6dd78cc3b694c4bebfaefef6b3c56",
    },
    {
        5000001,
        "2cd14041cfc3bd13064cfd58e26c0bddf1e97a4202c4b8076444a7cd4515f8c3",
        "1af47f2007922384",
        "46cb1268ac6b218eb01b9bd46f1533561d085620c9bfc23eadb8ab106f84b2d8",
        "0000000000000a4230cfc483a3f739a5101192c3d4f9c8314f4508be24c28257",
    },
    {
        5000002,
        "9e79bced19062baf7c47e516ad3a1bd779222404d05b4205def30a13c7d87b5b",
        "c9a044201dd998f2",
        "028a19d6dcf0975972285f71a23fe3a468886ad24cc63f969158d427556d6bb5",
        "0000000000000eec80fbc4c472507b6c212c4f216043318f3fb3e696a0e80b06",
    },
    {
        5306861,
        "53a005f209a4dc013f022a5078c6b38ced76e767a30367ff64725f23ec652a9f",
        "d337f82001e992c5",
        "26c64f063dac85ae3585526b446be6754faf044876aa3e20f770bea567e04d1d",
        "00000000000003f1554d8071ff0903268fcb70f30f4af3bf7ec7dc69cdf509f3",
    },
};
}  // namespace