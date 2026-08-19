#include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h>
#include <validation.h>
#include <chainparams.h>
#include <test/qtumtests/precompiled_utils.h>
#include <test/qtumtests/data/secp256r1_k_and_s.json.h>
#include <test/qtumtests/data/secp256r1_modified_r_s.json.h>
#include <test/qtumtests/data/secp256r1_public_key.json.h>
#include <test/qtumtests/data/secp256r1_shamir_multiplication.json.h>
#include <test/qtumtests/data/secp256r1_signature_specific.json.h>
#include <test/qtumtests/data/secp256r1_small_large_r_s.json.h>
#include <test/qtumtests/data/secp256r1_special_case_hash.json.h>
#include <test/qtumtests/data/secp256r1_special_case_r_s.json.h>
#include <test/qtumtests/data/secp256r1_special_points.json.h>
#include <test/qtumtests/data/secp256r1_u1_u2.json.h>

#include <span>

// Tests for EIP-7951
namespace Secp256r1Test{

BOOST_FIXTURE_TEST_SUITE(secp256r1_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(checking_p256verify){
    // Call p256verify 0x100
    dev::eth::ChainOperationParams const& params = globalSealEngine->chainParams();
    dev::u256 blockNumber = 0;
    {
        LOCK(::cs_main);
        blockNumber = m_node.chainman->ActiveChain().Tip()->nHeight;
    }
    RunPrecompiledTests(p256verify, secp256r1_k_and_s, params, blockNumber);
    RunPrecompiledTests(p256verify, secp256r1_modified_r_s, params, blockNumber);
    RunPrecompiledTests(p256verify, secp256r1_public_key, params, blockNumber);
    RunPrecompiledTests(p256verify, secp256r1_shamir_multiplication, params, blockNumber);
    RunPrecompiledTests(p256verify, secp256r1_signature_specific, params, blockNumber);
    RunPrecompiledTests(p256verify, secp256r1_small_large_r_s, params, blockNumber);
    RunPrecompiledTests(p256verify, secp256r1_special_case_hash, params, blockNumber);
    RunPrecompiledTests(p256verify, secp256r1_special_case_r_s, params, blockNumber);
    RunPrecompiledTests(p256verify, secp256r1_special_points, params, blockNumber);
    RunPrecompiledTests(p256verify, secp256r1_u1_u2, params, blockNumber);
}

BOOST_AUTO_TEST_SUITE_END()

}
