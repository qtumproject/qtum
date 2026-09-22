#pragma once

#include <libdevcore/Common.h>

namespace dev
{
namespace crypto
{

constexpr auto SINGLE_ENTRY_SIZE_MSM_G1 = (64 * 2 + 32);
constexpr auto SINGLE_ENTRY_SIZE_MSM_G2 = (128 * 2 + 32);
constexpr auto PAIR_SIZE_G1_G2 = (64 * 2 + 128 * 2);

std::pair<bool, bytes> add_G1_bls(bytesConstRef _in);

std::pair<bool, bytes> msm_G1_bls(bytesConstRef _in);

std::pair<bool, bytes> add_G2_bls(bytesConstRef _in);

std::pair<bool, bytes> msm_G2_bls(bytesConstRef _in);

std::pair<bool, bytes> pairing_check_bls(bytesConstRef _in);

std::pair<bool, bytes> map_fp_to_G1_bls(bytesConstRef _in);

std::pair<bool, bytes> map_fp2_to_G2_bls(bytesConstRef _in);

}
}
