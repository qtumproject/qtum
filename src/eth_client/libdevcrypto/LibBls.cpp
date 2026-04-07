#include <libdevcrypto/LibBls.h>
#include <evmone_precompiles/bls.hpp>
#include <algorithm>

using namespace std;
using namespace dev;
using namespace dev::crypto;

pair<bool, bytes> dev::crypto::add_G1_bls(dev::bytesConstRef input)
{
    bytes output(128, 0);
    size_t output_size = output.size();
    size_t input_size = input.size();
    assert(output_size >= 128);
    if (input_size != 256)
        return {false, bytes{}};

    uint8_t _rx[64];
    uint8_t _ry[64];
    const auto r = evmone::crypto::bls::g1_add(_rx,
        _ry,
        &input[0],
        &input[64],
        &input[128],
        &input[192]);

    if (!r)
        return {false, bytes{}};

    std::copy(_rx, _rx + 64, output.begin());
    std::copy(_ry, _ry + 64, output.begin() + 64);
    return {true, output};
}

pair<bool, bytes> dev::crypto::msm_G1_bls(dev::bytesConstRef input)
{
    bytes output(128, 0);
    size_t output_size = output.size();
    size_t input_size = input.size();
    assert(output_size >= 128);
    if (input_size == 0 || input_size % SINGLE_ENTRY_SIZE_MSM_G1 != 0)
        return {false, bytes{}};

    uint8_t _rx[64];
    uint8_t _ry[64];
    const auto r = evmone::crypto::bls::g1_msm(_rx,
        _ry,
        &input[0],
        input_size);

    if (!r)
        return {false, bytes{}};

    std::copy(_rx, _rx + 64, output.begin());
    std::copy(_ry, _ry + 64, output.begin() + 64);
    return {true, output};
}

pair<bool, bytes> dev::crypto::add_G2_bls(dev::bytesConstRef input)
{
    bytes output(256, 0);
    size_t output_size = output.size();
    size_t input_size = input.size();
    assert(output_size >= 256);
    if (input_size != 512)
        return {false, bytes{}};

    uint8_t _rx[128];
    uint8_t _ry[128];
    const auto r = evmone::crypto::bls::g2_add(_rx,
        _ry,
        &input[0],
        &input[128],
        &input[256],
        &input[384]);

    if (!r)
        return {false, bytes{}};

    std::copy(_rx, _rx + 128, output.begin());
    std::copy(_ry, _ry + 128, output.begin() + 128);
    return {true, output};
}

pair<bool, bytes> dev::crypto::msm_G2_bls(dev::bytesConstRef input)
{
    bytes output(256, 0);
    size_t output_size = output.size();
    size_t input_size = input.size();
    assert(output_size >= 256);
    if (input_size == 0 || input_size % SINGLE_ENTRY_SIZE_MSM_G2 != 0)
        return {false, bytes{}};

    uint8_t _rx[128];
    uint8_t _ry[128];
    const auto r = evmone::crypto::bls::g2_msm(_rx,
        _ry,
        &input[0],
        input_size);

    if (!r)
        return {false, bytes{}};

    std::copy(_rx, _rx + 128, output.begin());
    std::copy(_ry, _ry + 128, output.begin() + 128);
    return {true, output};
}

pair<bool, bytes> dev::crypto::pairing_check_bls(dev::bytesConstRef input)
{
    bytes output(32, 0);
    size_t output_size = output.size();
    size_t input_size = input.size();
    assert(output_size >= 32);
    if (input_size == 0 || input_size % PAIR_SIZE_G1_G2 != 0)
        return {false, bytes{}};

    uint8_t _r[32];
    const auto r = evmone::crypto::bls::pairing_check(_r,
        &input[0],
        input_size);

    if (!r)
        return {false, bytes{}};

    std::copy(_r, _r + 32, output.begin());
    return {true, output};
}

pair<bool, bytes> dev::crypto::map_fp_to_G1_bls(dev::bytesConstRef input)
{
    bytes output(128, 0);
    size_t output_size = output.size();
    size_t input_size = input.size();
    assert(output_size >= 128);
    if (input_size != 64)
        return {false, bytes{}};

    uint8_t _rx[64];
    uint8_t _ry[64];
    const auto r = evmone::crypto::bls::map_fp_to_g1(_rx,
        _ry,
        &input[0]);

    if (!r)
        return {false, bytes{}};

    std::copy(_rx, _rx + 64, output.begin());
    std::copy(_ry, _ry + 64, output.begin() + 64);
    return {true, output};
}

pair<bool, bytes> dev::crypto::map_fp2_to_G2_bls(dev::bytesConstRef input)
{
    bytes output(256, 0);
    size_t output_size = output.size();
    size_t input_size = input.size();
    assert(output_size >= 256);
    if (input_size != 128)
        return {false, bytes{}};

    uint8_t _rx[128];
    uint8_t _ry[128];
    const auto r = evmone::crypto::bls::map_fp2_to_g2(_rx,
        _ry,
        &input[0]);

    if (!r)
        return {false, bytes{}};

    std::copy(_rx, _rx + 128, output.begin());
    std::copy(_ry, _ry + 128, output.begin() + 128);
    return {true, output};
}
