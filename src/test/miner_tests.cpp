// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "coins.h"
#include "consensus/consensus.h"
#include "consensus/merkle.h"
#include "consensus/validation.h"
#include "validation.h"
#include "miner.h"
#include "policy/policy.h"
#include "pubkey.h"
#include "script/standard.h"
#include "txmempool.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"

#include "test/test_bitcoin.h"

#include <memory>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(miner_tests, TestingSetup)

static CFeeRate blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);

static
struct {
    unsigned char extranonce;
    unsigned int nonce;
} blockinfo[] = {
        {4, 0xf738}, {2, 0x1443f}, {1, 0x1791c}, {1, 0x4680d},
        {2, 0x49026}, {2, 0x565a0}, {1, 0x5741d}, {2, 0x63cfc},
        {2, 0xa0f88}, {1, 0xa567b}, {1, 0xac610}, {2, 0xca561},
        {2, 0xcb49c}, {1, 0xd0f62}, {2, 0xda054}, {2, 0xfc5dc},
        {1, 0x101072}, {2, 0x10b7c5}, {1, 0x1102a1}, {1, 0x11edb6},
        {3, 0x12088d}, {2, 0x12aa04}, {2, 0x15122a}, {1, 0x154289},
        {2, 0x16a541}, {1, 0x1794f8}, {2, 0x183811}, {2, 0x1933df},
        {2, 0x1be5c0}, {2, 0x1d72d6}, {2, 0x1f9848}, {2, 0x1fb28f},
        {1, 0x201d33}, {2, 0x205542}, {2, 0x207e07}, {1, 0x213628},
        {2, 0x226901}, {1, 0x242717}, {2, 0x24a9ab}, {1, 0x257a45},
        {1, 0x25ca42}, {3, 0x262394}, {2, 0x275158}, {5, 0x280c90},
        {1, 0x284ccd}, {5, 0x2c4560}, {1, 0x2c7b86}, {1, 0x2c95dc},
        {1, 0x2dc136}, {2, 0x2dcb69}, {1, 0x2e2613}, {1, 0x306df5},
        {1, 0x311f69}, {1, 0x3147ee}, {5, 0x330e55}, {5, 0x3360a6},
        {1, 0x33b45c}, {1, 0x352d7e}, {6, 0x35a86e}, {2, 0x35d31e},
        {2, 0x3911af}, {1, 0x3c51d3}, {1, 0x3d0b43}, {1, 0x3e418e},
        {2, 0x3e9dfa}, {2, 0x4042c5}, {1, 0x40dc54}, {1, 0x413544},
        {1, 0x425530}, {5, 0x435f93}, {5, 0x43cdd1}, {1, 0x46666b},
        {1, 0x4723ab}, {2, 0x476407}, {2, 0x47e005}, {1, 0x4827a8},
        {2, 0x49ab8d}, {1, 0x4a1ce8}, {2, 0x4bb7fa}, {2, 0x4f3045},
        {1, 0x51864d}, {1, 0x52a0ce}, {1, 0x52e0b5}, {5, 0x53b4b6},
        {1, 0x53b4f3}, {1, 0x54310e}, {1, 0x55747b}, {1, 0x559376},
        {1, 0x55eeb3}, {1, 0x589938}, {1, 0x5906d4}, {2, 0x5a8c8a},
        {0, 0x5cb39f}, {1, 0x5f04a5}, {2, 0x5f19a1}, {2, 0x5f640c},
        {2, 0x5f7bb2}, {1, 0x61204d}, {1, 0x64db21}, {1, 0x672a4d},
        {1, 0x68c4ee}, {1, 0x69f98a}, {1, 0x6bd05e}, {5, 0x6ce7fd},
        {2, 0x6e0238}, {1, 0x6e1caf}, {1, 0x6e46c9}, {1, 0x6fa854},
        {2, 0x72ba33}, {2, 0x72e5cc}, {4, 0x732ada}, {2, 0x737864},
        {1, 0x755532}, {1, 0x75693b}, {2, 0x763f69}, {2, 0x7680f2},
        {1, 0x780e6f}, {2, 0x7aa7c4}, {2, 0x7bfde7}, {1, 0x7e8173},
        {1, 0x7e9049}, {2, 0x7ef10c}, {2, 0x7f1672}, {1, 0x7f31c8},
        {2, 0x8057a1}, {2, 0x817b2a}, {1, 0x817e20}, {2, 0x824b89},
        {1, 0x8311d3}, {1, 0x84f1cd}, {3, 0x85c9ee}, {2, 0x8651fb},
        {2, 0x86da7a}, {1, 0x8998a0}, {2, 0x8a38ee}, {1, 0x8c348f},
        {2, 0x8c565b}, {2, 0x8de749}, {2, 0x8eb3bd}, {2, 0x8f1fca},
        {2, 0x90fabc}, {2, 0x913885}, {1, 0x934a35}, {2, 0x95365e},
        {2, 0x969c95}, {1, 0x977faf}, {2, 0x97ff21}, {1, 0x987e01},
        {2, 0x9a42f6}, {1, 0x9b4058}, {1, 0x9c4085}, {3, 0x9e4039},
        {2, 0x9eccee}, {5, 0xa3f79d}, {1, 0xa430a6}, {5, 0xa483ed},
        {1, 0xa585a5}, {1, 0xa63231}, {1, 0xa6f061}, {2, 0xa78c8c},
        {1, 0xacbf5e}, {1, 0xad6d58}, {1, 0xaebb01}, {1, 0xaee03f},
        {5, 0xaf993d}, {5, 0xb44fb7}, {1, 0xb4fc12}, {1, 0xb51e15},
        {6, 0xb59758}, {2, 0xb70da9}, {2, 0xb7a5b6}, {1, 0xb7e036},
        {1, 0xb8018f}, {1, 0xb87a4a}, {2, 0xb90e56}, {2, 0xb9a38f},
        {1, 0xba9f91}, {1, 0xbb1446}, {1, 0xbda0b1}, {5, 0xbebb40},
        {5, 0xbef8e3}, {1, 0xc04b81}, {1, 0xc0c21b}, {2, 0xc1a8c8},
        {2, 0xc20ab5}, {1, 0xc68f1f}, {2, 0xc83133}, {1, 0xc873c6},
        {2, 0xc8c54e}, {2, 0xca1429}, {1, 0xcc7fe9}, {1, 0xcd1d55},
        {1, 0xce2ec0}, {5, 0xce741e}, {1, 0xcf551e}, {1, 0xcf566b},
        {1, 0xcf6b55}, {1, 0xcf70cd}, {1, 0xd32367}, {1, 0xd47bbc},
        {1, 0xd72035}, {2, 0xdc9c14}, {0, 0xdd15a3}, {1, 0xddb65a},
        {2, 0xe11032}, {2, 0xe1430b}, {2, 0xe1b094}, {1, 0xe25307},
        {1, 0xe2b8cb}, {1, 0xe4848c}, {1, 0xe4ef54}, {1, 0xe51189},
        {1, 0xe6ad10}, {5, 0xe6c3dd}, {2, 0xe8ed89}, {1, 0xea1880},
        {1, 0xeb62ce}, {1, 0xec8449}, {2, 0xecbd14}, {2, 0xefbc4e},
        {4, 0xf0f29a}, {2, 0xf266e2}, {1, 0xf2fcf1}, {1, 0xf49cb7},
        {2, 0xf819fc}, {2, 0xf860a2}, {1, 0xfa7837}, {2, 0xfd627c},
        {2, 0xfe94da}, {1, 0xfed7a2}, {1, 0xffa301}, {2, 0xffb5e2},
        {2, 0x10000a2}, {1, 0x1023c78}, {2, 0x103ae2c}, {2, 0x1043892},
        {1, 0x106a11b}, {2, 0x106cd32}, {1, 0x1087a1d}, {1, 0x108d7c1},
        {3, 0x1096147}, {2, 0x10b62fc}, {2, 0x10c0a9c}, {1, 0x10c88be},
        {2, 0x10c8f4a}, {1, 0x10f28fc}, {2, 0x110ee0c}, {2, 0x111ae92},
        {2, 0x112a396}, {2, 0x113ade8}, {2, 0x1143eb5}, {2, 0x1157d24},
        {1, 0x1165146}, {2, 0x116b326}, {2, 0x1172998}, {1, 0x118008a},
        {2, 0x118d721}, {1, 0x118e578}, {2, 0x11a3a1a}, {1, 0x11d1f47},
        {1, 0x11f5715}, {3, 0x11fc253}, {2, 0x1201ff8}, {5, 0x1206c3b},
        {1, 0x121ecd7}, {5, 0x1225d5a}, {1, 0x122eaba}, {1, 0x12bb537},
        {1, 0x12bd2e3}, {2, 0x12e2daa}, {1, 0x130750a}, {1, 0x1335559},
        {1, 0x1362ecb}, {1, 0x1364279}, {5, 0x1372632}, {5, 0x1376b8c},
        {1, 0x13b1a78}, {1, 0x13b6a50}, {6, 0x13c2a2a}, {2, 0x13d1f5d},
        {2, 0x13d2d2d}, {1, 0x13d3aae}, {1, 0x13d6365}, {1, 0x13d8711},
        {2, 0x13f41b7}, {2, 0x13f7b9d}, {1, 0x13ff6c7}, {1, 0x1403c8e},
        {1, 0x140c8d3}, {5, 0x141f30f}, {5, 0x14281ac}, {1, 0x1431d1d},
        {1, 0x1431fa4}, {2, 0x143f553}, {2, 0x144312d}, {1, 0x1449f7d},
        {2, 0x144ed80}, {1, 0x145a50a}, {2, 0x147c968}, {2, 0x1494228},
        {1, 0x1499c52}, {1, 0x14c8355}, {1, 0x14ccdd5}, {5, 0x14d0335},
        {1, 0x14ddbbc}, {1, 0x14ed352}, {1, 0x14f5c56}, {1, 0x150b50c},
        {1, 0x150e494}, {1, 0x151cd09}, {1, 0x152d08c}, {2, 0x153ca41},
        {0, 0x153d58b}, {1, 0x1543b71}, {2, 0x15443b1}, {2, 0x1577f60},
        {2, 0x1579506}, {1, 0x15900b1}, {1, 0x15907d8}, {1, 0x1599ff4},
        {1, 0x159c811}, {1, 0x15ab7d2}, {1, 0x15b025f}, {5, 0x15c7ab0},
        {2, 0x15d64e4}, {1, 0x15d9bdc}, {1, 0x15e2db0}, {1, 0x15f2c3d},
        {2, 0x15f9247}, {2, 0x15fc4c9}, {4, 0x15fed1d}, {2, 0x1637464},
        {1, 0x1648ad0}, {1, 0x1663e1a}, {2, 0x1673f84}, {2, 0x16b9bba},
        {1, 0x16f7f29}, {2, 0x1715979}, {2, 0x1728eeb}, {1, 0x17358bf},
        {1, 0x1765a15}, {2, 0x177c476}, {2, 0x1783a8a}, {1, 0x179a82f},
        {2, 0x17a31a5}, {2, 0x17bf61e}, {1, 0x17d4338}, {2, 0x17e072c},
        {1, 0x17e0cde}, {1, 0x17f1324}, {3, 0x1812496}, {2, 0x1816b7d},
        {2, 0x1819bef}, {1, 0x181f75b}, {2, 0x1820eb5}, {1, 0x183b2de},
        {2, 0x1847086}, {2, 0x184c00b}, {2, 0x187f407}, {2, 0x1883527},
        {2, 0x188f4b4}, {2, 0x18a6474}, {1, 0x18c0217}, {2, 0x18eae58},
        {2, 0x191ad62}, {1, 0x191ede6}, {2, 0x192013b}, {1, 0x1943228},
        {2, 0x194474b}, {1, 0x194b836}, {1, 0x19599a0}, {3, 0x19633ca},
        {2, 0x196e3b8}, {5, 0x196ecbe}, {1, 0x198334e}, {5, 0x1984e1d},
        {1, 0x1985d14}, {1, 0x198f75f}, {1, 0x1999593}, {2, 0x19c11b2},
        {1, 0x19e63a1}, {1, 0x19f0891}, {1, 0x19fdea7}, {1, 0x1a349eb},
        {5, 0x1a721b8}, {5, 0x1a7cf18}, {1, 0x1a90cfd}, {1, 0x1aa79bf},
        {6, 0x1abe407}, {2, 0x1ae1ae5}, {2, 0x1ae812f}, {1, 0x1b0c408},
        {1, 0x1b203c1}, {1, 0x1b22dd4}, {2, 0x1b231b8}, {2, 0x1b281f0},
        {1, 0x1b30a64}, {1, 0x1b3d80d}, {1, 0x1b3fafe}, {5, 0x1b65cb9},
        {5, 0x1b68cd4}, {1, 0x1b7f7b3}, {1, 0x1b91933}, {2, 0x1baf8e5},
        {2, 0x1bbc017}, {1, 0x1bbfc93}, {2, 0x1bc1f1e}, {1, 0x1bc5ddf},
        {2, 0x1bc8557}, {2, 0x1bd3b9c}, {1, 0x1bd9281}, {1, 0x1be071e},
        {1, 0x1c02ddb}, {5, 0x1c0d12f}, {1, 0x1c1fb93}, {1, 0x1c3064c},
        {1, 0x1c549f8}, {1, 0x1c564c6}, {1, 0x1c61d9c}, {1, 0x1c8e865},
        {1, 0x1c93778}, {2, 0x1c956f0}, {0, 0x1c9a23c}, {1, 0x1cecbd7},
        {2, 0x1cf1ad2}, {2, 0x1cf3f58}, {2, 0x1cf8831}, {1, 0x1d2f850},
        {1, 0x1d45a7e}, {1, 0x1d4ef73}, {1, 0x1d6c522}, {1, 0x1d910b6},
        {1, 0x1d99ff8}, {5, 0x1da099b}, {2, 0x1da5e98}, {1, 0x1dbc005},
        {1, 0x1dc7cb9}, {1, 0x1dc84a4}, {2, 0x1defe13}, {2, 0x1df9f1f},
        {4, 0x1dff649}, {2, 0x1e03677}, {1, 0x1e19f18}, {1, 0x1e20ba3},
        {2, 0x1e253e5}, {2, 0x1e296f7}, {1, 0x1e59d6b}, {2, 0x1e61b74},
        {2, 0x1e65e07}, {1, 0x1e723b4}, {1, 0x1e770e1}, {2, 0x1e7e2d2},
        {2, 0x1e8f3fc}, {1, 0x1ea95c3}, {2, 0x1eab873}, {2, 0x1eb6ae5},
        {1, 0x1ecae42}, {2, 0x1ede65d}, {1, 0x1ef4af3}, {1, 0x1efa386},
        {3, 0x1f0f4c5}, {2, 0x1f1ca65}, {2, 0x1f2170f}, {1, 0x1f34742},
        {2, 0x1f3ea3d}, {1, 0x1f52e69}, {2, 0x1f57b2d}, {2, 0x1f69941},
        {2, 0x1f75d3d}, {2, 0x1f85f67}, {2, 0x1f90d21}, {2, 0x1fb4f0e},
        {1, 0x1fc9595}, {2, 0x1fcb68f}, {2, 0x1fdb04f}, {1, 0x1ff5e31},
        {2, 0x1ff6672}, {1, 0x1ff9e82}, {2, 0x20104dd}, {1, 0x206f220},
        {1, 0x2071bc7}, {3, 0x207b21c}, {2, 0x2092b17}, {5, 0x20aa7f6},
        {1, 0x20b2720}, {5, 0x20cb821}, {1, 0x20d0f7a}, {1, 0x20de7ee},
        {1, 0x20e789d}, {2, 0x20ea305}, {1, 0x20f623d}, {1, 0x210f6ec},
        {1, 0x2117241}, {1, 0x212b9f4}, {5, 0x213a9b7}, {5, 0x21423e7},
        {1, 0x214a485}, {1, 0x216682a}, {6, 0x2167881}, {2, 0x2169b00},
        {2, 0x2171aaa}, {1, 0x2178cf1}, {1, 0x217bfe1}, {1, 0x2180b15},
        {2, 0x218aa68}, {2, 0x2192a7b}, {1, 0x219b739}, {1, 0x219cd5b},
        {1, 0x21b1d10}, {5, 0x21c8927}, {5, 0x21e9a79}, {1, 0x2202aad},
        {1, 0x220dcc3}, {2, 0x220ee7c}, {2, 0x221618d}, {1, 0x22275fa},
        {2, 0x222cd12}, {1, 0x2239ccc}, {2, 0x223df99}, {2, 0x2257943},
        {1, 0x2258b33}, {1, 0x225deb6}, {1, 0x2288e43}, {5, 0x22c82b0},
        {1, 0x22d66dd}, {1, 0x23015dd}, {1, 0x2315d7d}, {1, 0x231c2b9},
        {1, 0x2335828}, {1, 0x2345822}, {1, 0x234dbc8}, {2, 0x2351f3f},
        {0, 0x235b847}, {1, 0x23619d3}, {2, 0x2376a88}, {2, 0x237d169},
        {2, 0x238c8a1}, {1, 0x23911be}, {1, 0x239d2f3}, {1, 0x23b5b93},
        {1, 0x23b930b}, {1, 0x23be039}, {1, 0x23cc2d9}, {5, 0x23dadcb},
        {2, 0x24081df}, {1, 0x2408345}, {1, 0x244917a}, {1, 0x244a6ad},
        {2, 0x2473670}, {2, 0x247b3aa},

};

CBlockIndex CreateBlockIndex(int nHeight)
{
    CBlockIndex index;
    index.nHeight = nHeight;
    index.pprev = chainActive.Tip();
    return index;
}

bool TestSequenceLocks(const CTransaction &tx, int flags)
{
    LOCK(mempool.cs);
    return CheckSequenceLocks(tx, flags);
}

// Test suite for ancestor feerate transaction selection.
// Implemented as an additional function, rather than a separate test case,
// to allow reusing the blockchain created in CreateNewBlock_validity.
// Note that this test assumes blockprioritysize is 0.
void TestPackageSelection(const CChainParams& chainparams, CScript scriptPubKey, std::vector<CTransactionRef>& txFirst)
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
    tx.vout[0].nValue = 5000000000LL - 1000;
    // This tx has a low fee: 1000 satoshis
    uint256 hashParentTx = tx.GetHash(); // save this txid for later use
    mempool.addUnchecked(hashParentTx, entry.Fee(1000).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));

    // This tx has a medium fee: 10000 satoshis
    tx.vin[0].prevout.hash = txFirst[1]->GetHash();
    tx.vout[0].nValue = 5000000000LL - 10000;
    uint256 hashMediumFeeTx = tx.GetHash();
    mempool.addUnchecked(hashMediumFeeTx, entry.Fee(10000).Time(GetTime()).SpendsCoinbase(true).FromTx(tx));

    // This tx has a high fee, but depends on the first transaction
    tx.vin[0].prevout.hash = hashParentTx;
    tx.vout[0].nValue = 5000000000LL - 1000 - 50000; // 50k satoshi fee
    uint256 hashHighFeeTx = tx.GetHash();
    mempool.addUnchecked(hashHighFeeTx, entry.Fee(50000).Time(GetTime()).SpendsCoinbase(false).FromTx(tx));

    std::unique_ptr<CBlockTemplate> pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey);
    BOOST_CHECK(pblocktemplate->block.vtx[1]->GetHash() == hashParentTx);
    BOOST_CHECK(pblocktemplate->block.vtx[2]->GetHash() == hashHighFeeTx);
    BOOST_CHECK(pblocktemplate->block.vtx[3]->GetHash() == hashMediumFeeTx);

    // Test that a package below the block min tx fee doesn't get included
    tx.vin[0].prevout.hash = hashHighFeeTx;
    tx.vout[0].nValue = 5000000000LL - 1000 - 50000; // 0 fee
    uint256 hashFreeTx = tx.GetHash();
    mempool.addUnchecked(hashFreeTx, entry.Fee(0).FromTx(tx));
    size_t freeTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);

    // Calculate a fee on child transaction that will put the package just
    // below the block min tx fee (assuming 1 child tx of the same size).
    CAmount feeToUse = blockMinFeeRate.GetFee(2*freeTxSize) - 1;

    tx.vin[0].prevout.hash = hashFreeTx;
    tx.vout[0].nValue = 5000000000LL - 1000 - 50000 - feeToUse;
    uint256 hashLowFeeTx = tx.GetHash();
    mempool.addUnchecked(hashLowFeeTx, entry.Fee(feeToUse).FromTx(tx));
    pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey);
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
    pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey);
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
    pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey);

    // Verify that this tx isn't selected.
    for (size_t i=0; i<pblocktemplate->block.vtx.size(); ++i) {
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashFreeTx2);
        BOOST_CHECK(pblocktemplate->block.vtx[i]->GetHash() != hashLowFeeTx2);
    }

    // This tx will be mineable, and should cause hashLowFeeTx2 to be selected
    // as well.
    tx.vin[0].prevout.n = 1;
    tx.vout[0].nValue = 100000000 - 10000; // 10k satoshi fee
    mempool.addUnchecked(tx.GetHash(), entry.Fee(10000).FromTx(tx));
    pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey);
    BOOST_CHECK(pblocktemplate->block.vtx[8]->GetHash() == hashLowFeeTx2);
}

// NOTE: These tests rely on CreateNewBlock doing its own self-validation!
BOOST_AUTO_TEST_CASE(CreateNewBlock_validity)
{
    // Note that by default, these tests run with size accounting enabled.
    const CChainParams& chainparams = Params(CBaseChainParams::MAIN);
    CScript scriptPubKey = CScript() << ParseHex("040d61d8653448c98731ee5fffd303c15e71ec2057b77f11ab3601979728cdaff2d68afbba14e4fa0bc44f2072b0b23ef63717f8cdfbe58dcd33f32b6afe98741a") << OP_CHECKSIG;
    std::unique_ptr<CBlockTemplate> pblocktemplate;
    CMutableTransaction tx,tx2;
    CScript script;
    uint256 hash;
    TestMemPoolEntryHelper entry;
    entry.nFee = 11;
    entry.dPriority = 111.0;
    entry.nHeight = 11;

    LOCK(cs_main);
    fCheckpointsEnabled = false;

    // Simple block creation, nothing special yet:
    BOOST_CHECK(pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey));

    // We can't make transactions until we have inputs
    // Therefore, load 100 blocks :)
    int baseheight = 0;
    std::vector<CTransactionRef> txFirst;
    for (unsigned int i = 0; i < sizeof(blockinfo)/sizeof(*blockinfo); ++i)
    {
        CBlock *pblock = &pblocktemplate->block; // pointer for convenience
        pblock->nVersion = 4; //use version 4 as we enable BIP34, BIP65 and BIP66 since genesis
        pblock->nTime = chainActive.Tip()->GetMedianTimePast()+1+i;
        CMutableTransaction txCoinbase(*pblock->vtx[0]);
        txCoinbase.nVersion = 1;
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
        std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(*pblock);
        BOOST_CHECK(ProcessNewBlock(chainparams, shared_pblock, true, NULL));
        pblock->hashPrevBlock = pblock->GetHash();
    }

    // Just to make sure we can still make simple blocks
    BOOST_CHECK(pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey));

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
        bool spendsCoinbase = (i == 0) ? true : false; // only first tx spends coinbase
        // If we don't set the # of sig ops in the CTxMemPoolEntry, template creation fails
        mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(spendsCoinbase).FromTx(tx));
        tx.vin[0].prevout.hash = hash;
    }
    BOOST_CHECK_THROW(BlockAssembler(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error);
    mempool.clear();

    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vout[0].nValue = BLOCKSUBSIDY;
    for (unsigned int i = 0; i < 1001; ++i)
    {
        tx.vout[0].nValue -= LOWFEE;
        hash = tx.GetHash();
        bool spendsCoinbase = (i == 0) ? true : false; // only first tx spends coinbase
        // If we do set the # of sig ops in the CTxMemPoolEntry, template creation passes
        mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(spendsCoinbase).SigOpsCost(80).FromTx(tx));
        tx.vin[0].prevout.hash = hash;
    }
    BOOST_CHECK(pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey));
    mempool.clear();

    // block size > limit
    tx.vin[0].scriptSig = CScript();
    // 18 * (520char + DROP) + OP_1 = 9433 bytes
    std::vector<unsigned char> vchData(520);
    for (unsigned int i = 0; i < 18; ++i)
        tx.vin[0].scriptSig << vchData << OP_DROP;
    tx.vin[0].scriptSig << OP_1;
    tx.vin[0].prevout.hash = txFirst[0]->GetHash();
    tx.vout[0].nValue = BLOCKSUBSIDY;
    for (unsigned int i = 0; i < 128; ++i)
    {
        tx.vout[0].nValue -= LOWFEE;
        hash = tx.GetHash();
        bool spendsCoinbase = (i == 0) ? true : false; // only first tx spends coinbase
        mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(spendsCoinbase).FromTx(tx));
        tx.vin[0].prevout.hash = hash;
    }
    BOOST_CHECK(pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey));
    mempool.clear();

    // orphan in mempool, template creation fails
    hash = tx.GetHash();
    mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).FromTx(tx));
    BOOST_CHECK_THROW(BlockAssembler(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error);
    mempool.clear();

    // child with higher priority than parent
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
    BOOST_CHECK(pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey));
    mempool.clear();

    // coinbase in mempool, template creation fails
    tx.vin.resize(1);
    tx.vin[0].prevout.SetNull();
    tx.vin[0].scriptSig = CScript() << OP_0 << OP_1;
    tx.vout[0].nValue = 0;
    hash = tx.GetHash();
    // give it a fee so it'll get mined
    mempool.addUnchecked(hash, entry.Fee(LOWFEE).Time(GetTime()).SpendsCoinbase(false).FromTx(tx));
    BOOST_CHECK_THROW(BlockAssembler(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error);
    mempool.clear();

    // invalid (pre-p2sh) txn in mempool, template creation fails
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
    BOOST_CHECK_THROW(BlockAssembler(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error);
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
    BOOST_CHECK_THROW(BlockAssembler(chainparams).CreateNewBlock(scriptPubKey), std::runtime_error);
    mempool.clear();

    // subsidy changing
    int nHeight = chainActive.Height();
    // Create an actual 209999-long block chain (without valid blocks).
    while (chainActive.Tip()->nHeight < 209999) {
        CBlockIndex* prev = chainActive.Tip();
        CBlockIndex* next = new CBlockIndex();
        next->phashBlock = new uint256(GetRandHash());
        pcoinsTip->SetBestBlock(next->GetBlockHash());
        next->pprev = prev;
        next->nHeight = prev->nHeight + 1;
        next->BuildSkip();
        chainActive.SetTip(next);
    }
    BOOST_CHECK(pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey));
    // Extend to a 210000-long block chain.
    while (chainActive.Tip()->nHeight < 210000) {
        CBlockIndex* prev = chainActive.Tip();
        CBlockIndex* next = new CBlockIndex();
        next->phashBlock = new uint256(GetRandHash());
        pcoinsTip->SetBestBlock(next->GetBlockHash());
        next->pprev = prev;
        next->nHeight = prev->nHeight + 1;
        next->BuildSkip();
        chainActive.SetTip(next);
    }
    BOOST_CHECK(pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey));
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

    BOOST_CHECK(pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey));

    // None of the of the absolute height/time locked tx should have made
    // it into the template because we still check IsFinalTx in CreateNewBlock,
    // but relative locked txs will if inconsistently added to mempool.
    // For now these will still generate a valid template until BIP68 soft fork
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 3);
    // However if we advance height by 1 and time by 512, all of them should be mined
    for (int i = 0; i < CBlockIndex::nMedianTimeSpan; i++)
        chainActive.Tip()->GetAncestor(chainActive.Tip()->nHeight - i)->nTime += 512; //Trick the MedianTimePast
    chainActive.Tip()->nHeight++;
    SetMockTime(chainActive.Tip()->GetMedianTimePast() + 1);

    BOOST_CHECK(pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey));
    BOOST_CHECK_EQUAL(pblocktemplate->block.vtx.size(), 5);

    chainActive.Tip()->nHeight--;
    SetMockTime(0);
    mempool.clear();

    TestPackageSelection(chainparams, scriptPubKey, txFirst);

    fCheckpointsEnabled = true;
}

BOOST_AUTO_TEST_SUITE_END()
