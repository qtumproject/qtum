#include <libdevcrypto/LibSecp256r1.h>
#include <evmone_precompiles/secp256r1.hpp>

#include <cstring>

using namespace std;
using namespace dev;
using namespace dev::crypto;

pair<bool, bytes> dev::crypto::p256verify(dev::bytesConstRef input)
{
    bytes output(32, 0);
    size_t output_size = output.size();
    size_t input_size = input.size();
    assert(output_size >= 32);
    if (input_size != 160)
        return {false, bytes{}};

    ethash::hash256 h;
    std::memcpy(h.bytes, reinterpret_cast<const uint8_t*>(&input[0]), 32);
    intx::uint256 r = intx::be::unsafe::load<intx::uint256>(reinterpret_cast<const uint8_t*>(&input[32]));
    intx::uint256 s = intx::be::unsafe::load<intx::uint256>(reinterpret_cast<const uint8_t*>(&input[64]));
    intx::uint256 qx = intx::be::unsafe::load<intx::uint256>(reinterpret_cast<const uint8_t*>(&input[96]));
    intx::uint256 qy = intx::be::unsafe::load<intx::uint256>(reinterpret_cast<const uint8_t*>(&input[128]));

    if (!evmmax::secp256r1::verify(h, r, s, qx, qy))
        return {false, bytes{}};

    output[31] = 1;
    return {true, output};
}
