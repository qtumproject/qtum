#include <libdevcrypto/LibKzg.h>
#include <evmone_precompiles/kzg.hpp>

using namespace std;
using namespace dev;
using namespace dev::crypto;

pair<bool, bytes> dev::crypto::point_evaluation_execute(dev::bytesConstRef input)
{
    bytes output(64, 0);
    size_t output_size = output.size();
    size_t input_size = input.size();
    assert(output_size >= 64);
    if (input_size != 192)
        return {false, bytes{}};

    const auto r = evmone::crypto::kzg_verify_proof(reinterpret_cast<const std::byte*>(&input[0]),
        reinterpret_cast<const std::byte*>(&input[32]),
        reinterpret_cast<const std::byte*>(&input[64]),
        reinterpret_cast<const std::byte*>(&input[96]),
        reinterpret_cast<const std::byte*>(&input[96 + 48]));

    if (!r)
        return {false, bytes{}};

    // Return FIELD_ELEMENTS_PER_BLOB and BLS_MODULUS as padded 32 byte big endian values
    // as required by the EIP-4844.
    intx::be::unsafe::store(output.data(), evmone::crypto::FIELD_ELEMENTS_PER_BLOB);
    intx::be::unsafe::store(output.data() + 32, evmone::crypto::BLS_MODULUS);
        return {true, output};
}
