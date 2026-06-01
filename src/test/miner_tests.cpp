// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <addresstype.h>
#include <coins.h>
#include <common/system.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <consensus/tx_verify.h>
#include <interfaces/mining.h>
#include <node/miner.h>
#include <policy/policy.h>
#include <test/util/random.h>
#include <test/util/transaction_utils.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/check.h>
#include <util/feefrac.h>
#include <util/strencodings.h>
#include <util/time.h>
#include <util/translation.h>
#include <validation.h>
#include <versionbits.h>
#include <pow.h>

#include <test/util/common.h>
#include <test/util/setup_common.h>

#include <memory>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace util::hex_literals;
using interfaces::BlockTemplate;
using interfaces::Mining;
using node::BlockAssembler;

namespace miner_tests {
struct MinerTestingSetup : public TestingSetup {
    void TestPackageSelection(const CScript& scriptPubKey, const std::vector<CTransactionRef>& txFirst) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    void TestBasicMining(const CScript& scriptPubKey, const std::vector<CTransactionRef>& txFirst, int baseheight) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    void TestPrioritisedMining(const CScript& scriptPubKey, const std::vector<CTransactionRef>& txFirst) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool TestSequenceLocks(const CTransaction& tx, CTxMemPool& tx_mempool) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        CCoinsViewMemPool view_mempool{&m_node.chainman->ActiveChainstate().CoinsTip(), tx_mempool};
        CBlockIndex* tip{m_node.chainman->ActiveChain().Tip()};
        const std::optional<LockPoints> lock_points{CalculateLockPointsAtTip(tip, view_mempool, tx)};
        return lock_points.has_value() && CheckSequenceLocksAtTip(tip, *lock_points);
    }
    CTxMemPool& MakeMempool()
    {
        // Delete the previous mempool to ensure with valgrind that the old
        // pointer is not accessed, when the new one should be accessed
        // instead.
        m_node.mempool.reset();
        bilingual_str error;
        auto opts = MemPoolOptionsForTest(m_node);
        // The "block size > limit" test creates a cluster of 1192590 vbytes,
        // so set the cluster vbytes limit big enough so that the txgraph
        // doesn't become oversized.
        opts.limits.cluster_size_vbytes = 1'200'000;
        m_node.mempool = std::make_unique<CTxMemPool>(opts, error);
        Assert(error.empty());
        return *m_node.mempool;
    }
    std::unique_ptr<Mining> MakeMining()
    {
        return interfaces::MakeMining(m_node, /*wait_loaded=*/false);
    }
};
} // namespace miner_tests

BOOST_FIXTURE_TEST_SUITE(miner_tests, MinerTestingSetup)

static CFeeRate blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);

constexpr static struct {
    unsigned int extranonce;
    unsigned int nonce;
} BLOCKINFO[]{{4, 0x000192a0}, {2, 0x0001d2b6}, {1, 0x0002f5d8}, {1, 0x0004f913},
              {2, 0x0004fa6f}, {2, 0x0004ae04}, {1, 0x00048ec1}, {2, 0x00066185},
              {2, 0x0007a24b}, {1, 0x0009caef}, {1, 0x000e5bc4}, {2, 0x000ba124},
              {2, 0x000bac26}, {1, 0x000d2a68}, {2, 0x000bfd73}, {2, 0x000ef319},
              {1, 0x00119496}, {2, 0x000e758e}, {1, 0x0010182e}, {1, 0x000f08e8},
              {3, 0x0010654b}, {2, 0x00127fae}, {2, 0x0011ca28}, {1, 0x00147744},
              {2, 0x0015b085}, {1, 0x0015503f}, {2, 0x00170ad6}, {2, 0x001727c8},
              {2, 0x0017cb72}, {2, 0x00192067}, {2, 0x001b3fe9}, {2, 0x001c8cb1},
              {1, 0x001ab854}, {2, 0x001ec350}, {2, 0x001be586}, {1, 0x00202a9b},
              {2, 0x0020e78e}, {1, 0x002002b1}, {2, 0x0022c2e3}, {1, 0x0025097a},
              {1, 0x00287536}, {3, 0x002ac9ae}, {2, 0x002899e4}, {5, 0x0028ed8e},
              {1, 0x002c80bd}, {5, 0x002b5bc8}, {1, 0x0029e46c}, {1, 0x002c01c3},
              {1, 0x002bfe45}, {2, 0x002dd7bc}, {1, 0x002ee460}, {1, 0x0030cf33},
              {1, 0x0031a9c2}, {1, 0x00320de8}, {5, 0x00353e3b}, {5, 0x0033055f},
              {1, 0x00375485}, {1, 0x0037338c}, {6, 0x003be03f}, {2, 0x003c449d},
              {2, 0x003e1e27}, {1, 0x003c0fac}, {1, 0x003ca16d}, {1, 0x003f011d},
              {2, 0x003e5502}, {2, 0x003f132b}, {1, 0x00405658}, {1, 0x00411d00},
              {1, 0x00440de8}, {5, 0x004b3e82}, {5, 0x004c6672}, {1, 0x004d614b},
              {1, 0x004fa1a0}, {2, 0x004ef387}, {2, 0x004f91d9}, {1, 0x0050acb8},
              {2, 0x00512da7}, {1, 0x004f841b}, {2, 0x0050ffe0}, {2, 0x0052ae46},
              {1, 0x0054e0ea}, {1, 0x00554ffd}, {1, 0x0056a556}, {5, 0x0059fbf3},
              {1, 0x0059eada}, {1, 0x005b7715}, {1, 0x005c16c4}, {1, 0x005ec266},
              {1, 0x005f5a1e}, {1, 0x006109b5}, {1, 0x005f03e5}, {2, 0x00618000},
              {0, 0x006145d9}, {1, 0x00646931}, {2, 0x0062b04c}, {2, 0x00652d0a},
              {2, 0x0065f715}, {1, 0x0066f8ed}, {1, 0x006af32c}, {1, 0x006b7712},
              {1, 0x006c3e31}, {1, 0x006dc158}, {1, 0x006e71f9}, {5, 0x0075c9f1},
              {2, 0x006e10d6}, {1, 0x006ef321}, {1, 0x00717d82}, {1, 0x00718844},
              {2, 0x0071ea80}, {2, 0x00757c69}, {4, 0x0077c818}, {2, 0x0076d089},
              {1, 0x0078fcfc}, {1, 0x00799d31}, {2, 0x0079f533}, {2, 0x007dc2b3},
              {1, 0x007b038f}, {2, 0x007b9916}, {2, 0x007b467a}, {1, 0x007daab6},
              {1, 0x007c8fab}, {2, 0x0081e07b}, {2, 0x007f8d7e}, {1, 0x00818ac1},
              {2, 0x00805b08}, {2, 0x008361bd}, {1, 0x00833f6a}, {2, 0x0087e1c7},
              {1, 0x0084914a}, {1, 0x008454ca}, {3, 0x008471e3}, {2, 0x008a08e1},
              {2, 0x008a4916}, {1, 0x008e745d}, {2, 0x008bd1aa}, {1, 0x00908735},
              {2, 0x008e9827}, {2, 0x008e807d}, {2, 0x008ecdd9}, {2, 0x0092a382},
              {2, 0x0095dcd4}, {2, 0x0094a26b}, {1, 0x00960fd5}, {2, 0x0095acd7},
              {2, 0x00994437}, {1, 0x00994932}, {2, 0x0097bc78}, {1, 0x00990b2d},
              {2, 0x0097d28f}, {1, 0x009a58d6}, {1, 0x00998b73}, {3, 0x009b6591},
              {2, 0x009e742f}, {5, 0x009b5c55}, {1, 0x009aeeba}, {5, 0x009cc10a},
              {1, 0x009df73c}, {1, 0x009f0a8b}, {1, 0x00a02717}, {2, 0x00a36736},
              {1, 0x00a0cc93}, {1, 0x00a8a047}, {1, 0x00a1010e}, {1, 0x00a0e5ad},
              {5, 0x00a16d92}, {5, 0x00a1a77e}, {1, 0x00a7853c}, {1, 0x00a4ed9c},
              {6, 0x00a5d69a}, {2, 0x00a814c7}, {2, 0x00a745ba}, {1, 0x00ab6f3b},
              {1, 0x00ac6263}, {1, 0x00b082e4}, {2, 0x00b0635c}, {2, 0x00b263ff},
              {1, 0x00b27bd8}, {1, 0x00b2abed}, {1, 0x00b89e4f}, {5, 0x00b74fec},
              {5, 0x00b756ae}, {1, 0x00b7e68e}, {1, 0x00b7fd5c}, {2, 0x00bac23f},
              {2, 0x00bbfb82}, {1, 0x00c56007}, {2, 0x00c76ae4}, {1, 0x00c50dd8},
              {2, 0x00c4d443}, {2, 0x00c85e58}, {1, 0x00c8d53b}, {1, 0x00c82b08},
              {1, 0x00c74b7e}, {5, 0x00c85e50}, {1, 0x00ca49c5}, {1, 0x00ccb498},
              {1, 0x00cd86a3}, {1, 0x00ccea5d}, {1, 0x00d0875a}, {1, 0x00cd547f},
              {1, 0x00ce6ecf}, {2, 0x00cf83df}, {0, 0x00d0062f}, {1, 0x00d040c0},
              {2, 0x00d301da}, {2, 0x00d112f7}, {2, 0x00d03254}, {1, 0x00d2444d},
              {1, 0x00d3ad46}, {1, 0x00d57f40}, {1, 0x00d3f0f9}, {1, 0x00d55ae0},
              {1, 0x00d3df94}, {5, 0x00d4a6fb}, {2, 0x00d8f4e9}, {1, 0x00d87d1d},
              {1, 0x00dbbec9}, {1, 0x00d934bd}, {2, 0x00d8c3fc}, {2, 0x00dab1c9},
              {4, 0x00db2896}, {2, 0x00dbd401}, {1, 0x00db4495}, {1, 0x00e09dd6},
              {2, 0x00e06512}, {2, 0x00e1425f}, {1, 0x00e170fa}, {2, 0x00e3d5d7},
              {2, 0x00e40e77}, {1, 0x00e51f15}, {1, 0x00e5e594}, {2, 0x00e613ab},
              {2, 0x00e72e8b}, {1, 0x00e94959}, {2, 0x00e984d5}, {2, 0x00edabed},
              {1, 0x00edeacc}, {2, 0x00ef643d}, {1, 0x00f3c73a}, {1, 0x00f4a270},
              {3, 0x00f7a870}, {2, 0x00f7445f}, {2, 0x00fa971e}, {1, 0x00faf983},
              {2, 0x00ff21f7}, {1, 0x00fd32c3}, {2, 0x00fd4c9a}, {2, 0x00fe62e5},
              {2, 0x00ff9e4a}, {2, 0x01007cf7}, {2, 0x01013e61}, {2, 0x0100bda5},
              {1, 0x0103261d}, {2, 0x0103c540}, {2, 0x0104fdc9}, {1, 0x010995e0},
              {2, 0x0109d9e3}, {1, 0x010af516}, {2, 0x010b5d33}, {1, 0x010ded6b},
              {1, 0x0110c773}, {3, 0x010efe64}, {2, 0x011431b2}, {5, 0x01104d05},
              {1, 0x011315c8}, {5, 0x0113bb91}, {1, 0x0113198f}, {1, 0x01137c76},
              {1, 0x0116aa9d}, {2, 0x01175001}, {1, 0x0117f810}, {1, 0x011a1779},
              {1, 0x011e14d1}, {1, 0x011d4f8f}, {5, 0x011f9bc1}, {5, 0x011e8290},
              {1, 0x011e90a9}, {1, 0x01223dae}, {6, 0x012036d5}, {2, 0x0120a877},
              {2, 0x01241cad}, {1, 0x01278141}, {1, 0x01251df6}, {1, 0x01262723},
              {2, 0x01268f18}, {2, 0x01284da7}, {1, 0x0128d7f3}, {1, 0x012adf33},
              {1, 0x012b9c22}, {5, 0x012e0b7f}, {5, 0x012fd494}, {1, 0x0132de85},
              {1, 0x0132b6d4}, {2, 0x01342dbe}, {2, 0x01373e92}, {1, 0x0134c5e3},
              {2, 0x0136892c}, {1, 0x0137c63e}, {2, 0x01375ee3}, {2, 0x0138f36e},
              {1, 0x01382ca5}, {1, 0x013a4c1e}, {1, 0x013b70a7}, {5, 0x013d4996},
              {1, 0x013c782e}, {1, 0x013d68a4}, {1, 0x0140e7f0}, {1, 0x013f9932},
              {1, 0x014103fb}, {1, 0x0140f854}, {1, 0x014316b1}, {2, 0x01402ca5},
              {0, 0x01469369}, {1, 0x01454ac4}, {2, 0x01448a10}, {2, 0x0147c1ad},
              {2, 0x014627d3}, {1, 0x0146d4d4}, {1, 0x014aefad}, {1, 0x014c3d89},
              {1, 0x014f4301}, {1, 0x014c9c70}, {1, 0x014dde03}, {5, 0x01509431},
              {2, 0x014fefe7}, {1, 0x01504013}, {1, 0x0151438d}, {1, 0x0150bc00},
              {2, 0x0153a5e8}, {2, 0x0152dfad}, {4, 0x0153f3b5}, {2, 0x01542e7a},
              {1, 0x0153bc11}, {1, 0x01550bbc}, {2, 0x015701d6}, {2, 0x01579563},
              {1, 0x0159f464}, {2, 0x0158fc62}, {2, 0x015a4802}, {1, 0x015e8244},
              {1, 0x015e2879}, {2, 0x015e6325}, {2, 0x01610bd3}, {1, 0x015f0c9f},
              {2, 0x01606fc7}, {2, 0x0160f496}, {1, 0x015ff278}, {2, 0x01613a74},
              {1, 0x0160a518}, {1, 0x016199a4}, {3, 0x0162871a}, {2, 0x0162f885},
              {2, 0x01660682}, {1, 0x01678351}, {2, 0x016a0e0d}, {1, 0x0168b17e},
              {2, 0x01676f8e}, {2, 0x016b33ce}, {2, 0x016e2eaa}, {2, 0x016b746a},
              {2, 0x016c8e68}, {2, 0x01715a2e}, {1, 0x016e252f}, {2, 0x016eaa18},
              {2, 0x01728ea2}, {1, 0x01700da6}, {2, 0x017385dd}, {1, 0x01719d85},
              {2, 0x01740acb}, {1, 0x017481a8}, {1, 0x01748109}, {3, 0x01746b21},
              {2, 0x0178f8e8}, {5, 0x01779d9d}, {1, 0x017b20ec}, {5, 0x017aec24},
              {1, 0x017b4b5b}, {1, 0x017a50b4}, {1, 0x017a2936}, {2, 0x017c5287},
              {1, 0x017ba847}, {1, 0x017cd3d5}, {1, 0x017ff0ba}, {1, 0x0180ddef},
              {5, 0x01802207}, {5, 0x01825bf9}, {1, 0x017ebfbe}, {1, 0x017f3ebf},
              {6, 0x0180b152}, {2, 0x0180b413}, {2, 0x01821943}, {1, 0x01825a66},
              {1, 0x01817892}, {1, 0x01861a68}, {2, 0x0186518c}, {2, 0x01877ad5},
              {1, 0x01859932}, {1, 0x0186bc6d}, {1, 0x0188cf11}, {5, 0x0188a3d0},
              {5, 0x018aec05}, {1, 0x0189a5c3}, {1, 0x018bd1d3}, {2, 0x018ee02e},
              {2, 0x018f0dae}, {1, 0x0190235b}, {2, 0x018e996d}, {1, 0x018ec441},
              {2, 0x0193c677}, {2, 0x01922dae}, {1, 0x019247cc}, {1, 0x0193b2ec},
              {1, 0x0195b268}, {5, 0x019488ab}, {1, 0x0196b031}, {1, 0x0198de72},
              {1, 0x0199217b}, {1, 0x019c80e9}, {1, 0x01995d5d}, {1, 0x0199fce9},
              {1, 0x019b54e9}, {2, 0x019b94c6}, {0, 0x019e67bd}, {1, 0x019cc367},
              {2, 0x01a00b0b}, {2, 0x01a1d594}, {2, 0x01a0ca35}, {1, 0x01a2625e},
              {1, 0x01a2fdd6}, {1, 0x01a41f99}, {1, 0x01a444b1}, {1, 0x01a36856},
              {1, 0x01a71811}, {5, 0x01a547c9}, {2, 0x01a4a272}, {1, 0x01a65fd5},
              {1, 0x01a8889f}, {1, 0x01a82a76}, {2, 0x01a8bbda}, {2, 0x01aabc2e},
              {4, 0x01a8b88b}, {2, 0x01aaf146}, {1, 0x01ab570f}, {1, 0x01ac3501},
              {2, 0x01af258d}, {2, 0x01afd5ed}, {1, 0x01af043a}, {2, 0x01afa9c6},
              {2, 0x01b28d7c}, {1, 0x01b26b23}, {1, 0x01b2fe10}, {2, 0x01b250f5},
              {2, 0x01b68203}, {1, 0x01b48e23}, {2, 0x01b5da66}, {2, 0x01b6e2e0},
              {1, 0x01b5ff6f}, {2, 0x01b844b8}, {1, 0x01b6bc35}, {1, 0x01b8cc17},
              {3, 0x01b7f17e}, {2, 0x01ba6d0c}, {2, 0x01b97523}, {1, 0x01bb55d6},
              {2, 0x01bf0d53}, {1, 0x01bf38e0}, {2, 0x01bedd54}, {2, 0x01beeca7},
              {2, 0x01bf80f7}, {2, 0x01c08d05}, {2, 0x01c0b079}, {2, 0x01c385c5},
              {1, 0x01c43c80}, {2, 0x01c99c9d}, {2, 0x01c76aea}, {1, 0x01c8bafd},
              {2, 0x01c8c4dd}, {1, 0x01c90411}, {2, 0x01c96625}, {1, 0x01cb1f7f},
              {1, 0x01cb9ed2}, {3, 0x01d0ccba}, {2, 0x01d2c8f5}, {5, 0x01d14b5d},
              {1, 0x01d5711f}, {5, 0x01d5583d}, {1, 0x01d69aa1}, {1, 0x01d95c50},
              {1, 0x01dad422}, {2, 0x01dba4b1}, {1, 0x01dfbab5}, {1, 0x01e072d4},
              {1, 0x01e2d3da}, {1, 0x01e24219}, {5, 0x01e55cd8}, {5, 0x01e6f4bf},
              {1, 0x01eb82c5}, {1, 0x01e996af}, {6, 0x01e962ed}, {2, 0x01e93952},
              {2, 0x01eac2c9}, {1, 0x01ebd83a}, {1, 0x01ec1157}, {1, 0x01ec69b9},
              {2, 0x01ec9223}, {2, 0x01ee56da}, {1, 0x01ef3ee8}, {1, 0x01ef6196},
              {1, 0x01ef57a0}, {5, 0x01efe7e1}, {5, 0x01f44682}, {1, 0x01f4c9f2},
              {1, 0x01f525f4}, {2, 0x01f869eb}, {2, 0x01f71e9c}, {1, 0x01f78e49},
              {2, 0x01f73a2f}, {1, 0x01f9f058}, {2, 0x01ff69ca}, {2, 0x01fe3151},
              {1, 0x02004981}, {1, 0x02042a38}, {1, 0x0205a325}, {5, 0x0205e007},
              {1, 0x020680ec}, {1, 0x020b5d90}, {1, 0x0208c36d}, {1, 0x020b8e3f},
              {1, 0x0209ce05}, {1, 0x0209ef22}, {1, 0x020c30a7}, {2, 0x020e9bd9},
              {0, 0x020ff9e1}, {1, 0x021309b0}, {2, 0x02143276}, {2, 0x0214db7a},
              {2, 0x021a4811}, {1, 0x021bdc6d}, {1, 0x021b0a68}, {1, 0x021ae270},
              {1, 0x021c61ac}, {1, 0x021cef7f}, {1, 0x02203dbf}, {5, 0x022264e3},
              {2, 0x0221597c}, {1, 0x02228469}, {1, 0x02261db3}, {1, 0x02268128},
              {2, 0x0226a042}, {2, 0x022841c4}};

static std::unique_ptr<CBlockIndex> CreateBlockIndex(int nHeight, CBlockIndex* active_chain_tip) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    auto index{std::make_unique<CBlockIndex>()};
    index->nHeight = nHeight;
    index->pprev = active_chain_tip;
    return index;
}

// Test suite for ancestor feerate transaction selection.
// Implemented as an additional function, rather than a separate test case,
// to allow reusing the blockchain created in CreateNewBlock_validity.
void MinerTestingSetup::TestPackageSelection(const CScript& scriptPubKey, const std::vector<CTransactionRef>& txFirst)
{
    CTxMemPool& tx_mempool{MakeMempool()};
    auto mining{MakeMining()};
    BlockAssembler::Options options;
    options.coinbase_output_script = scriptPubKey;
    options.include_dummy_extranonce = true;

    LOCK(tx_mempool.cs);
    BOOST_CHECK(tx_mempool.size() == 0);

    // Block template should only have a coinbase when there's nothing in the mempool
    std::unique_ptr<BlockTemplate> block_template = mining->createNewBlock(options, /*cooldown=*/false);
    BOOST_REQUIRE(block_template);
    CBlock block{block_template->getBlock()};
    BOOST_REQUIRE_EQUAL(block.vtx.size(), 1U);

    // waitNext() on an empty mempool should return nullptr because there is no better template
    auto should_be_nullptr = block_template->waitNext({.timeout = MillisecondsDouble{0}, .fee_threshold = 1});
    BOOST_REQUIRE(should_be_nullptr == nullptr);

    // Unless fee_threshold is 0
    block_template = block_template->waitNext({.timeout = MillisecondsDouble{0}, .fee_threshold = 0});
    BOOST_REQUIRE(block_template);

    // Test the ancestor feerate transaction selection.
    TestMemPoolEntryHelper entry;

    // Test that a medium fee transaction will be selected after a higher fee
    // rate package with a low fee rate parent.
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vout.resize(1);
    tx.vout[0].nValue = 5000000000LL - 400000;
    // This tx has a low fee: 1000 satoshis
    Txid hashParentTx = tx.GetHash(); // save this txid for later use
    const auto parent_tx{entry.Fee(400000).Time(Now<NodeSeconds>()).SpendsCoinbase(true).FromTx(tx)};
    TryAddToMempool(tx_mempool, parent_tx);

    // This tx has a medium fee: 10000 satoshis
    tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    tx.vout[0].nValue = 5000000000LL - 4000000;
    Txid hashMediumFeeTx = tx.GetHash();
    const auto medium_fee_tx{entry.Fee(4000000).Time(Now<NodeSeconds>()).SpendsCoinbase(true).FromTx(tx)};
    TryAddToMempool(tx_mempool, medium_fee_tx);

    // This tx has a high fee, but depends on the first transaction
    tx.vin[0].prevout.hash = hashParentTx;
    tx.vout[0].nValue = 5000000000LL - 10000 - 400000 * 50; // 50k satoshi fee
    Txid hashHighFeeTx = tx.GetHash();
    const auto high_fee_tx{entry.Fee(400000 * 50).Time(Now<NodeSeconds>()).SpendsCoinbase(false).FromTx(tx)};
    TryAddToMempool(tx_mempool, high_fee_tx);

    block_template = mining->createNewBlock(options, /*cooldown=*/false);
    BOOST_REQUIRE(block_template);
    block = block_template->getBlock();
    BOOST_REQUIRE_EQUAL(block.vtx.size(), 4U);
    BOOST_CHECK(block.vtx[1]->GetHash() == hashParentTx);
    BOOST_CHECK(block.vtx[2]->GetHash() == hashHighFeeTx);
    BOOST_CHECK(block.vtx[3]->GetHash() == hashMediumFeeTx);

    // Test the inclusion of package feerates in the block template and ensure they are sequential.
    const auto block_package_feerates = BlockAssembler{m_node.chainman->ActiveChainstate(), &tx_mempool, options}.CreateNewBlock()->m_package_feerates;
    BOOST_CHECK(block_package_feerates.size() == 2);

    // parent_tx and high_fee_tx are added to the block as a package.
    const auto combined_txs_fee = parent_tx.GetFee() + high_fee_tx.GetFee();
    const auto combined_txs_size = parent_tx.GetTxSize() + high_fee_tx.GetTxSize();
    FeeFrac package_feefrac{combined_txs_fee, combined_txs_size};
    // The package should be added first.
    BOOST_CHECK(block_package_feerates[0] == package_feefrac);

    // The medium_fee_tx should be added next.
    FeeFrac medium_tx_feefrac{medium_fee_tx.GetFee(), medium_fee_tx.GetTxSize()};
    BOOST_CHECK(block_package_feerates[1] == medium_tx_feefrac);

    // Test that a package below the block min tx fee doesn't get included
    tx.vin[0].prevout.hash = hashHighFeeTx;
    tx.vout[0].nValue = 5000000000LL - 10000 - 20000000; // 0 fee
    Txid hashFreeTx = tx.GetHash();
    TryAddToMempool(tx_mempool, entry.Fee(0).FromTx(tx));
    uint64_t freeTxSize{::GetSerializeSize(TX_WITH_WITNESS(tx))};

    // Calculate a fee on child transaction that will put the package just
    // below the block min tx fee (assuming 1 child tx of the same size).
    CAmount feeToUse = blockMinFeeRate.GetFee(2*freeTxSize) - 1;

    tx.vin[0].prevout.hash = hashFreeTx;
    tx.vout[0].nValue = 5000000000LL - 10000 - 20000000 - feeToUse;
    Txid hashLowFeeTx = tx.GetHash();
    TryAddToMempool(tx_mempool, entry.Fee(feeToUse).FromTx(tx));

    // waitNext() should return nullptr because there is no better template
    should_be_nullptr = block_template->waitNext({.timeout = MillisecondsDouble{0}, .fee_threshold = 1});
    BOOST_REQUIRE(should_be_nullptr == nullptr);

    block = block_template->getBlock();
    // Verify that the free tx and the low fee tx didn't get selected
    for (size_t i=0; i<block.vtx.size(); ++i) {
        BOOST_CHECK(block.vtx[i]->GetHash() != hashFreeTx);
        BOOST_CHECK(block.vtx[i]->GetHash() != hashLowFeeTx);
    }

    // Test that packages above the min relay fee do get included, even if one
    // of the transactions is below the min relay fee
    // Remove the low fee transaction and replace with a higher fee transaction
    tx_mempool.removeRecursive(CTransaction(tx), MemPoolRemovalReason::REPLACED);
    tx.vout[0].nValue -= 2; // Now we should be just over the min relay fee
    hashLowFeeTx = tx.GetHash();
    TryAddToMempool(tx_mempool, entry.Fee(feeToUse + 2).FromTx(tx));

    // waitNext() should return if fees for the new template are at least 1 sat up
    block_template = block_template->waitNext({.fee_threshold = 1});
    BOOST_REQUIRE(block_template);
    block = block_template->getBlock();
    BOOST_REQUIRE_EQUAL(block.vtx.size(), 6U);
    BOOST_CHECK(block.vtx[4]->GetHash() == hashFreeTx);
    BOOST_CHECK(block.vtx[5]->GetHash() == hashLowFeeTx);

    // Test that transaction selection properly updates ancestor fee
    // calculations as ancestor transactions get included in a block.
    // Add a 0-fee transaction that has 2 outputs.
    tx.vin[0].prevout.hash = txFirst[2]->GetHash();
    tx.vout.resize(2);
    tx.vout[0].nValue = 5000000000LL - 100000000;
    tx.vout[1].nValue = 100000000; // 1BTC output
    Txid hashFreeTx2 = tx.GetHash();
    TryAddToMempool(tx_mempool, entry.Fee(0).SpendsCoinbase(true).FromTx(tx));

    // This tx can't be mined by itself
    tx.vin[0].prevout.hash = hashFreeTx2;
    tx.vout.resize(1);
    feeToUse = blockMinFeeRate.GetFee(freeTxSize);
    tx.vout[0].nValue = 5000000000LL - 100000000 - feeToUse;
    Txid hashLowFeeTx2 = tx.GetHash();
    TryAddToMempool(tx_mempool, entry.Fee(feeToUse).SpendsCoinbase(false).FromTx(tx));
    block_template = mining->createNewBlock(options, /*cooldown=*/false);
    BOOST_REQUIRE(block_template);
    block = block_template->getBlock();

    // Verify that this tx isn't selected.
    for (size_t i=0; i<block.vtx.size(); ++i) {
        BOOST_CHECK(block.vtx[i]->GetHash() != hashFreeTx2);
        BOOST_CHECK(block.vtx[i]->GetHash() != hashLowFeeTx2);
    }

    // This tx will be mineable, and should cause hashLowFeeTx2 to be selected
    // as well.
    tx.vin[0].prevout.n = 1;
    tx.vout[0].nValue = 100000000 - 4000000; // 10k satoshi fee
    TryAddToMempool(tx_mempool, entry.Fee(4000000).FromTx(tx));
    block_template = mining->createNewBlock(options, /*cooldown=*/false);
    BOOST_REQUIRE(block_template);
    block = block_template->getBlock();
    BOOST_REQUIRE_EQUAL(block.vtx.size(), 9U);
    BOOST_CHECK(block.vtx[8]->GetHash() == hashLowFeeTx2);
}

std::vector<CTransactionRef> CreateBigSigOpsCluster(const CTransactionRef& first_tx)
{
    std::vector<CTransactionRef> ret;

    CMutableTransaction tx;
    // block sigops > limit: 1000 CHECKMULTISIG + 1
    tx.vin.resize(1);
    // NOTE: OP_NOP is used to force 20 SigOps for the CHECKMULTISIG
    tx.vin[0].scriptSig = CScript() << OP_0 << OP_0 << OP_CHECKSIG << OP_1;
    tx.vin[0].prevout.hash = first_tx->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vout.resize(50);
    for (auto &out : tx.vout) {
        out.nValue = first_tx->vout[0].nValue / 50;
        out.scriptPubKey = CScript() << OP_1;
    }

    tx.vout[0].nValue -= CENT;
    CTransactionRef parent_tx = MakeTransactionRef(tx);
    ret.push_back(parent_tx);
    assert(GetLegacySigOpCount(*parent_tx) == 1);

    // Tx1 has 1 sigops, 1 input, 50 outputs.
    // Tx2-51 has 400 sigops: 1 input, 20 CHECKMULTISIG outputs
    // Total: 1000 CHECKMULTISIG + 1
    for (unsigned int i = 0; i < 50; ++i) {
        auto tx2 = tx;
        tx2.vin.resize(1);
        tx2.vin[0].prevout.hash = parent_tx->GetHash();
        tx2.vin[0].prevout.n = i;
        tx2.vin[0].scriptSig = CScript() << OP_1;
        tx2.vout.resize(20);
        tx2.vout[0].nValue = parent_tx->vout[i].nValue - CENT;
        for (auto &out : tx2.vout) {
            out.nValue = 0;
            out.scriptPubKey = CScript() << OP_0 << OP_0 << OP_0 << OP_NOP << OP_CHECKMULTISIG << OP_1;
        }
        ret.push_back(MakeTransactionRef(tx2));
    }
    return ret;
}

CAmount calculateReward(const CBlock& block, ChainstateManager& chainman){
    LOCK(cs_main);
    CAmount sumVout = 0, fee = 0;
    for(const CTransactionRef& t : block.vtx){
        fee += chainman.ActiveChainstate().CoinsTip().GetValueIn(*t);
        sumVout += t->GetValueOut();
    }
    return sumVout - fee;
}

void MinerTestingSetup::TestBasicMining(const CScript& scriptPubKey, const std::vector<CTransactionRef>& txFirst, int baseheight)
{
    Txid hash;
    CMutableTransaction tx;
    TestMemPoolEntryHelper entry;
    entry.nFee = 11;
    entry.nHeight = 11;
    const Consensus::Params& consensusParams = Params().GetConsensus();

    const CAmount BLOCKSUBSIDY = 50 * COIN;
    const CAmount LOWFEE = CENT;
    const CAmount HIGHFEE = COIN;
    const CAmount HIGHERFEE = 4 * COIN;

    auto mining{MakeMining()};
    BOOST_REQUIRE(mining);

    BlockAssembler::Options options;
    options.coinbase_output_script = scriptPubKey;
    options.include_dummy_extranonce = true;

    {
        CTxMemPool& tx_mempool{MakeMempool()};
        LOCK(tx_mempool.cs);

        // Just to make sure we can still make simple blocks
        auto block_template{mining->createNewBlock(options, /*cooldown=*/false)};
        BOOST_REQUIRE(block_template);
        CBlock block{block_template->getBlock()};

        auto txs = CreateBigSigOpsCluster(txFirst[0]);

        int64_t legacy_sigops = 0;
        for (auto& t : txs) {
            // If we don't set the number of sigops in the CTxMemPoolEntry,
            // template creation fails during sanity checks.
            TryAddToMempool(tx_mempool, entry.Fee(LOWFEE).Time(Now<NodeSeconds>()).SpendsCoinbase(true).FromTx(t));
            legacy_sigops += GetLegacySigOpCount(*t);
            BOOST_CHECK(tx_mempool.GetIter(t->GetHash()).has_value());
        }
        assert(tx_mempool.mapTx.size() == 51);
        assert(legacy_sigops == 20001);
        BOOST_CHECK_EXCEPTION(mining->createNewBlock(options, /*cooldown=*/false), std::runtime_error, HasReason("bad-blk-sigops"));
    }

    {
        CTxMemPool& tx_mempool{MakeMempool()};
        LOCK(tx_mempool.cs);

        // Check that the mempool is empty.
        assert(tx_mempool.mapTx.empty());

        // Just to make sure we can still make simple blocks
        auto block_template{mining->createNewBlock(options, /*cooldown=*/false)};
        BOOST_REQUIRE(block_template);
        CBlock block{block_template->getBlock()};

        auto txs = CreateBigSigOpsCluster(txFirst[0]);

        int64_t legacy_sigops = 0;
        for (auto& t : txs) {
            TryAddToMempool(tx_mempool, entry.Fee(LOWFEE).Time(Now<NodeSeconds>()).SpendsCoinbase(true).SigOpsCost(GetLegacySigOpCount(*t)*WITNESS_SCALE_FACTOR).FromTx(t));
            legacy_sigops += GetLegacySigOpCount(*t);
            BOOST_CHECK(tx_mempool.GetIter(t->GetHash()).has_value());
        }
        assert(tx_mempool.mapTx.size() == 51);
        assert(legacy_sigops == 20001);

        BOOST_REQUIRE(mining->createNewBlock(options, /*cooldown=*/false));
    }

    {
        CTxMemPool& tx_mempool{MakeMempool()};
        LOCK(tx_mempool.cs);

        // block size > limit
        tx.vin.resize(1);
        tx.vout.resize(1);
        tx.vout[0].nValue = BLOCKSUBSIDY;
        // 36 * (520char + DROP) + OP_1 = 18757 bytes
        std::vector<unsigned char> vchData(52);
        for (unsigned int i = 0; i < 18; ++i) {
            tx.vin[0].scriptSig << vchData << OP_DROP;
            tx.vout[0].scriptPubKey << vchData << OP_DROP;
        }
        tx.vin[0].scriptSig << OP_1;
        tx.vout[0].scriptPubKey << OP_1;
        tx.vin[0].prevout.hash = txFirst[0]->GetHash();
        tx.vin[0].prevout.n = 0;
        tx.vout[0].nValue = BLOCKSUBSIDY;
        for (unsigned int i = 0; i < 63; ++i) {
            tx.vout[0].nValue -= LOWFEE;
            hash = tx.GetHash();
            bool spendsCoinbase = i == 0; // only first tx spends coinbase
            TryAddToMempool(tx_mempool, entry.Fee(LOWFEE).Time(Now<NodeSeconds>()).SpendsCoinbase(spendsCoinbase).FromTx(tx));
            BOOST_CHECK(tx_mempool.GetIter(hash).has_value());
            tx.vin[0].prevout.hash = hash;
        }
        BOOST_REQUIRE(mining->createNewBlock(options, /*cooldown=*/false));
    }

    {
        CTxMemPool& tx_mempool{MakeMempool()};
        LOCK(tx_mempool.cs);

        // orphan in tx_mempool, template creation fails
        hash = tx.GetHash();
        TryAddToMempool(tx_mempool, entry.Fee(LOWFEE).Time(Now<NodeSeconds>()).FromTx(tx));
        BOOST_CHECK_EXCEPTION(mining->createNewBlock(options, /*cooldown=*/false), std::runtime_error, HasReason("bad-txns-inputs-missingorspent"));
    }

    {
        CTxMemPool& tx_mempool{MakeMempool()};
        LOCK(tx_mempool.cs);

        // child with higher feerate than parent
        tx.vin[0].scriptSig = CScript() << OP_1;
        tx.vin[0].prevout.hash = txFirst[1]->GetHash();
        tx.vout[0].nValue = BLOCKSUBSIDY - HIGHFEE;
        hash = tx.GetHash();
        TryAddToMempool(tx_mempool, entry.Fee(HIGHFEE).Time(Now<NodeSeconds>()).SpendsCoinbase(true).FromTx(tx));
        tx.vin[0].prevout.hash = hash;
        tx.vin.resize(2);
        tx.vin[1].scriptSig = CScript() << OP_1;
        tx.vin[1].prevout.hash = txFirst[0]->GetHash();
        tx.vin[1].prevout.n = 0;
        tx.vout[0].nValue = tx.vout[0].nValue + BLOCKSUBSIDY - HIGHERFEE; // First txn output + fresh coinbase - new txn fee
        hash = tx.GetHash();
        TryAddToMempool(tx_mempool, entry.Fee(HIGHERFEE).Time(Now<NodeSeconds>()).SpendsCoinbase(true).FromTx(tx));
        BOOST_REQUIRE(mining->createNewBlock(options, /*cooldown=*/false));
    }

    {
        CTxMemPool& tx_mempool{MakeMempool()};
        LOCK(tx_mempool.cs);

        // coinbase in tx_mempool, template creation fails
        tx.vin.resize(1);
        tx.vin[0].prevout.SetNull();
        tx.vin[0].scriptSig = CScript() << OP_0 << OP_1;
        tx.vout[0].nValue = 0;
        hash = tx.GetHash();
        // give it a fee so it'll get mined
        TryAddToMempool(tx_mempool, entry.Fee(LOWFEE).Time(Now<NodeSeconds>()).SpendsCoinbase(false).FromTx(tx));
        // Should throw bad-cb-multiple
        BOOST_CHECK_EXCEPTION(mining->createNewBlock(options, /*cooldown=*/false), std::runtime_error, HasReason("bad-cb-multiple"));
    }

    {
        CTxMemPool& tx_mempool{MakeMempool()};
        LOCK(tx_mempool.cs);

        // double spend txn pair in tx_mempool, template creation fails
        tx.vin[0].prevout.hash = txFirst[0]->GetHash();
        tx.vin[0].scriptSig = CScript() << OP_1;
        tx.vout[0].nValue = BLOCKSUBSIDY - HIGHFEE;
        tx.vout[0].scriptPubKey = CScript() << OP_1;
        hash = tx.GetHash();
        TryAddToMempool(tx_mempool, entry.Fee(HIGHFEE).Time(Now<NodeSeconds>()).SpendsCoinbase(true).FromTx(tx));
        tx.vout[0].scriptPubKey = CScript() << OP_2;
        hash = tx.GetHash();
        TryAddToMempool(tx_mempool, entry.Fee(HIGHFEE).Time(Now<NodeSeconds>()).SpendsCoinbase(true).FromTx(tx));
        BOOST_CHECK_EXCEPTION(mining->createNewBlock(options, /*cooldown=*/false), std::runtime_error, HasReason("bad-txns-inputs-missingorspent"));
    }

    {
        CTxMemPool& tx_mempool{MakeMempool()};
        LOCK(tx_mempool.cs);

        // subsidy changing
        int nHeight = m_node.chainman->ActiveChain().Height();
        // Create an actual 1427002-long block chain (without valid blocks).
        while (m_node.chainman->ActiveChain().Tip()->nHeight < 1427002) {
            CBlockIndex* prev = m_node.chainman->ActiveChain().Tip();
            CBlockIndex* next = new CBlockIndex();
            next->phashBlock = new uint256(m_rng.rand256());
            m_node.chainman->ActiveChainstate().CoinsTip().SetBestBlock(next->GetBlockHash());
            next->pprev = prev;
            next->nHeight = prev->nHeight + 1;
            next->BuildSkip();
            m_node.chainman->ActiveChain().SetTip(*next);
        }
        int blocktimeDownscaleFactor = consensusParams.BlocktimeDownscaleFactor(m_node.chainman->ActiveChain().Tip()->nHeight + 1);
        options.is_coinstake = true;
        auto pblocktemplate = mining->createNewBlock(options);
        BOOST_REQUIRE(pblocktemplate);
        BOOST_CHECK(calculateReward(pblocktemplate->getBlock(), *m_node.chainman) == 400000000/blocktimeDownscaleFactor);
        // Extend to a 1427004-long block chain.
        while (m_node.chainman->ActiveChain().Tip()->nHeight < 1427004) {
            CBlockIndex* prev = m_node.chainman->ActiveChain().Tip();
            CBlockIndex* next = new CBlockIndex();
            next->phashBlock = new uint256(m_rng.rand256());
            m_node.chainman->ActiveChainstate().CoinsTip().SetBestBlock(next->GetBlockHash());
            next->pprev = prev;
            next->nHeight = prev->nHeight + 1;
            next->BuildSkip();
            m_node.chainman->ActiveChain().SetTip(*next);
        }
        blocktimeDownscaleFactor = consensusParams.BlocktimeDownscaleFactor(m_node.chainman->ActiveChain().Tip()->nHeight + 1);
        pblocktemplate = mining->createNewBlock(options);
        options.is_coinstake = false;
        BOOST_REQUIRE(pblocktemplate);
        BOOST_CHECK(calculateReward(pblocktemplate->getBlock(), *m_node.chainman) == 200000000/blocktimeDownscaleFactor);

        // invalid p2sh txn in tx_mempool, template creation fails
        tx.vin[0].prevout.hash = txFirst[0]->GetHash();
        tx.vin[0].prevout.n = 0;
        tx.vin[0].scriptSig = CScript() << OP_1;
        tx.vout[0].nValue = BLOCKSUBSIDY - LOWFEE;
        CScript script = CScript() << OP_0;
        tx.vout[0].scriptPubKey = GetScriptForDestination(ScriptHash(script));
        hash = tx.GetHash();
        TryAddToMempool(tx_mempool, entry.Fee(LOWFEE).Time(Now<NodeSeconds>()).SpendsCoinbase(true).FromTx(tx));
        tx.vin[0].prevout.hash = hash;
        tx.vin[0].scriptSig = CScript() << std::vector<unsigned char>(script.begin(), script.end());
        tx.vout[0].nValue -= LOWFEE;
        hash = tx.GetHash();
        TryAddToMempool(tx_mempool, entry.Fee(LOWFEE).Time(Now<NodeSeconds>()).SpendsCoinbase(false).FromTx(tx));
        BOOST_CHECK_EXCEPTION(mining->createNewBlock(options, /*cooldown=*/false), std::runtime_error, HasReason("TestBlockValidity failed"));

        // Delete the dummy blocks again.
        while (m_node.chainman->ActiveChain().Tip()->nHeight > nHeight) {
            CBlockIndex* del = m_node.chainman->ActiveChain().Tip();
            m_node.chainman->ActiveChain().SetTip(*Assert(del->pprev));
            m_node.chainman->ActiveChainstate().CoinsTip().SetBestBlock(del->pprev->GetBlockHash());
            delete del->phashBlock;
            delete del;
        }
    }

    CTxMemPool& tx_mempool{MakeMempool()};
    LOCK(tx_mempool.cs);

    // non-final txs in mempool
    SetMockTime(m_node.chainman->ActiveChain().Tip()->GetMedianTimePast() + 1);
    const int flags{LOCKTIME_VERIFY_SEQUENCE};
    // height map
    std::vector<int> prevheights;

    // relative height locked
    tx.version = 2;
    tx.vin.resize(1);
    prevheights.resize(1);
    tx.vin[0].prevout.hash = txFirst[0]->GetHash(); // only 1 transaction
    tx.vin[0].prevout.n = 0;
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].nSequence = m_node.chainman->ActiveChain().Tip()->nHeight + 1; // txFirst[0] is the 2nd block
    prevheights[0] = baseheight + 1;
    tx.vout.resize(1);
    tx.vout[0].nValue = BLOCKSUBSIDY-HIGHFEE;
    tx.vout[0].scriptPubKey = CScript() << OP_1;
    tx.nLockTime = 0;
    hash = tx.GetHash();
    TryAddToMempool(tx_mempool, entry.Fee(HIGHFEE).Time(Now<NodeSeconds>()).SpendsCoinbase(true).FromTx(tx));
    BOOST_CHECK(CheckFinalTxAtTip(*Assert(m_node.chainman->ActiveChain().Tip()), CTransaction{tx})); // Locktime passes
    BOOST_CHECK(!TestSequenceLocks(CTransaction{tx}, tx_mempool)); // Sequence locks fail

    {
        CBlockIndex* active_chain_tip = m_node.chainman->ActiveChain().Tip();
        BOOST_CHECK(SequenceLocks(CTransaction(tx), flags, prevheights, *CreateBlockIndex(active_chain_tip->nHeight + 2, active_chain_tip))); // Sequence locks pass on 2nd block
    }

    // relative time locked
    tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    tx.vin[0].nSequence = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG | (((m_node.chainman->ActiveChain().Tip()->GetMedianTimePast()+1-m_node.chainman->ActiveChain()[1]->GetMedianTimePast()) >> CTxIn::SEQUENCE_LOCKTIME_GRANULARITY) + 1); // txFirst[1] is the 3rd block
    prevheights[0] = baseheight + 2;
    hash = tx.GetHash();
    TryAddToMempool(tx_mempool, entry.Time(Now<NodeSeconds>()).FromTx(tx));
    BOOST_CHECK(CheckFinalTxAtTip(*Assert(m_node.chainman->ActiveChain().Tip()), CTransaction{tx})); // Locktime passes
    BOOST_CHECK(!TestSequenceLocks(CTransaction{tx}, tx_mempool)); // Sequence locks fail

    const int SEQUENCE_LOCK_TIME = 512; // Sequence locks pass 512 seconds later
    for (int i = 0; i < CBlockIndex::nMedianTimeSpan; ++i)
        m_node.chainman->ActiveChain().Tip()->GetAncestor(m_node.chainman->ActiveChain().Tip()->nHeight - i)->nTime += SEQUENCE_LOCK_TIME; // Trick the MedianTimePast
    {
        CBlockIndex* active_chain_tip = m_node.chainman->ActiveChain().Tip();
        BOOST_CHECK(SequenceLocks(CTransaction(tx), flags, prevheights, *CreateBlockIndex(active_chain_tip->nHeight + 1, active_chain_tip)));
    }

    for (int i = 0; i < CBlockIndex::nMedianTimeSpan; ++i) {
        CBlockIndex* ancestor{Assert(m_node.chainman->ActiveChain().Tip()->GetAncestor(m_node.chainman->ActiveChain().Tip()->nHeight - i))};
        ancestor->nTime -= SEQUENCE_LOCK_TIME; // undo tricked MTP
    }

    // absolute height locked
    tx.vin[0].prevout.hash = txFirst[2]->GetHash();
    tx.vin[0].nSequence = CTxIn::MAX_SEQUENCE_NONFINAL;
    prevheights[0] = baseheight + 3;
    tx.nLockTime = m_node.chainman->ActiveChain().Tip()->nHeight + 1;
    hash = tx.GetHash();
    TryAddToMempool(tx_mempool, entry.Time(Now<NodeSeconds>()).FromTx(tx));
    BOOST_CHECK(!CheckFinalTxAtTip(*Assert(m_node.chainman->ActiveChain().Tip()), CTransaction{tx})); // Locktime fails
    BOOST_CHECK(TestSequenceLocks(CTransaction{tx}, tx_mempool)); // Sequence locks pass
    BOOST_CHECK(IsFinalTx(CTransaction(tx), m_node.chainman->ActiveChain().Tip()->nHeight + 2, m_node.chainman->ActiveChain().Tip()->GetMedianTimePast())); // Locktime passes on 2nd block

    // ensure tx is final for a specific case where there is no locktime and block height is zero
    tx.nLockTime = 0;
    BOOST_CHECK(IsFinalTx(CTransaction(tx), /*nBlockHeight=*/0, m_node.chainman->ActiveChain().Tip()->GetMedianTimePast()));

    // absolute time locked
    tx.vin[0].prevout.hash = txFirst[3]->GetHash();
    tx.nLockTime = m_node.chainman->ActiveChain().Tip()->GetMedianTimePast();
    prevheights.resize(1);
    prevheights[0] = baseheight + 4;
    hash = tx.GetHash();
    TryAddToMempool(tx_mempool, entry.Time(Now<NodeSeconds>()).FromTx(tx));
    BOOST_CHECK(!CheckFinalTxAtTip(*Assert(m_node.chainman->ActiveChain().Tip()), CTransaction{tx})); // Locktime fails
    BOOST_CHECK(TestSequenceLocks(CTransaction{tx}, tx_mempool)); // Sequence locks pass
    BOOST_CHECK(IsFinalTx(CTransaction(tx), m_node.chainman->ActiveChain().Tip()->nHeight + 2, m_node.chainman->ActiveChain().Tip()->GetMedianTimePast() + 1)); // Locktime passes 1 second later

    // mempool-dependent transactions (not added)
    tx.vin[0].prevout.hash = hash;
    prevheights[0] = m_node.chainman->ActiveChain().Tip()->nHeight + 1;
    tx.nLockTime = 0;
    tx.vin[0].nSequence = 0;
    BOOST_CHECK(CheckFinalTxAtTip(*Assert(m_node.chainman->ActiveChain().Tip()), CTransaction{tx})); // Locktime passes
    BOOST_CHECK(TestSequenceLocks(CTransaction{tx}, tx_mempool)); // Sequence locks pass
    tx.vin[0].nSequence = 1;
    BOOST_CHECK(!TestSequenceLocks(CTransaction{tx}, tx_mempool)); // Sequence locks fail
    tx.vin[0].nSequence = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG;
    BOOST_CHECK(TestSequenceLocks(CTransaction{tx}, tx_mempool)); // Sequence locks pass
    tx.vin[0].nSequence = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG | 1;
    BOOST_CHECK(!TestSequenceLocks(CTransaction{tx}, tx_mempool)); // Sequence locks fail

    auto block_template = mining->createNewBlock(options, /*cooldown=*/false);
    BOOST_REQUIRE(block_template);

    // None of the of the absolute height/time locked tx should have made
    // it into the template because we still check IsFinalTx in CreateNewBlock,
    // but relative locked txs will if inconsistently added to mempool.
    // For now these will still generate a valid template until BIP68 soft fork
    CBlock block{block_template->getBlock()};
    BOOST_CHECK_EQUAL(block.vtx.size(), 3U);
    // However if we advance height by 1 and time by SEQUENCE_LOCK_TIME, all of them should be mined
    for (int i = 0; i < CBlockIndex::nMedianTimeSpan; ++i) {
        CBlockIndex* ancestor{Assert(m_node.chainman->ActiveChain().Tip()->GetAncestor(m_node.chainman->ActiveChain().Tip()->nHeight - i))};
        ancestor->nTime += SEQUENCE_LOCK_TIME; // Trick the MedianTimePast
    }
    m_node.chainman->ActiveChain().Tip()->nHeight++;
    SetMockTime(m_node.chainman->ActiveChain().Tip()->GetMedianTimePast() + 1);

    block_template = mining->createNewBlock(options, /*cooldown=*/false);
    BOOST_REQUIRE(block_template);
    block = block_template->getBlock();
    BOOST_CHECK_EQUAL(block.vtx.size(), 5U);
}

void MinerTestingSetup::TestPrioritisedMining(const CScript& scriptPubKey, const std::vector<CTransactionRef>& txFirst)
{
    auto mining{MakeMining()};
    BOOST_REQUIRE(mining);

    BlockAssembler::Options options;
    options.coinbase_output_script = scriptPubKey;
    options.include_dummy_extranonce = true;

    CTxMemPool& tx_mempool{MakeMempool()};
    LOCK(tx_mempool.cs);

    TestMemPoolEntryHelper entry;

    // Test that a tx below min fee but prioritised is included
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vout.resize(1);
    tx.vout[0].nValue = 5000000000LL; // 0 fee
    Txid hashFreePrioritisedTx = tx.GetHash();
    TryAddToMempool(tx_mempool, entry.Fee(0).Time(Now<NodeSeconds>()).SpendsCoinbase(true).FromTx(tx));
    tx_mempool.PrioritiseTransaction(hashFreePrioritisedTx, 5 * COIN);

    tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vout[0].nValue = 5000000000LL - 1000;
    // This tx has a low fee: 1000 satoshis
    Txid hashParentTx = tx.GetHash(); // save this txid for later use
    TryAddToMempool(tx_mempool, entry.Fee(1000).Time(Now<NodeSeconds>()).SpendsCoinbase(true).FromTx(tx));

    // This tx has a medium fee: 10000 satoshis
    tx.vin[0].prevout.hash = txFirst[2]->GetHash();
    tx.vout[0].nValue = 5000000000LL - 10000;
    Txid hashMediumFeeTx = tx.GetHash();
    TryAddToMempool(tx_mempool, entry.Fee(10000).Time(Now<NodeSeconds>()).SpendsCoinbase(true).FromTx(tx));
    tx_mempool.PrioritiseTransaction(hashMediumFeeTx, -5 * COIN);

    // This tx also has a low fee, but is prioritised
    tx.vin[0].prevout.hash = hashParentTx;
    tx.vout[0].nValue = 5000000000LL - 1000 - 1000; // 1000 satoshi fee
    Txid hashPrioritsedChild = tx.GetHash();
    TryAddToMempool(tx_mempool, entry.Fee(1000).Time(Now<NodeSeconds>()).SpendsCoinbase(false).FromTx(tx));
    tx_mempool.PrioritiseTransaction(hashPrioritsedChild, 2 * COIN);

    // Test that transaction selection properly updates ancestor fee calculations as prioritised
    // parents get included in a block. Create a transaction with two prioritised ancestors, each
    // included by itself: FreeParent <- FreeChild <- FreeGrandchild.
    // When FreeParent is added, a modified entry will be created for FreeChild + FreeGrandchild
    // FreeParent's prioritisation should not be included in that entry.
    // When FreeChild is included, FreeChild's prioritisation should also not be included.
    tx.vin[0].prevout.hash = txFirst[3]->GetHash();
    tx.vout[0].nValue = 5000000000LL; // 0 fee
    Txid hashFreeParent = tx.GetHash();
    TryAddToMempool(tx_mempool, entry.Fee(0).SpendsCoinbase(true).FromTx(tx));
    tx_mempool.PrioritiseTransaction(hashFreeParent, 10 * COIN);

    tx.vin[0].prevout.hash = hashFreeParent;
    tx.vout[0].nValue = 5000000000LL; // 0 fee
    Txid hashFreeChild = tx.GetHash();
    TryAddToMempool(tx_mempool, entry.Fee(0).SpendsCoinbase(false).FromTx(tx));
    tx_mempool.PrioritiseTransaction(hashFreeChild, 1 * COIN);

    tx.vin[0].prevout.hash = hashFreeChild;
    tx.vout[0].nValue = 5000000000LL; // 0 fee
    Txid hashFreeGrandchild = tx.GetHash();
    TryAddToMempool(tx_mempool, entry.Fee(0).SpendsCoinbase(false).FromTx(tx));

    auto block_template = mining->createNewBlock(options, /*cooldown=*/false);
    BOOST_REQUIRE(block_template);
    CBlock block{block_template->getBlock()};
    BOOST_REQUIRE_EQUAL(block.vtx.size(), 6U);
    BOOST_CHECK(block.vtx[1]->GetHash() == hashFreeParent);
    BOOST_CHECK(block.vtx[2]->GetHash() == hashFreePrioritisedTx);
    BOOST_CHECK(block.vtx[3]->GetHash() == hashParentTx);
    BOOST_CHECK(block.vtx[4]->GetHash() == hashPrioritsedChild);
    BOOST_CHECK(block.vtx[5]->GetHash() == hashFreeChild);
    for (size_t i=0; i<block.vtx.size(); ++i) {
        // The FreeParent and FreeChild's prioritisations should not impact the child.
        BOOST_CHECK(block.vtx[i]->GetHash() != hashFreeGrandchild);
        // De-prioritised transaction should not be included.
        BOOST_CHECK(block.vtx[i]->GetHash() != hashMediumFeeTx);
    }
}

// NOTE: These tests rely on CreateNewBlock doing its own self-validation!
BOOST_AUTO_TEST_CASE(CreateNewBlock_validity)
{
    auto mining{MakeMining()};
    BOOST_REQUIRE(mining);

    // Note that by default, these tests run with size accounting enabled.
    CScript scriptPubKey = CScript() << "040d61d8653448c98731ee5fffd303c15e71ec2057b77f11ab3601979728cdaff2d68afbba14e4fa0bc44f2072b0b23ef63717f8cdfbe58dcd33f32b6afe98741a"_hex << OP_CHECKSIG;
    BlockAssembler::Options options;
    options.coinbase_output_script = scriptPubKey;
    options.include_dummy_extranonce = true;

    // Create and check a simple template
    std::unique_ptr<BlockTemplate> block_template = mining->createNewBlock(options, /*cooldown=*/false);
    BOOST_REQUIRE(block_template);
    {
        CBlock block{block_template->getBlock()};
        {
            std::string reason;
            std::string debug;
            BOOST_REQUIRE(!mining->checkBlock(block, {.check_pow = false}, reason, debug));
            BOOST_REQUIRE_EQUAL(reason, "bad-txnmrklroot");
            BOOST_REQUIRE_EQUAL(debug, "hashMerkleRoot mismatch");
        }

        block.hashMerkleRoot = BlockMerkleRoot(block);

        {
            std::string reason;
            std::string debug;
            BOOST_REQUIRE(mining->checkBlock(block, {.check_pow = false}, reason, debug));
            BOOST_REQUIRE_EQUAL(reason, "");
            BOOST_REQUIRE_EQUAL(debug, "");
        }

        {
            // A block template does not have proof-of-work, but it might pass
            // verification by coincidence. Grind the nonce if needed:
            while (CheckProofOfWork(block.GetHash(), block.nBits, Assert(m_node.chainman)->GetParams().GetConsensus())) {
                block.nNonce++;
            }

            std::string reason;
            std::string debug;
            BOOST_REQUIRE(!mining->checkBlock(block, {.check_pow = true}, reason, debug));
            BOOST_REQUIRE_EQUAL(reason, "high-hash");
            BOOST_REQUIRE_EQUAL(debug, "proof of work failed");
        }
    }

    // We can't make transactions until we have inputs
    // Therefore, load 110 blocks :)
    static_assert(std::size(BLOCKINFO) == 550, "Should have 550 blocks to import");
    int baseheight = 0;
    std::vector<CTransactionRef> txFirst;
    unsigned int i = 0;
    for (const auto& bi : BLOCKINFO) {
        const int current_height{mining->getTip()->height};

        /**
         * Simple block creation, nothing special yet.
         * If current_height is odd, block_template will have already been
         * set at the end of the previous loop.
         */
        if (current_height % 2 == 0) {
            block_template = mining->createNewBlock(options, /*cooldown=*/false);
            BOOST_REQUIRE(block_template);
        }

        CBlock block{block_template->getBlock()};
        CMutableTransaction txCoinbase(*block.vtx[0]);
        {
            LOCK(cs_main);
            block.nVersion = 4; //use version 4 as we enable BIP34, BIP65 and BIP66 since genesis
            block.nTime = Assert(m_node.chainman)->ActiveChain().Tip()->GetMedianTimePast()+1+i++;
            txCoinbase.version = 1;
            txCoinbase.vin[0].scriptSig = CScript{} << (current_height + 1) << bi.extranonce;
            txCoinbase.vout.resize(1); // Ignore the (optional) segwit commitment added by CreateNewBlock (as the hardcoded nonces don't account for this)
            txCoinbase.vout[0].scriptPubKey = CScript();
            block.vtx[0] = MakeTransactionRef(txCoinbase);
            if (txFirst.size() == 0)
                baseheight = current_height;
            if (txFirst.size() < 4)
                txFirst.push_back(block.vtx[0]);
            block.hashMerkleRoot = BlockMerkleRoot(block);
            block.nNonce = bi.nonce;
        }
        std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(block);
        // Alternate calls between Chainman's ProcessNewBlock and submitSolution
        // via the Mining interface. The former is used by net_processing as well
        // as the submitblock RPC.
        if (current_height % 2 == 0) {
            BOOST_REQUIRE(Assert(m_node.chainman)->ProcessNewBlock(shared_pblock, /*force_processing=*/true, /*min_pow_checked=*/true, nullptr));
        } else {
            BOOST_REQUIRE(block_template->submitSolution(block.nVersion, block.nTime, block.nNonce, MakeTransactionRef(txCoinbase)));
        }
        {
            LOCK(cs_main);
            // The above calls don't guarantee the tip is actually updated, so
            // we explicitly check this.
            auto maybe_new_tip{Assert(m_node.chainman)->ActiveChain().Tip()};
            BOOST_REQUIRE_EQUAL(maybe_new_tip->GetBlockHash(), block.GetHash());
        }
        if (current_height % 2 == 0) {
            block_template = block_template->waitNext();
            BOOST_REQUIRE(block_template);
        } else {
            // This just adds coverage
            mining->waitTipChanged(block.hashPrevBlock);
        }
    }

    LOCK(cs_main);

    TestBasicMining(scriptPubKey, txFirst, baseheight);

    m_node.chainman->ActiveChain().Tip()->nHeight--;
    SetMockTime(0);

    TestPackageSelection(scriptPubKey, txFirst);

    m_node.chainman->ActiveChain().Tip()->nHeight--;
    SetMockTime(0);

    TestPrioritisedMining(scriptPubKey, txFirst);
}

BOOST_AUTO_TEST_SUITE_END()
