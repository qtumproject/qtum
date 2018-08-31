// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <coins.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <validation.h>
#include <miner.h>
#include <policy/policy.h>
#include <pubkey.h>
#include <script/standard.h>
#include <txmempool.h>
#include <uint256.h>
#include <util.h>
#include <utilstrencodings.h>

#include <test/test_bitcoin.h>

#include <memory>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(miner_tests, TestingSetup)

// BOOST_CHECK_EXCEPTION predicates to check the specific validation error
class HasReason {
public:
    HasReason(const std::string& reason) : m_reason(reason) {}
    bool operator() (const std::runtime_error& e) const {
        return std::string(e.what()).find(m_reason) != std::string::npos;
    };
private:
    const std::string m_reason;
};

static CFeeRate blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);

static BlockAssembler AssemblerForTest(const CChainParams& params) {
    BlockAssembler::Options options;

    options.nBlockMaxWeight = dgpMaxBlockWeight;
    options.blockMinFeeRate = blockMinFeeRate;
    return BlockAssembler(params, options);
}

static
struct {
    unsigned char extranonce;
    unsigned int nonce;
} blockinfo[] = {
        {4, 0x000135b5}, {2, 0x0001535a}, {1, 0x00029f51}, {1, 0x000447a7},
        {2, 0x00046a5d}, {2, 0x000452af}, {1, 0x000463ec}, {2, 0x00062f5e},
        {2, 0x00079e2c}, {1, 0x0008a304}, {1, 0x000ae9a9}, {2, 0x000a6c15},
        {2, 0x000b38f8}, {1, 0x000c58a1}, {2, 0x000ba426}, {2, 0x000dfb38},
        {1, 0x000eb164}, {2, 0x000e46d9}, {1, 0x000e38c7}, {1, 0x000e4ff9},
        {3, 0x00102efa}, {2, 0x0011c056}, {2, 0x001158b9}, {1, 0x00144969},
        {2, 0x00146f39}, {1, 0x0014d8e8}, {2, 0x001601bc}, {2, 0x0016551d},
        {2, 0x00177b22}, {2, 0x0018e947}, {2, 0x0019bb2d}, {2, 0x001bdc48},
        {1, 0x0019fac5}, {2, 0x001b724d}, {2, 0x001b3967}, {1, 0x001dddb0},
        {2, 0x001fb782}, {1, 0x001fb80f}, {2, 0x00220adb}, {1, 0x0023a504},
        {1, 0x0024a22d}, {3, 0x00250dd7}, {2, 0x0025ee99}, {5, 0x0027b428},
        {1, 0x002b2a28}, {5, 0x00299ac8}, {1, 0x0028a6cb}, {1, 0x002adf44},
        {1, 0x002a8060}, {2, 0x002b99be}, {1, 0x002e9f05}, {1, 0x00306140},
        {1, 0x002fa812}, {1, 0x0031ba13}, {5, 0x0033d7c8}, {5, 0x0032f635},
        {1, 0x00361303}, {1, 0x0035ec1e}, {6, 0x003ac161}, {2, 0x003b8c90},
        {2, 0x003a8544}, {1, 0x003befbf}, {1, 0x003c4593}, {1, 0x003dc673},
        {2, 0x003c86fe}, {2, 0x003d6942}, {1, 0x003ffb6e}, {1, 0x0040b722},
        {1, 0x004395ba}, {5, 0x004aa396}, {5, 0x004c338c}, {1, 0x004cb1cb},
        {1, 0x004f17fd}, {2, 0x004e5b2a}, {2, 0x004f5944}, {1, 0x00503c96},
        {2, 0x004f265c}, {1, 0x004f4d4d}, {2, 0x005058c9}, {2, 0x0052148c},
        {1, 0x005489f8}, {1, 0x0054dd61}, {1, 0x005686fb}, {5, 0x00598f70},
        {1, 0x0059064b}, {1, 0x005b574c}, {1, 0x005bc594}, {1, 0x005e65fe},
        {1, 0x005e9c05}, {1, 0x0060c7a3}, {1, 0x005ef50f}, {2, 0x00607240},
        {0, 0x0060f60f}, {1, 0x00626677}, {2, 0x0061feb9}, {2, 0x00644c94},
        {2, 0x00655ec3}, {1, 0x006632cd}, {1, 0x006a4091}, {1, 0x006a5d59},
        {1, 0x006b4be0}, {1, 0x006bd08f}, {1, 0x006dd899}, {5, 0x0075b7ce},
        {2, 0x006dd957}, {1, 0x006ddd91}, {1, 0x00700ff2}, {1, 0x007177dd},
        {2, 0x0070ef64}, {2, 0x0072fdfe}, {4, 0x0076ecfb}, {2, 0x0076bfe7},
        {1, 0x0078a1c1}, {1, 0x00795659}, {2, 0x007989e8}, {2, 0x007d378e},
        {1, 0x007ac186}, {2, 0x007afd06}, {2, 0x007b3640}, {1, 0x007bcbd8},
        {1, 0x007bebf5}, {2, 0x00811938}, {2, 0x007f1a60}, {1, 0x007f59b6},
        {2, 0x008020b2}, {2, 0x0082919a}, {1, 0x0082d03d}, {2, 0x008372ff},
        {1, 0x0083d1bc}, {1, 0x0083a951}, {3, 0x008456c3}, {2, 0x008983bf},
        {2, 0x0089d194}, {1, 0x008c74a1}, {2, 0x008b9d71}, {1, 0x008daf24},
        {2, 0x008d97b1}, {2, 0x008e3f8c}, {2, 0x008e18ea}, {2, 0x0091d324},
        {2, 0x00940a83}, {2, 0x009446ed}, {1, 0x00958602}, {2, 0x00957089},
        {2, 0x00955338}, {1, 0x009721cc}, {2, 0x0096284d}, {1, 0x0095e5b9},
        {2, 0x009772cb}, {1, 0x0099d53b}, {1, 0x0099860c}, {3, 0x009a3b3b},
        {2, 0x009e6597}, {5, 0x009b44be}, {1, 0x009ae757}, {5, 0x009c89bf},
        {1, 0x009cd503}, {1, 0x009d9a08}, {1, 0x009dbc32}, {2, 0x009fae8a},
        {1, 0x009fa89d}, {1, 0x00a0f881}, {1, 0x00a08f52}, {1, 0x00a0cfb2},
        {5, 0x00a0b3d9}, {5, 0x00a189d6}, {1, 0x00a3f1e4}, {1, 0x00a34149},
        {6, 0x00a54ae6}, {2, 0x00a76413}, {2, 0x00a731ac}, {1, 0x00aa1e61},
        {1, 0x00aae545}, {1, 0x00ae616e}, {2, 0x00af8b04}, {2, 0x00b15897},
        {1, 0x00b1d797}, {1, 0x00b2514a}, {1, 0x00b6da2c}, {5, 0x00b5d69d},
        {5, 0x00b60f8b}, {1, 0x00b7ddf4}, {1, 0x00b7f2d6}, {2, 0x00b9f1b8},
        {2, 0x00bb1328}, {1, 0x00c3b765}, {2, 0x00c3c4c1}, {1, 0x00c35e83},
        {2, 0x00c3299f}, {2, 0x00c6d7ca}, {1, 0x00c8cf93}, {1, 0x00c80b8c},
        {1, 0x00c72875}, {5, 0x00c7a637}, {1, 0x00c8e4b7}, {1, 0x00ca7818},
        {1, 0x00cbf135}, {1, 0x00cc76e1}, {1, 0x00cda36a}, {1, 0x00cc8307},
        {1, 0x00ce5bac}, {2, 0x00cf4fd3}, {0, 0x00cf91a4}, {1, 0x00d03423},
        {2, 0x00d06969}, {2, 0x00d07d66}, {2, 0x00d0302f}, {1, 0x00d13474},
        {1, 0x00d3155e}, {1, 0x00d1b9bc}, {1, 0x00d25df2}, {1, 0x00d1f159},
        {1, 0x00d3007c}, {5, 0x00d3bf36}, {2, 0x00d70c84}, {1, 0x00d5e3d4},
        {1, 0x00db4fbc}, {1, 0x00d9267f}, {2, 0x00d8ac8a}, {2, 0x00d9a8b9},
        {4, 0x00d94ff2}, {2, 0x00d94334}, {1, 0x00dab723}, {1, 0x00e09787},
        {2, 0x00dfec52}, {2, 0x00e0a697}, {1, 0x00e0527b}, {2, 0x00e13147},
        {2, 0x00e3fcbd}, {1, 0x00e457fd}, {1, 0x00e52160}, {2, 0x00e5ee55},
        {2, 0x00e63f4f}, {1, 0x00e6c82f}, {2, 0x00e85025}, {2, 0x00ed6541},
        {1, 0x00edb959}, {2, 0x00ef2b7f}, {1, 0x00f3949a}, {1, 0x00f440bf},
        {3, 0x00f745dc}, {2, 0x00f6fe41}, {2, 0x00fa4ae2}, {1, 0x00faa6a6},
        {2, 0x00fd1c9a}, {1, 0x00fbe743}, {2, 0x00fcf092}, {2, 0x00fe2a3e},
        {2, 0x00fd7cc5}, {2, 0x00ff701e}, {2, 0x0100e795}, {2, 0x0100942d},
        {1, 0x01021e8c}, {2, 0x01037e9a}, {2, 0x0104892d}, {1, 0x0108794c},
        {2, 0x01096d59}, {1, 0x0109f51d}, {2, 0x010b3152}, {1, 0x010d491b},
        {1, 0x010eda18}, {3, 0x010eae14}, {2, 0x01124e31}, {5, 0x01104819},
        {1, 0x0112cbc8}, {5, 0x01123ea2}, {1, 0x0112e2ce}, {1, 0x01129f34},
        {1, 0x0114ff67}, {2, 0x0116ec09}, {1, 0x011664b4}, {1, 0x011984af},
        {1, 0x011b8afc}, {1, 0x011cbfc9}, {5, 0x011f52cf}, {5, 0x011d569a},
        {1, 0x011da9ed}, {1, 0x0121dd81}, {6, 0x01201296}, {2, 0x01207bd7},
        {2, 0x0124163c}, {1, 0x01275484}, {1, 0x0124e21a}, {1, 0x0125f068},
        {2, 0x0126614a}, {2, 0x0127a6fe}, {1, 0x012865fb}, {1, 0x012a4f19},
        {1, 0x012b3841}, {5, 0x012c7c35}, {5, 0x012fbe45}, {1, 0x01306e93},
        {1, 0x0131cbc4}, {2, 0x01323e3d}, {2, 0x013698a4}, {1, 0x01349f3c},
        {2, 0x01360791}, {1, 0x01372a89}, {2, 0x01374e37}, {2, 0x0138ce82},
        {1, 0x01381729}, {1, 0x013a4922}, {1, 0x013b21ea}, {5, 0x013c035d},
        {1, 0x013c2648}, {1, 0x013cea75}, {1, 0x013f953a}, {1, 0x013d409a},
        {1, 0x013e2eba}, {1, 0x01406e6d}, {1, 0x013e9b18}, {2, 0x013ea1e7},
        {0, 0x0146472a}, {1, 0x01453129}, {2, 0x01444e90}, {2, 0x014558c3},
        {2, 0x0145c66a}, {1, 0x0146a5c5}, {1, 0x01499c5c}, {1, 0x014b3719},
        {1, 0x014c82d6}, {1, 0x014c61fe}, {1, 0x014d7d29}, {5, 0x014f6021},
        {2, 0x014f8391}, {1, 0x014faa01}, {1, 0x0150f07f}, {1, 0x01508eec},
        {2, 0x015339f7}, {2, 0x01522946}, {4, 0x0153d413}, {2, 0x01531132},
        {1, 0x01539c10}, {1, 0x01538fa3}, {2, 0x01568d50}, {2, 0x01570520},
        {1, 0x01598663}, {2, 0x0158f956}, {2, 0x0159de4c}, {1, 0x015e2e2a},
        {1, 0x015e0f39}, {2, 0x015dc1aa}, {2, 0x015e7ef0}, {1, 0x015dd8e4},
        {2, 0x0160427f}, {2, 0x015fd33e}, {1, 0x015fcd11}, {2, 0x016129e2},
        {1, 0x01608d9f}, {1, 0x01618bb9}, {3, 0x0161c62c}, {2, 0x0162cb31},
        {2, 0x0165398b}, {1, 0x0166ad09}, {2, 0x0167610e}, {1, 0x016727cb},
        {2, 0x01675f3d}, {2, 0x016b1e1b}, {2, 0x016afabc}, {2, 0x016ad30c},
        {2, 0x016c733b}, {2, 0x016e02ae}, {1, 0x016ccb3b}, {2, 0x016e68ac},
        {2, 0x016eb853}, {1, 0x016fd28c}, {2, 0x017208c1}, {1, 0x017129d5},
        {2, 0x0173c8a5}, {1, 0x01742270}, {1, 0x017409d7}, {3, 0x01744606},
        {2, 0x0178d4de}, {5, 0x017755fd}, {1, 0x0177bae9}, {5, 0x017a98f4},
        {1, 0x017abf35}, {1, 0x017a273e}, {1, 0x0179dab0}, {2, 0x017c1698},
        {1, 0x017b6e4c}, {1, 0x017aa311}, {1, 0x017dc7a0}, {1, 0x017e5421},
        {5, 0x017fc31a}, {5, 0x01814d53}, {1, 0x017eafba}, {1, 0x017f20cc},
        {6, 0x017fa19c}, {2, 0x0180a3b1}, {2, 0x0180ba39}, {1, 0x0180cbaa},
        {1, 0x01815cfb}, {1, 0x0185280d}, {2, 0x018583ff}, {2, 0x0184e4fc},
        {1, 0x018518cf}, {1, 0x0186370b}, {1, 0x0186ef77}, {5, 0x01867169},
        {5, 0x01891212}, {1, 0x01894126}, {1, 0x018b5f43}, {2, 0x018e5a9f},
        {2, 0x018df690}, {1, 0x018f6ef1}, {2, 0x018dd6ec}, {1, 0x018e4acc},
        {2, 0x01910d40}, {2, 0x0191cd46}, {1, 0x0191a3fb}, {1, 0x0193ad3d},
        {1, 0x01935a45}, {5, 0x01945f12}, {1, 0x0196817e}, {1, 0x0197f5a4},
        {1, 0x0199189a}, {1, 0x0198f075}, {1, 0x01988ec0}, {1, 0x0199a26e},
        {1, 0x019b0f64}, {2, 0x019aeb8b}, {0, 0x019d464e}, {1, 0x019cb645},
        {2, 0x019e7492}, {2, 0x019ec4b2}, {2, 0x01a059c5}, {1, 0x01a15c8d},
        {1, 0x01a1c1de}, {1, 0x01a35e34}, {1, 0x01a42613}, {1, 0x01a315c1},
        {1, 0x01a6136d}, {5, 0x01a4ed09}, {2, 0x01a3d3e2}, {1, 0x01a56163},
        {1, 0x01a4b140}, {1, 0x01a75dff}, {2, 0x01a8445e}, {2, 0x01aa1963},
        {4, 0x01a8b072}, {2, 0x01aa2810}, {1, 0x01ab5524}, {1, 0x01abc7e6},
        {2, 0x01ab49ba}, {2, 0x01afc536}, {1, 0x01ae9fa1}, {2, 0x01af41c8},
        {2, 0x01b0be8f}, {1, 0x01af032f}, {1, 0x01b2ce20}, {2, 0x01b23d60},
        {2, 0x01b52c9c}, {1, 0x01b41f2d}, {2, 0x01b4bb56}, {2, 0x01b6535f},
        {1, 0x01b559d1}, {2, 0x01b63797}, {1, 0x01b6a188}, {1, 0x01b6ef01},
        {3, 0x01b742e4}, {2, 0x01b9d0c9}, {2, 0x01b909fd}, {1, 0x01b97990},
        {2, 0x01be4857}, {1, 0x01bdb55a}, {2, 0x01bdb46f}, {2, 0x01be3d77},
        {2, 0x01bf503e}, {2, 0x01beb04e}, {2, 0x01bf00c2}, {2, 0x01c34b6f},
        {1, 0x01c3f337}, {2, 0x01c568ea}, {2, 0x01c6bafb}, {1, 0x01c7f571},
        {2, 0x01c7630d}, {1, 0x01c8360c}, {2, 0x01c8f730}, {1, 0x01ca19e8},
        {1, 0x01cb719c}, {3, 0x01cfc1a7}, {2, 0x01d1f198}, {5, 0x01d106ca},
        {1, 0x01d3c805}, {5, 0x01d55380}, {1, 0x01d60c07}, {1, 0x01d87f3d},
        {1, 0x01d977a5}, {2, 0x01db79fd}, {1, 0x01ddc7ec}, {1, 0x01e041e9},
        {1, 0x01e01084}, {1, 0x01e228ce}, {5, 0x01e5386d}, {5, 0x01e6f35e},
        {1, 0x01e9dac0}, {1, 0x01e9185d}, {6, 0x01e93c13}, {2, 0x01e90352},
        {2, 0x01ea4add}, {1, 0x01ea5884}, {1, 0x01eacdc6}, {1, 0x01ec02a3},
        {2, 0x01ec6ca1}, {2, 0x01edd3b8}, {1, 0x01eee113}, {1, 0x01ede61e},
        {1, 0x01edd5a9}, {5, 0x01ef71ab}, {5, 0x01f3fe61}, {1, 0x01f43899},
        {1, 0x01f45068}, {2, 0x01f7965c}, {2, 0x01f56398}, {1, 0x01f71b25},
        {2, 0x01f6a0da}, {1, 0x01f850ee}, {2, 0x01fcde96}, {2, 0x01fce16e},
        {1, 0x01ff1acb}, {1, 0x02041cf4}, {1, 0x0204836f}, {5, 0x0205be24},
        {1, 0x0206413c}, {1, 0x020a65ee}, {1, 0x02081188}, {1, 0x020b6c15},
        {1, 0x02085954}, {1, 0x0209426a}, {1, 0x020adb63}, {2, 0x020cf735},
        {0, 0x020ed674}, {1, 0x0211fa9a}, {2, 0x02141c0b}, {2, 0x0213c1b0},
        {2, 0x021a0e52}, {1, 0x0219e24e}, {1, 0x021a49ad}, {1, 0x021ab401},
        {1, 0x021b6158}, {1, 0x021c9ad7}, {1, 0x021f9bbf}, {5, 0x0220bdb1},
        {2, 0x0220c642}, {1, 0x0220f703}, {1, 0x022397a0}, {1, 0x022663a5},
        {2, 0x022681d6}, {2, 0x0227a53a},
};

static CBlockIndex CreateBlockIndex(int nHeight)
{
    CBlockIndex index;
    index.nHeight = nHeight;
    index.pprev = chainActive.Tip();
    return index;
}

static bool TestSequenceLocks(const CTransaction &tx, int flags)
{
    LOCK(mempool.cs);
    return CheckSequenceLocks(tx, flags);
}

// Test suite for ancestor feerate transaction selection.
// Implemented as an additional function, rather than a separate test case,
// to allow reusing the blockchain created in CreateNewBlock_validity.
static void TestPackageSelection(const CChainParams& chainparams, const CScript& scriptPubKey, const std::vector<CTransactionRef>& txFirst) EXCLUSIVE_LOCKS_REQUIRED(::mempool.cs)
{
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
    uint256 hashParentTx = tx.GetHash(); // save this txid for later use
    mempool.addUnchecked(hashParentTx, entry.Fee(400000).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));

    // This tx has a medium fee: 10000 satoshis
    tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    tx.vout[0].nValue = 5000000000LL - 4000000;
    uint256 hashMediumFeeTx = tx.GetHash();
    mempool.addUnchecked(hashMediumFeeTx, entry.Fee(4000000).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));

    // This tx has a high fee, but depends on the first transaction
    tx.vin[0].prevout.hash = hashParentTx;
    tx.vout[0].nValue = 5000000000LL - 10000 - 400000 * 50; // 50k satoshi fee
    uint256 hashHighFeeTx = tx.GetHash();
    mempool.addUnchecked(hashHighFeeTx, entry.Fee(400000 * 50).Time(GetTime()).SpendsCoinbase(false).FromTx(tx));

    std::unique_ptr<CBlockTemplate> pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey);
    BOOST_CHECK(pblocktemplate->block.vtx[1]->GetHash() == hashParentTx);
    BOOST_CHECK(pblocktemplate->block.vtx[2]->GetHash() == hashHighFeeTx);
    BOOST_CHECK(pblocktemplate->block.vtx[3]->GetHash() == hashMediumFeeTx);

    // Test that a package below the block min tx fee doesn't get included
    tx.vin[0].prevout.hash = hashHighFeeTx;
    tx.vout[0].nValue = 5000000000LL - 10000 - 20000000; // 0 fee
    uint256 hashFreeTx = tx.GetHash();
    mempool.addUnchecked(hashFreeTx, entry.Fee(0).FromTx(tx));
    size_t freeTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);

    // Calculate a fee on child transaction that will put the package just
    // below the block min tx fee (assuming 1 child tx of the same size).
    CAmount feeToUse = blockMinFeeRate.GetFee(2*freeTxSize) - 1;

    tx.vin[0].prevout.hash = hashFreeTx;
    tx.vout[0].nValue = 5000000000LL - 10000 - 20000000 - feeToUse;
    uint256 hashLowFeeTx = tx.GetHash();
    mempool.addUnchecked(hashLowFeeTx, entry.Fee(feeToUse).FromTx(tx));
    pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey);
    // Verify that the free tx and the low fee tx didn't get selected
    for (size_t i=0; i<pblocktemplate->block.vtx.size(); ++i) {
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashFreeTx);
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashLowFeeTx);
    }

    // Test that packages above the min relay fee do get included, even if one
    // of the transactions is below the min relay fee
    // Remove the low fee transaction and replace with a higher fee transaction
    mempool.removeRecursive(tx);
    tx.vout[0].nValue -= 2; // Now we should be just over the min relay fee
    hashLowFeeTx = tx.GetHash();
    mempool.addUnchecked(hashLowFeeTx, entry.Fee(feeToUse+2).FromTx(tx));
    pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey);
    BOOST_CHECK(pblocktemplate->block.vtx[4]->GetHash() == hashFreeTx);
    BOOST_CHECK(pblocktemplate->block.vtx[5]->GetHash() == hashLowFeeTx);

    // Test that transaction selection properly updates ancestor fee
    // calculations as ancestor transactions get included in a block.
    // Add a 0-fee transaction that has 2 outputs.
    tx.vin[0].prevout.hash = txFirst[2]->GetHash();
    tx.vout.resize(2);
    tx.vout[0].nValue = 5000000000LL - 100000000;
    tx.vout[1].nValue = 100000000; // 1BTC output
    uint256 hashFreeTx2 = tx.GetHash();
    mempool.addUnchecked(hashFreeTx2, entry.Fee(0).SpendsCoinbase(true).FromTx(tx));

    // This tx can't be mined by itself
    tx.vin[0].prevout.hash = hashFreeTx2;
    tx.vout.resize(1);
    feeToUse = blockMinFeeRate.GetFee(freeTxSize);
    tx.vout[0].nValue = 5000000000LL - 100000000 - feeToUse;
    uint256 hashLowFeeTx2 = tx.GetHash();
    mempool.addUnchecked(hashLowFeeTx2, entry.Fee(feeToUse).SpendsCoinbase(false).FromTx(tx));
    pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey);

    // Verify that this tx isn't selected.
    for (size_t i=0; i<pblocktemplate->block.vtx.size(); ++i) {
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashFreeTx2);
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashLowFeeTx2);
    }

    // This tx will be mineable, and should cause hashLowFeeTx2 to be selected
    // as well.
    tx.vin[0].prevout.n = 1;
    tx.vout[0].nValue = 100000000 - 4000000; // 10k satoshi fee
    mempool.addUnchecked(tx.GetHash(), entry.Fee(4000000).FromTx(tx));
    pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey);
    BOOST_CHECK(pblocktemplate->block.vtx[8]->GetHash() == hashLowFeeTx2);
}
CAmount calculateReward(const CBlock& block){
    CAmount sumVout = 0, fee = 0;
    for(const CTransactionRef t : block.vtx){
        fee += pcoinsTip->GetValueIn(*t);
        sumVout += t->GetValueOut();
    }
    return sumVout - fee;
}

// NOTE: These tests rely on CreateNewBlock doing its own self-validation!
BOOST_AUTO_TEST_CASE(CreateNewBlock_validity)
{
    // Note that by default, these tests run with size accounting enabled.
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const CChainParams& chainparams = *chainParams;
    CScript scriptPubKey = CScript() << ParseHex("040d61d8653448c98731ee5fffd303c15e71ec2057b77f11ab3601979728cdaff2d68afbba14e4fa0bc44f2072b0b23ef63717f8cdfbe58dcd33f32b6afe98741a") << OP_CHECKSIG;
    std::unique_ptr<CBlockTemplate> pblocktemplate;
    CMutableTransaction tx;
    CScript script;
    uint256 hash;
    TestMemPoolEntryHelper entry;
    entry.nFee = 11;
    entry.nHeight = 11;

    fCheckpointsEnabled = false;

    // Simple block creation, nothing special yet:
    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));

    // We can't make transactions until we have inputs
    // Therefore, load 100 blocks :)
    int baseheight = 0;
    std::vector<CTransactionRef> txFirst;
    for (unsigned int i = 0; i < sizeof(blockinfo)/sizeof(*blockinfo); ++i)
    {
        CBlock *pblock = &pblocktemplate->block; // pointer for convenience
        {
            LOCK(cs_main);
            pblock->nVersion = 4; //use version 4 as we enable BIP34, BIP65 and BIP66 since genesis
            pblock->nTime = chainActive.Tip()->GetMedianTimePast()+1+i;
            CMutableTransaction txCoinbase(*pblock->vtx[0]);
            txCoinbase.nVersion = 1;
            txCoinbase.vin[0].scriptSig = CScript();
            txCoinbase.vin[0].scriptSig = CScript() << chainActive.Height()+1 << blockinfo[i].extranonce;
            txCoinbase.vout.resize(1); // Ignore the (optional) segwit commitment added by CreateNewBlock (as the hardcoded nonces don't account for this)
            txCoinbase.vout[0].scriptPubKey = CScript();
            pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));
            if (txFirst.size() == 0)
                baseheight = chainActive.Height();
            if (txFirst.size() < 4)
                txFirst.push_back(pblock->vtx[0]);
            pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
            pblock->nNonce = blockinfo[i].nonce;
        }
        std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(*pblock);
        BOOST_CHECK(ProcessNewBlock(chainparams, shared_pblock, true, nullptr));
        pblock->hashPrevBlock = pblock->GetHash();
    }

    LOCK(cs_main);
    LOCK(::mempool.cs);

    // Just to make sure we can still make simple blocks
    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));

    const CAmount BLOCKSUBSIDY = 50*COIN;
    const CAmount LOWFEE = CENT;
    const CAmount HIGHFEE = COIN;
    const CAmount HIGHERFEE = 4*COIN;

    // block sigops > limit: 1000 CHECKMULTISIG + 1
    tx.vin.resize(1);
    // NOTE: OP_NOP is used to force 20 SigOps for the CHECKMULTISIG
    tx.vin[0].scriptSig = CScript() << OP_0 << OP_0 << OP_0 << OP_NOP << OP_CHECKMULTISIG << OP_1;
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vout.resize(1);
    tx.vout[0].nValue = BLOCKSUBSIDY;
    for (unsigned int i = 0; i < 1001; ++i)
    {
        tx.vout[0].nValue -= LOWFEE;
        hash = tx.GetHash();
        bool spendsCoinbase = i == 0; // only first tx spends coinbase
        // If we don't set the # of sig ops in the CTxMemPoolEntry, template creation fails
        mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(spendsCoinbase).FromTx(tx));
        tx.vin[0].prevout.hash = hash;
    }

    BOOST_CHECK_EXCEPTION(AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error, HasReason("bad-blk-sigops"));
    mempool.clear();

    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vout[0].nValue = BLOCKSUBSIDY;
    for (unsigned int i = 0; i < 1001; ++i)
    {
        tx.vout[0].nValue -= LOWFEE;
        hash = tx.GetHash();
        bool spendsCoinbase = i == 0; // only first tx spends coinbase
        // If we do set the # of sig ops in the CTxMemPoolEntry, template creation passes
        mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(spendsCoinbase).SigOpsCost(80).FromTx(tx));
        tx.vin[0].prevout.hash = hash;
    }
    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));
    mempool.clear();

    // block size > limit
    tx.vin[0].scriptSig = CScript();
    // 18 * (520char + DROP) + OP_1 = 9433 bytes
    std::vector<unsigned char> vchData(52);
    for (unsigned int i = 0; i < 18; ++i)
        tx.vin[0].scriptSig << vchData << OP_DROP;
    tx.vin[0].scriptSig << OP_1;
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vout[0].nValue = BLOCKSUBSIDY;
    for (unsigned int i = 0; i < 128; ++i)
    {
        tx.vout[0].nValue -= LOWFEE;
        hash = tx.GetHash();
        bool spendsCoinbase = i == 0; // only first tx spends coinbase
        mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(spendsCoinbase).FromTx(tx));
        tx.vin[0].prevout.hash = hash;
    }
    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));
    mempool.clear();

    // orphan in mempool, template creation fails
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).FromTx(tx));
    BOOST_CHECK_EXCEPTION(AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error, HasReason("bad-txns-inputs-missingorspent"));
    mempool.clear();

    // child with higher feerate than parent
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    tx.vout[0].nValue = BLOCKSUBSIDY-HIGHFEE;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(HIGHFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    tx.vin[0].prevout.hash = hash;
    tx.vin.resize(2);
    tx.vin[1].scriptSig = CScript() << OP_1;
    tx.vin[1].prevout.hash = txFirst[0]->GetHash();
    tx.vin[1].prevout.n = 0;
    tx.vout[0].nValue = tx.vout[0].nValue+BLOCKSUBSIDY-HIGHERFEE; //First txn output + fresh coinbase - new txn fee
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(HIGHERFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));
    mempool.clear();

    // coinbase in mempool, template creation fails
    tx.vin.resize(1);
    tx.vin[0].prevout.SetNull();
    tx.vin[0].scriptSig = CScript() << OP_0 << OP_1;
    tx.vout[0].nValue = 0;
    hash = tx.GetHash();
    // give it a fee so it'll get mined
    mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(false).FromTx(tx));
    // Should throw bad-cb-multiple
    BOOST_CHECK_EXCEPTION(AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error, HasReason("bad-cb-multiple"));
    mempool.clear();

    // double spend txn pair in mempool, template creation fails
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vout[0].nValue = BLOCKSUBSIDY-HIGHFEE;
    tx.vout[0].scriptPubKey = CScript() << OP_1;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(HIGHFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    tx.vout[0].scriptPubKey = CScript() << OP_2;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(HIGHFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    BOOST_CHECK_EXCEPTION(AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error, HasReason("bad-txns-inputs-missingorspent"));
    mempool.clear();

    // subsidy changing
    int nHeight = chainActive.Height();
    // Create an actual 209999-long block chain (without valid blocks).
    while (chainActive.Tip()->nHeight < 990499) {
        CBlockIndex* prev = chainActive.Tip();
        CBlockIndex* next = new CBlockIndex();
        next->phashBlock = new uint256(InsecureRand256());
        pcoinsTip->SetBestBlock(next->GetBlockHash());
        next->pprev = prev;
        next->nHeight = prev->nHeight + 1;
        next->BuildSkip();
        chainActive.SetTip(next);
    }
    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey, true, true));
    BOOST_CHECK(calculateReward(pblocktemplate->block) == 400000000);

    // Extend to a 210000-long block chain.
    while (chainActive.Tip()->nHeight < 990501) {
        CBlockIndex* prev = chainActive.Tip();
        CBlockIndex* next = new CBlockIndex();
        next->phashBlock = new uint256(InsecureRand256());
        pcoinsTip->SetBestBlock(next->GetBlockHash());
        next->pprev = prev;
        next->nHeight = prev->nHeight + 1;
        next->BuildSkip();
        chainActive.SetTip(next);
    }

    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey, true, true));
    BOOST_CHECK(calculateReward(pblocktemplate->block) == 200000000);

    // invalid p2sh txn in mempool, template creation fails
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vin[0].prevout.n = 0;
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vout[0].nValue = BLOCKSUBSIDY-LOWFEE;
    script = CScript() << OP_0;
    tx.vout[0].scriptPubKey = GetScriptForDestination(CScriptID(script));
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    tx.vin[0].prevout.hash = hash;
    tx.vin[0].scriptSig = CScript() << std::vector<unsigned char>(script.begin(), script.end());
    tx.vout[0].nValue -= LOWFEE;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(false).FromTx(tx));
    // Should throw block-validation-failed
    BOOST_CHECK_EXCEPTION(AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error, HasReason("TestBlockValidity failed"));
    mempool.clear();

    // Delete the dummy blocks again.
    while (chainActive.Tip()->nHeight > nHeight) {
        CBlockIndex* del = chainActive.Tip();
        chainActive.SetTip(del->pprev);
        pcoinsTip->SetBestBlock(del->pprev->GetBlockHash());
        delete del->phashBlock;
        delete del;
    }

    // non-final txs in mempool
    SetMockTime(chainActive.Tip()->GetMedianTimePast()+1);
    int flags = LOCKTIME_VERIFY_SEQUENCE|LOCKTIME_MEDIAN_TIME_PAST;
    // height map
    std::vector<int> prevheights;

    // relative height locked
    tx.nVersion = 2;
    tx.vin.resize(1);
    prevheights.resize(1);
    tx.vin[0].prevout.hash = txFirst[0]->GetHash(); // only 1 transaction
    tx.vin[0].prevout.n = 0;
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vin[0].nSequence = chainActive.Tip()->nHeight + 1; // txFirst[0] is the 2nd block
    prevheights[0] = baseheight + 1;
    tx.vout.resize(1);
    tx.vout[0].nValue = BLOCKSUBSIDY-HIGHFEE;
    tx.vout[0].scriptPubKey = CScript() << OP_1;
    tx.nLockTime = 0;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(HIGHFEE).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));
    BOOST_CHECK(CheckFinalTx(tx, flags)); // Locktime passes
    BOOST_CHECK(!TestSequenceLocks(tx, flags)); // Sequence locks fail
    BOOST_CHECK(SequenceLocks(tx, flags, &prevheights, CreateBlockIndex(chainActive.Tip()->nHeight + 2))); // Sequence locks pass on 2nd block

    // relative time locked
    tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    tx.vin[0].nSequence = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG | (((chainActive.Tip()->GetMedianTimePast()+1-chainActive[1]->GetMedianTimePast()) >> CTxIn::SEQUENCE_LOCKTIME_GRANULARITY) + 1); // txFirst[1] is the 3rd block
    prevheights[0] = baseheight + 2;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Time(GetTime()).FromTx(tx));
    BOOST_CHECK(CheckFinalTx(tx, flags)); // Locktime passes
    BOOST_CHECK(!TestSequenceLocks(tx, flags)); // Sequence locks fail

    for (int i = 0; i < CBlockIndex::nMedianTimeSpan; i++)
        chainActive.Tip()->GetAncestor(chainActive.Tip()->nHeight - i)->nTime += 512; //Trick the MedianTimePast
    BOOST_CHECK(SequenceLocks(tx, flags, &prevheights, CreateBlockIndex(chainActive.Tip()->nHeight + 1))); // Sequence locks pass 512 seconds later
    for (int i = 0; i < CBlockIndex::nMedianTimeSpan; i++)
        chainActive.Tip()->GetAncestor(chainActive.Tip()->nHeight - i)->nTime -= 512; //undo tricked MTP

    // absolute height locked
    tx.vin[0].prevout.hash = txFirst[2]->GetHash();
    tx.vin[0].nSequence = CTxIn::SEQUENCE_FINAL - 1;
    prevheights[0] = baseheight + 3;
    tx.nLockTime = chainActive.Tip()->nHeight + 1;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Time(GetTime()).FromTx(tx));
    BOOST_CHECK(!CheckFinalTx(tx, flags)); // Locktime fails
    BOOST_CHECK(TestSequenceLocks(tx, flags)); // Sequence locks pass
    BOOST_CHECK(IsFinalTx(tx, chainActive.Tip()->nHeight + 2, chainActive.Tip()->GetMedianTimePast())); // Locktime passes on 2nd block

    // absolute time locked
    tx.vin[0].prevout.hash = txFirst[3]->GetHash();
    tx.nLockTime = chainActive.Tip()->GetMedianTimePast();
    prevheights.resize(1);
    prevheights[0] = baseheight + 4;
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Time(GetTime()).FromTx(tx));
    BOOST_CHECK(!CheckFinalTx(tx, flags)); // Locktime fails
    BOOST_CHECK(TestSequenceLocks(tx, flags)); // Sequence locks pass
    BOOST_CHECK(IsFinalTx(tx, chainActive.Tip()->nHeight + 2, chainActive.Tip()->GetMedianTimePast() + 1)); // Locktime passes 1 second later

    // mempool-dependent transactions (not added)
    tx.vin[0].prevout.hash = hash;
    prevheights[0] = chainActive.Tip()->nHeight + 1;
    tx.nLockTime = 0;
    tx.vin[0].nSequence = 0;
    BOOST_CHECK(CheckFinalTx(tx, flags)); // Locktime passes
    BOOST_CHECK(TestSequenceLocks(tx, flags)); // Sequence locks pass
    tx.vin[0].nSequence = 1;
    BOOST_CHECK(!TestSequenceLocks(tx, flags)); // Sequence locks fail
    tx.vin[0].nSequence = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG;
    BOOST_CHECK(TestSequenceLocks(tx, flags)); // Sequence locks pass
    tx.vin[0].nSequence = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG | 1;
    BOOST_CHECK(!TestSequenceLocks(tx, flags)); // Sequence locks fail

    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));

    // None of the of the absolute height/time locked tx should have made
    // it into the template because we still check IsFinalTx in CreateNewBlock,
    // but relative locked txs will if inconsistently added to mempool.
    // For now these will still generate a valid template until BIP68 soft fork
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 3U);
    // However if we advance height by 1 and time by 512, all of them should be mined
    for (int i = 0; i < CBlockIndex::nMedianTimeSpan; i++)
        chainActive.Tip()->GetAncestor(chainActive.Tip()->nHeight - i)->nTime += 512; //Trick the MedianTimePast
    chainActive.Tip()->nHeight++;
    SetMockTime(chainActive.Tip()->GetMedianTimePast() + 1);

    BOOST_CHECK(pblocktemplate = AssemblerForTest(chainparams).CreateNewBlock(scriptPubKey));
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 5U);

    chainActive.Tip()->nHeight--;
    SetMockTime(0);
    mempool.clear();

    TestPackageSelection(chainparams, scriptPubKey, txFirst);

    fCheckpointsEnabled = true;
}

BOOST_AUTO_TEST_SUITE_END()
