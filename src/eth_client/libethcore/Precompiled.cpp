// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#include "Precompiled.h"
#include "ChainOperationParams.h"
#include <libdevcore/Log.h>
#include <libdevcore/SHA3.h>
#include <libdevcrypto/Blake2.h>
#include <libdevcrypto/Common.h>
#include <libdevcrypto/Hash.h>
#include <libdevcrypto/LibSnark.h>
#include <libdevcrypto/LibKzg.h>
#include <libdevcrypto/LibBls.h>
#include <libethcore/Common.h>
#include <qtum/qtumutils.h>
#include <algorithm>
using namespace std;
using namespace dev;
using namespace dev::eth;

PrecompiledRegistrar* PrecompiledRegistrar::s_this = nullptr;

PrecompiledExecutor const& PrecompiledRegistrar::executor(std::string const& _name)
{
    if (!get()->m_execs.count(_name))
        BOOST_THROW_EXCEPTION(ExecutorNotFound());
    return get()->m_execs[_name];
}

PrecompiledPricer const& PrecompiledRegistrar::pricer(std::string const& _name)
{
    if (!get()->m_pricers.count(_name))
        BOOST_THROW_EXCEPTION(PricerNotFound());
    return get()->m_pricers[_name];
}

namespace
{
bigint linearPricer(unsigned _base, unsigned _word, bytesConstRef _in)
{
    bigint const s = _in.size();
    bigint const b = _base;
    bigint const w = _word;
    return b + (s + 31) / 32 * w;
}

ETH_REGISTER_PRECOMPILED_PRICER(ecrecover)
(bytesConstRef /*_in*/, ChainOperationParams const& /*_chainParams*/, u256 const& /*_blockNumber*/)
{
    return 3000;
}

ETH_REGISTER_PRECOMPILED_PRICER(btc_ecrecover)
(bytesConstRef /*_in*/, ChainOperationParams const& /*_chainParams*/, u256 const& /*_blockNumber*/)
{
    return 3000;
}

ETH_REGISTER_PRECOMPILED(btc_ecrecover)(bytesConstRef _in)
{
    struct
    {
        h256 hash;
        h256 v;
        h256 r;
        h256 s;
    } in;

    memcpy(&in, _in.data(), min(_in.size(), sizeof(in)));

    h256 ret;
    try
    {
        bool recovered = false;
        u256 v = (u256)in.v;
        recovered = qtumutils::btc_ecrecover(in.hash, v, in.r, in.s, ret);
        if(recovered)
        {
            return {true, ret.asBytes()};
        }
    }
    catch (...) {}

    return {true, {}};
}

ETH_REGISTER_PRECOMPILED(ecrecover)(bytesConstRef _in)
{
    struct
    {
        h256 hash;
        h256 v;
        h256 r;
        h256 s;
    } in;

    memcpy(&in, _in.data(), min(_in.size(), sizeof(in)));

    h256 ret;
    u256 v = (u256)in.v;
    if (v >= 27 && v <= 28)
    {
        SignatureStruct sig(in.r, in.s, (dev::byte)((int)v - 27));
        if (sig.isValid())
        {
            try
            {
                if (Public rec = recover(sig, in.hash))
                {
                    ret = dev::sha3(rec);
                    memset(ret.data(), 0, 12);
                    return {true, ret.asBytes()};
                }
            }
            catch (...) {}
        }
    }
    return {true, {}};
}

ETH_REGISTER_PRECOMPILED_PRICER(sha256)
(bytesConstRef _in, ChainOperationParams const& /*_chainParams*/, u256 const& /*_blockNumber*/)
{
    return linearPricer(60, 12, _in);
}

ETH_REGISTER_PRECOMPILED(sha256)(bytesConstRef _in)
{
    return {true, dev::sha256(_in).asBytes()};
}

ETH_REGISTER_PRECOMPILED_PRICER(ripemd160)
(bytesConstRef _in, ChainOperationParams const& /*_chainParams*/, u256 const& /*_blockNumber*/)
{
    return linearPricer(600, 120, _in);
}

ETH_REGISTER_PRECOMPILED(ripemd160)(bytesConstRef _in)
{
    return {true, h256(dev::ripemd160(_in), h256::AlignRight).asBytes()};
}

ETH_REGISTER_PRECOMPILED_PRICER(identity)
(bytesConstRef _in, ChainOperationParams const& /*_chainParams*/, u256 const& /*_blockNumber*/)
{
    return linearPricer(15, 3, _in);
}

ETH_REGISTER_PRECOMPILED(identity)(bytesConstRef _in)
{
    return {true, _in.toBytes()};
}

// Parse _count bytes of _in starting with _begin offset as big endian int.
// If there's not enough bytes in _in, consider it infinitely right-padded with zeroes.
bigint parseBigEndianRightPadded(bytesConstRef _in, bigint const& _begin, bigint const& _count)
{
    if (_begin > _in.count())
        return 0;
    assert(_count <= numeric_limits<size_t>::max() / 8); // Otherwise, the return value would not fit in the memory.

    size_t const begin{_begin};
    size_t const count{_count};

    // crop _in, not going beyond its size
    bytesConstRef cropped = _in.cropped(begin, min(count, _in.count() - begin));

    bigint ret = fromBigEndian<bigint>(cropped);
    // shift as if we had right-padding zeroes
    assert(count - cropped.count() <= numeric_limits<size_t>::max() / 8);
    ret <<= 8 * (count - cropped.count());

    return ret;
}

ETH_REGISTER_PRECOMPILED(modexp)(bytesConstRef _in)
{
    bigint const baseLength(parseBigEndianRightPadded(_in, 0, 32));
    bigint const expLength(parseBigEndianRightPadded(_in, 32, 32));
    bigint const modLength(parseBigEndianRightPadded(_in, 64, 32));
    assert(modLength <= numeric_limits<size_t>::max() / 8); // Otherwise gas should be too expensive.
    assert(baseLength <= numeric_limits<size_t>::max() / 8); // Otherwise, gas should be too expensive.
    if (modLength == 0 && baseLength == 0)
        return {true, bytes{}}; // This is a special case where expLength can be very big.
    assert(expLength <= numeric_limits<size_t>::max() / 8);

    bigint const base(parseBigEndianRightPadded(_in, 96, baseLength));
    bigint const exp(parseBigEndianRightPadded(_in, 96 + baseLength, expLength));
    bigint const mod(parseBigEndianRightPadded(_in, 96 + baseLength + expLength, modLength));

    bigint const result = mod != 0 ? boost::multiprecision::powm(base, exp, mod) : bigint{0};

    size_t const retLength(modLength);
    bytes ret(retLength);
    toBigEndian(result, ret);

    return {true, ret};
}

namespace
{
    bigint expLengthAdjust(bigint const& _expOffset, bigint const& _expLength, bytesConstRef _in)
    {
        if (_expLength <= 32)
        {
            bigint const exp(parseBigEndianRightPadded(_in, _expOffset, _expLength));
            return exp ? msb(exp) : 0;
        }
        else
        {
            bigint const expFirstWord(parseBigEndianRightPadded(_in, _expOffset, 32));
            size_t const highestBit(expFirstWord ? msb(expFirstWord) : 0);
            return 8 * (_expLength - 32) + highestBit;
        }
    }

    bigint multComplexity(bigint const& _x)
    {
        if (_x <= 64)
            return _x * _x;
        if (_x <= 1024)
            return (_x * _x) / 4 + 96 * _x - 3072;
        else
            return (_x * _x) / 16 + 480 * _x - 199680;
    }
}

ETH_REGISTER_PRECOMPILED_PRICER(modexp)(bytesConstRef _in, ChainOperationParams const& _chainParams, u256 const& _blockNumber)
{
    bigint const baseLength(parseBigEndianRightPadded(_in, 0, 32));
    bigint const expLength(parseBigEndianRightPadded(_in, 32, 32));
    bigint const modLength(parseBigEndianRightPadded(_in, 64, 32));

    bigint const maxLength(max(modLength, baseLength));
    bigint const adjustedExpLength(expLengthAdjust(baseLength + 96, expLength, _in));

    bigint gas = maxLength;
    if(_blockNumber < _chainParams.berlinForkBlock)
    {
        gas = multComplexity(maxLength) * max<bigint>(adjustedExpLength, 1) / 20;
    }
    else
    {
        gas += 7;
        gas /= 8;
        gas *= gas;
        gas = gas * max<bigint>(adjustedExpLength, 1) / 3;
        gas = max<bigint>(200, gas);
    }

    return gas;
}

ETH_REGISTER_PRECOMPILED(alt_bn128_G1_add)(bytesConstRef _in)
{
    return dev::crypto::alt_bn128_G1_add(_in);
}

ETH_REGISTER_PRECOMPILED_PRICER(alt_bn128_G1_add)
(bytesConstRef /*_in*/, ChainOperationParams const& _chainParams, u256 const& _blockNumber)
{
    return _blockNumber < _chainParams.istanbulForkBlock ? 500 : 150;
}

ETH_REGISTER_PRECOMPILED(alt_bn128_G1_mul)(bytesConstRef _in)
{
    return dev::crypto::alt_bn128_G1_mul(_in);
}

ETH_REGISTER_PRECOMPILED_PRICER(alt_bn128_G1_mul)
(bytesConstRef /*_in*/, ChainOperationParams const& _chainParams, u256 const& _blockNumber)
{
    return _blockNumber < _chainParams.istanbulForkBlock ? 40000 : 6000;
}

ETH_REGISTER_PRECOMPILED(alt_bn128_pairing_product)(bytesConstRef _in)
{
    return dev::crypto::alt_bn128_pairing_product(_in);
}

ETH_REGISTER_PRECOMPILED_PRICER(alt_bn128_pairing_product)
(bytesConstRef _in, ChainOperationParams const& _chainParams, u256 const& _blockNumber)
{
    auto const k = _in.size() / 192;
    return _blockNumber < _chainParams.istanbulForkBlock ? 100000 + k * 80000 : 45000 + k * 34000;
}

ETH_REGISTER_PRECOMPILED(blake2_compression)(bytesConstRef _in)
{
    static constexpr size_t roundsSize = 4;
    static constexpr size_t stateVectorSize = 8 * 8;
    static constexpr size_t messageBlockSize = 16 * 8;
    static constexpr size_t offsetCounterSize = 8;
    static constexpr size_t finalBlockIndicatorSize = 1;
    static constexpr size_t totalInputSize = roundsSize + stateVectorSize + messageBlockSize +
                                             2 * offsetCounterSize + finalBlockIndicatorSize;

    if (_in.size() != totalInputSize)
        return {false, {}};

    auto const rounds = fromBigEndian<uint32_t>(_in.cropped(0, roundsSize));
    auto const stateVector = _in.cropped(roundsSize, stateVectorSize);
    auto const messageBlockVector = _in.cropped(roundsSize + stateVectorSize, messageBlockSize);
    auto const offsetCounter0 =
        _in.cropped(roundsSize + stateVectorSize + messageBlockSize, offsetCounterSize);
    auto const offsetCounter1 = _in.cropped(
        roundsSize + stateVectorSize + messageBlockSize + offsetCounterSize, offsetCounterSize);
    uint8_t const finalBlockIndicator =
        _in[roundsSize + stateVectorSize + messageBlockSize + 2 * offsetCounterSize];

    if (finalBlockIndicator != 0 && finalBlockIndicator != 1)
        return {false, {}};

    return {true, dev::crypto::blake2FCompression(rounds, stateVector, offsetCounter0,
                      offsetCounter1, finalBlockIndicator, messageBlockVector)};
}

ETH_REGISTER_PRECOMPILED_PRICER(blake2_compression)
(bytesConstRef _in, ChainOperationParams const&, u256 const&)
{
    auto const rounds = fromBigEndian<uint32_t>(_in.cropped(0, 4));
    return rounds;
}

ETH_REGISTER_PRECOMPILED_PRICER(point_evaluation)
(bytesConstRef /*_in*/, ChainOperationParams const& /*_chainParams*/, u256 const& /*_blockNumber*/)
{
    return 50000;
}

ETH_REGISTER_PRECOMPILED(point_evaluation)(bytesConstRef _in)
{
    return dev::crypto::point_evaluation_execute(_in);
}

constexpr std::array<int, 129> G1MSM_DISCOUNT_TABLE = {
    0,
    1000, 949, 848, 797, 764, 750, 738, 728, 719, 712, 705, 698, 692, 687, 682, 677, 673, 669,
    665, 661, 658, 654, 651, 648, 645, 642, 640, 637, 635, 632, 630, 627, 625, 623, 621, 619,
    617, 615, 613, 611, 609, 608, 606, 604, 603, 601, 599, 598, 596, 595, 593, 592, 591, 589,
    588, 586, 585, 584, 582, 581, 580, 579, 577, 576, 575, 574, 573, 572, 570, 569, 568, 567,
    566, 565, 564, 563, 562, 561, 560, 559, 558, 557, 556, 555, 554, 553, 552, 551, 550, 549,
    548, 547, 547, 546, 545, 544, 543, 542, 541, 540, 540, 539, 538, 537, 536, 536, 535, 534,
    533, 532, 532, 531, 530, 529, 528, 528, 527, 526, 525, 525, 524, 523, 522, 522, 521, 520,
    520, 519
};

constexpr std::array<int, 129> G2MSM_DISCOUNT_TABLE = {
    0,
    1000, 1000, 923, 884, 855, 832, 812, 796, 782, 770, 759, 749, 740, 732, 724, 717, 711, 704,
    699, 693, 688, 683, 679, 674, 670, 666, 663, 659, 655, 652, 649, 646, 643, 640, 637, 634,
    632, 629, 627, 624, 622, 620, 618, 615, 613, 611, 609, 607, 606, 604, 602, 600, 598, 597,
    595, 593, 592, 590, 589, 587, 586, 584, 583, 582, 580, 579, 578, 576, 575, 574, 573, 571,
    570, 569, 568, 567, 566, 565, 563, 562, 561, 560, 559, 558, 557, 556, 555, 554, 553, 552,
    552, 551, 550, 549, 548, 547, 546, 545, 545, 544, 543, 542, 541, 541, 540, 539, 538, 537,
    537, 536, 535, 535, 534, 533, 532, 532, 531, 530, 530, 529, 528, 528, 527, 526, 526, 525,
    524, 524
};

int msm_discount(bool bIsG1Curve, size_t k) {
    size_t index = std::min(k, (size_t)128);
    return bIsG1Curve ? G1MSM_DISCOUNT_TABLE[index] : G2MSM_DISCOUNT_TABLE[index];
}

ETH_REGISTER_PRECOMPILED_PRICER(add_G1_bls)
(bytesConstRef /*_in*/, ChainOperationParams const& /*_chainParams*/, u256 const& /*_blockNumber*/)
{
    return 375;
}

ETH_REGISTER_PRECOMPILED(add_G1_bls)(bytesConstRef _in)
{
    return dev::crypto::add_G1_bls(_in);
}

ETH_REGISTER_PRECOMPILED_PRICER(msm_G1_bls)
(bytesConstRef _in, ChainOperationParams const& /*_chainParams*/, u256 const& /*_blockNumber*/)
{
    size_t k = _in.size() / dev::crypto::SINGLE_ENTRY_SIZE_MSM_G1;
    if (k == 0) {
        return 0;
    }

    return k * 12000 * msm_discount(true, k) / 1000;
}

ETH_REGISTER_PRECOMPILED(msm_G1_bls)(bytesConstRef _in)
{
    return dev::crypto::msm_G1_bls(_in);
}

ETH_REGISTER_PRECOMPILED_PRICER(add_G2_bls)
(bytesConstRef /*_in*/, ChainOperationParams const& /*_chainParams*/, u256 const& /*_blockNumber*/)
{
    return 600;
}

ETH_REGISTER_PRECOMPILED(add_G2_bls)(bytesConstRef _in)
{
    return dev::crypto::add_G2_bls(_in);
}

ETH_REGISTER_PRECOMPILED_PRICER(msm_G2_bls)
(bytesConstRef _in, ChainOperationParams const& /*_chainParams*/, u256 const& /*_blockNumber*/)
{
    size_t k = _in.size() / dev::crypto::SINGLE_ENTRY_SIZE_MSM_G2;
    if (k == 0) {
        return 0;
    }

    return k * 22500 * msm_discount(false, k) / 1000;
}

ETH_REGISTER_PRECOMPILED(msm_G2_bls)(bytesConstRef _in)
{
    return dev::crypto::msm_G2_bls(_in);
}

ETH_REGISTER_PRECOMPILED_PRICER(pairing_check_bls)
(bytesConstRef _in, ChainOperationParams const& /*_chainParams*/, u256 const& /*_blockNumber*/)
{
    size_t k = _in.size() / dev::crypto::PAIR_SIZE_G1_G2;
    return 32600 * k + 37700;
}

ETH_REGISTER_PRECOMPILED(pairing_check_bls)(bytesConstRef _in)
{
    return dev::crypto::pairing_check_bls(_in);
}

ETH_REGISTER_PRECOMPILED_PRICER(map_fp_to_G1_bls)
(bytesConstRef /*_in*/, ChainOperationParams const& /*_chainParams*/, u256 const& /*_blockNumber*/)
{
    return 5500;
}

ETH_REGISTER_PRECOMPILED(map_fp_to_G1_bls)(bytesConstRef _in)
{
    return dev::crypto::map_fp_to_G1_bls(_in);
}

ETH_REGISTER_PRECOMPILED_PRICER(map_fp2_to_G2_bls)
(bytesConstRef /*_in*/, ChainOperationParams const& /*_chainParams*/, u256 const& /*_blockNumber*/)
{
    return 23800;
}

ETH_REGISTER_PRECOMPILED(map_fp2_to_G2_bls)(bytesConstRef _in)
{
    return dev::crypto::map_fp2_to_G2_bls(_in);
}

ETH_REGISTER_PRECOMPILED_PRICER(historical_hashes)
(bytesConstRef /*_in*/, ChainOperationParams const& /*_chainParams*/, u256 const& /*_blockNumber*/)
{
    return 4725;
}

ETH_REGISTER_PRECOMPILED(historical_hashes)(bytesConstRef input)
{
    size_t input_size = input.size();
    if (input_size != 32)
        return {false, bytes{}};

    try 
    {
        h256 in;
        memcpy(&in, input.data(), input_size);
        u256 blockNumber = (u256) in;
        h256 hash;
        if (qtumutils::HistoricalHashes::instance().get(blockNumber, hash))
            return {true, hash.asBytes()};
    }
    catch (...) {}
    return {false, {}};
}
}
