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
        {4, 0x23d58}, {2, 0x27838}, {1, 0x427de}, {1, 0x42aa1},
        {2, 0x466a0}, {2, 0x48ac2}, {1, 0x56269}, {2, 0x97cbd},
        {2, 0xa3d4b}, {1, 0xbac99}, {1, 0xcb215}, {2, 0xcec69},
        {2, 0xde449}, {1, 0xe00fe}, {2, 0xeee8c}, {2, 0xf7122},
        {1, 0x116a39}, {2, 0x11f89c}, {1, 0x132c64}, {1, 0x13d3e6},
        {3, 0x142956}, {2, 0x164559}, {2, 0x1a6ab0}, {1, 0x1b275f},
        {2, 0x1c1692}, {1, 0x1cae59}, {2, 0x1d0cda}, {2, 0x1e366b},
        {2, 0x2037c3}, {2, 0x22caaa}, {2, 0x2408fd}, {2, 0x242893},
        {1, 0x24417f}, {2, 0x255e98}, {2, 0x25cbe2}, {1, 0x25e78f},
        {2, 0x2642e0}, {1, 0x26fdca}, {2, 0x296d1d}, {1, 0x29ac70},
        {1, 0x2a4d02}, {3, 0x2b5511}, {2, 0x2d5a9b}, {5, 0x2f2848},
        {1, 0x2f5f55}, {5, 0x305555}, {1, 0x3237af}, {1, 0x32ec57},
        {1, 0x35125d}, {2, 0x35f7d3}, {1, 0x36e40b}, {1, 0x36e678},
        {1, 0x37610d}, {1, 0x3a4ca8}, {5, 0x3c5837}, {5, 0x3c88a6},
        {1, 0x3e83c4}, {1, 0x3f229c}, {6, 0x3fd68c}, {2, 0x3ff1d2},
        {2, 0x4018ba}, {1, 0x402771}, {1, 0x40ceeb}, {1, 0x41790a},
        {2, 0x44167b}, {2, 0x444d4f}, {1, 0x45260e}, {1, 0x452c51},
        {1, 0x46197c}, {5, 0x4863ef}, {5, 0x4882af}, {1, 0x48e0a1},
        {1, 0x492701}, {2, 0x49bb93}, {2, 0x4a04a6}, {1, 0x4c141c},
        {2, 0x4ceec3}, {1, 0x4d9ff8}, {2, 0x4edd05}, {2, 0x4feb95},
        {1, 0x51c750}, {1, 0x51df08}, {1, 0x534f92}, {5, 0x540255},
        {1, 0x56d82b}, {1, 0x57341e}, {1, 0x596211}, {1, 0x5a20da},
        {1, 0x5a73b2}, {1, 0x5be298}, {1, 0x5c536a}, {2, 0x5cd2e5},
        {0, 0x5d11f0}, {1, 0x5e8d8e}, {2, 0x601b4e}, {2, 0x60e427},
        {2, 0x6398e4}, {1, 0x6503dc}, {1, 0x654909}, {1, 0x65ee06},
        {1, 0x6d45d8}, {1, 0x6d8687}, {1, 0x70a86d}, {5, 0x710065},
        {2, 0x723fb9}, {1, 0x728e34}, {1, 0x7332d2}, {1, 0x739a88},
        {2, 0x73a483}, {2, 0x73b23a}, {4, 0x743e82}, {2, 0x771233},
        {1, 0x7730e3}, {1, 0x77aa84}, {2, 0x7a6b1c}, {2, 0x7acb1d},
        {1, 0x7b0e64}, {2, 0x7bbf88}, {2, 0x7dd39d}, {1, 0x7e90ef},
        {1, 0x7f4657}, {2, 0x7f6c27}, {2, 0x7fead4}, {1, 0x81a1f7},
        {2, 0x848f44}, {2, 0x8518d2}, {1, 0x859059}, {2, 0x865fd1},
        {1, 0x869232}, {1, 0x87c691}, {3, 0x88aa67}, {2, 0x88c769},
        {2, 0x893d8d}, {1, 0x8a87ad}, {2, 0x8ae16d}, {1, 0x8c19ec},
        {2, 0x8d8fd8}, {2, 0x8ddce1}, {2, 0x8f923e}, {2, 0x914364},
        {2, 0x91a420}, {2, 0x928008}, {1, 0x933884}, {2, 0x967fcb},
        {2, 0x969a03}, {1, 0x97d22d}, {2, 0x9bd256}, {1, 0x9bfb8d},
        {2, 0xa12adb}, {1, 0xa18f8f}, {1, 0xa21078}, {3, 0xa2c4b8},
        {2, 0xa32b8b}, {5, 0xa44ace}, {1, 0xa47b31}, {5, 0xa4820c},
        {1, 0xa4ff88}, {1, 0xa61b47}, {1, 0xa6fc80}, {2, 0xa7cfea},
        {1, 0xa92544}, {1, 0xaa4cc2}, {1, 0xaa60c0}, {1, 0xab0831},
        {5, 0xab3207}, {5, 0xadb66a}, {1, 0xae91ab}, {1, 0xb06991},
        {6, 0xb2cdc1}, {2, 0xb57e1d}, {2, 0xb65ab0}, {1, 0xb745cf},
        {1, 0xb78aa2}, {1, 0xb7e12e}, {2, 0xb83824}, {2, 0xb85349},
        {1, 0xba7341}, {1, 0xba7c65}, {1, 0xbade33}, {5, 0xbb105f},
        {5, 0xbc400c}, {1, 0xbcf01e}, {1, 0xbd028d}, {2, 0xbd3a13},
        {2, 0xbd3a4a}, {1, 0xbdddd1}, {2, 0xbef70f}, {1, 0xc17459},
        {2, 0xc41bff}, {2, 0xc42ea1}, {1, 0xc4ca07}, {1, 0xc5620d},
        {1, 0xc63c9e}, {5, 0xc6a4cf}, {1, 0xc7c224}, {1, 0xcb8152},
        {1, 0xcc693b}, {1, 0xcebfd2}, {1, 0xcfe441}, {1, 0xd0e485},
        {1, 0xd28078}, {2, 0xd3eea3}, {0, 0xd5229d}, {1, 0xd5c4c0},
        {2, 0xd6a25b}, {2, 0xd81a75}, {2, 0xd8e844}, {1, 0xd92906},
        {1, 0xd967c6}, {1, 0xd96a99}, {1, 0xda4a1e}, {1, 0xdb2025},
        {1, 0xdb383b}, {5, 0xdbc860}, {2, 0xdf6071}, {1, 0xe1ae69},
        {1, 0xe28ab4}, {1, 0xe2cba4}, {2, 0xe48491}, {2, 0xe58bed},
        {4, 0xe71ace}, {2, 0xe7a649}, {1, 0xe90c6c}, {1, 0xeb4fbf},
        {2, 0xeba95b}, {2, 0xec7c32}, {1, 0xef1474}, {2, 0xf0b6dc},
        {2, 0xf0d6e7}, {1, 0xf113f8}, {1, 0xf2790f}, {2, 0xf4ec08},
        {2, 0xf51dba}, {1, 0xf76ae3}, {2, 0xf81dbf}, {2, 0xf9715a},
        {1, 0xf98a52}, {2, 0xf99e94}, {1, 0xfca29b}, {1, 0xfec36c},
        {3, 0xffbd3c}, {2, 0x102d9df}, {2, 0x104715f}, {1, 0x1061012},
        {2, 0x10683eb}, {1, 0x107919b}, {2, 0x10793de}, {2, 0x108633c},
        {2, 0x108a3bf}, {2, 0x1095af5}, {2, 0x10ac416}, {2, 0x10b2cf9},
        {1, 0x10bd2c7}, {2, 0x10e1f03}, {2, 0x10f3f70}, {1, 0x1112256},
        {2, 0x1128bae}, {1, 0x113599b}, {2, 0x1146184}, {1, 0x11590b3},
        {1, 0x1162144}, {3, 0x11868bc}, {2, 0x11ad3e4}, {5, 0x11b61d3},
        {1, 0x11d2394}, {5, 0x11d957d}, {1, 0x11dab11}, {1, 0x11dc23d},
        {1, 0x11fbec8}, {2, 0x11fc27f}, {1, 0x1212f16}, {1, 0x123439b},
        {1, 0x129e2b4}, {1, 0x12aa389}, {5, 0x12add38}, {5, 0x12c727d},
        {1, 0x12cba36}, {1, 0x12e2d3e}, {6, 0x12e6666}, {2, 0x12f7b87},
        {2, 0x1307a36}, {1, 0x131d1ee}, {1, 0x1326162}, {1, 0x1334558},
        {2, 0x133dba3}, {2, 0x1344d2d}, {1, 0x1348ce9}, {1, 0x1352296},
        {1, 0x1355a6a}, {5, 0x137142a}, {5, 0x1375884}, {1, 0x137c211},
        {1, 0x137cf3c}, {2, 0x138199a}, {2, 0x1391c49}, {1, 0x139685e},
        {2, 0x139a9b6}, {1, 0x13aa90e}, {2, 0x13c086a}, {2, 0x14020de},
        {1, 0x1404ab9}, {1, 0x140ea2a}, {1, 0x14206c2}, {5, 0x144771f},
        {1, 0x1447f3d}, {1, 0x1459e07}, {1, 0x1466c13}, {1, 0x14bf4f8},
        {1, 0x14c4274}, {1, 0x14cc033}, {1, 0x14ee8b6}, {2, 0x15189ab},
        {0, 0x153771a}, {1, 0x153f783}, {2, 0x157ca89}, {2, 0x157e842},
        {2, 0x1588edc}, {1, 0x1593751}, {1, 0x1595a0e}, {1, 0x159de0f},
        {1, 0x159fd12}, {1, 0x15a0817}, {1, 0x15a3555}, {5, 0x15f926d},
        {2, 0x1628f18}, {1, 0x163f01c}, {1, 0x1641966}, {1, 0x1647cb1},
        {2, 0x164b81c}, {2, 0x1654fa2}, {4, 0x1658b12}, {2, 0x165a1c9},
        {1, 0x169190f}, {1, 0x16b17ce}, {2, 0x16dba2e}, {2, 0x16dca30},
        {1, 0x16f60a8}, {2, 0x16f6667}, {2, 0x16f75d5}, {1, 0x16f78bf},
        {1, 0x1731c0c}, {2, 0x174801a}, {2, 0x174ea14}, {1, 0x1753b88},
        {2, 0x175eac3}, {2, 0x176d408}, {1, 0x17821b5}, {2, 0x17c884f},
        {1, 0x18756b4}, {1, 0x18835e5}, {3, 0x1889e81}, {2, 0x1896aa8},
        {2, 0x189d16c}, {1, 0x18b7487}, {2, 0x18c18d0}, {1, 0x18d0264},
        {2, 0x18eb623}, {2, 0x18f97c6}, {2, 0x18f9c2e}, {2, 0x18ffa0e},
        {2, 0x191e845}, {2, 0x1929a10}, {1, 0x192d204}, {2, 0x19388e3},
        {2, 0x19796ea}, {1, 0x197fdca}, {2, 0x198544c}, {1, 0x198a0ad},
        {2, 0x198f173}, {1, 0x1991e00}, {1, 0x199fce2}, {3, 0x19a9e1d},
        {2, 0x19bec90}, {5, 0x19ced25}, {1, 0x19d21c1}, {5, 0x19db109},
        {1, 0x19f2b98}, {1, 0x1a03776}, {1, 0x1a03ffb}, {2, 0x1a0fbdc},
        {1, 0x1a1177e}, {1, 0x1a12c7d}, {1, 0x1a1ef9e}, {1, 0x1a1f0af},
        {5, 0x1a25564}, {5, 0x1a26056}, {1, 0x1a2a31d}, {1, 0x1a6a0a1},
        {6, 0x1a7c8b3}, {2, 0x1a7cfa2}, {2, 0x1a99439}, {1, 0x1aa29f5},
        {1, 0x1aaba24}, {1, 0x1ab7f69}, {2, 0x1abc15d}, {2, 0x1abc6eb},
        {1, 0x1af73fd}, {1, 0x1af9b2f}, {1, 0x1b09637}, {5, 0x1b0ca3b},
        {5, 0x1b0eff6}, {1, 0x1b22fb2}, {1, 0x1b26af7}, {2, 0x1b2e2fd},
        {2, 0x1b2f511}, {1, 0x1b36e09}, {2, 0x1b3a087}, {1, 0x1b4c358},
        {2, 0x1b502ff}, {2, 0x1b52ba2}, {1, 0x1b5e19a}, {1, 0x1b67885},
        {1, 0x1b7a187}, {5, 0x1b7eb14}, {1, 0x1b96f87}, {1, 0x1ba54b3},
        {1, 0x1ba6dc8}, {1, 0x1bb0aef}, {1, 0x1bb668b}, {1, 0x1bb7019},
        {1, 0x1be3d2f}, {2, 0x1bf5e80}, {0, 0x1c14d56}, {1, 0x1c1d614},
        {2, 0x1c21698}, {2, 0x1c581f8}, {2, 0x1c5ee6e}, {1, 0x1cc136f},
        {1, 0x1cd126e}, {1, 0x1cd49ed}, {1, 0x1d0c176}, {1, 0x1d0de62},
        {1, 0x1d1c740}, {5, 0x1d1ef10}, {2, 0x1d23552}, {1, 0x1d31f2e},
        {1, 0x1d3ef64}, {1, 0x1d3f090}, {2, 0x1d54091}, {2, 0x1d76f9f},
        {4, 0x1d7dac9}, {2, 0x1d85ed9}, {1, 0x1d8a299}, {1, 0x1d9836f},
        {2, 0x1db11af}, {2, 0x1dc1386}, {1, 0x1ddb583}, {2, 0x1ddf8b3},
        {2, 0x1de9a3a}, {1, 0x1df38f6}, {1, 0x1e22c78}, {2, 0x1e2b215},
        {2, 0x1e49594}, {1, 0x1e5849d}, {2, 0x1e5ece7}, {2, 0x1e6e319},
        {1, 0x1e78eee}, {2, 0x1e7d1ef}, {1, 0x1e8bff6}, {1, 0x1ea228f},
        {3, 0x1ea6c91}, {2, 0x1ea796c}, {2, 0x1edd826}, {1, 0x1eec9dd},
        {2, 0x1f05884}, {1, 0x1f2765e}, {2, 0x1f391e1}, {2, 0x1f3c4ba},
        {2, 0x1f5474a}, {2, 0x1f66fac}, {2, 0x1f69ca8}, {2, 0x1f7b390},
        {1, 0x1fa9510}, {2, 0x1faa32e}, {2, 0x1fb634e}, {1, 0x1fc1ddc},
        {2, 0x1fdd4a7}, {1, 0x1ffbcc7}, {2, 0x1ffe15b}, {1, 0x20090a0},
        {1, 0x2076d15}, {3, 0x207d8c7}, {2, 0x208e980}, {5, 0x20982a0},
        {1, 0x20aebc5}, {5, 0x20b7a13}, {1, 0x20c54df}, {1, 0x20d4739},
        {1, 0x20e6aef}, {2, 0x20ea06f}, {1, 0x20edba1}, {1, 0x20f2347},
        {1, 0x210a49d}, {1, 0x211ccdb}, {5, 0x21381a2}, {5, 0x2138515},
        {1, 0x2142c8d}, {1, 0x216537d}, {6, 0x217af73}, {2, 0x218ca34},
        {2, 0x219d590}, {1, 0x21a959c}, {1, 0x21ad7ac}, {1, 0x21b742f},
        {2, 0x21beaae}, {2, 0x21c5b71}, {1, 0x21cecab}, {1, 0x21d137e},
        {1, 0x21d196a}, {5, 0x21dd089}, {5, 0x21deba5}, {1, 0x2209d7e},
        {1, 0x220e274}, {2, 0x221d589}, {2, 0x2229316}, {1, 0x22391dc},
        {2, 0x22551a2}, {1, 0x225700f}, {2, 0x2265217}, {2, 0x226ca2e},
        {1, 0x228871c}, {1, 0x228e5e6}, {1, 0x22b69c7}, {5, 0x22c69cd},
        {1, 0x22ced72}, {1, 0x22d2916}, {1, 0x23187b8}, {1, 0x231bf4c},
        {1, 0x2335e3a}, {1, 0x233b518}, {1, 0x2343203}, {2, 0x234378d},
        {0, 0x234b477}, {1, 0x23531f8}, {2, 0x235fb61}, {2, 0x23603c3},
        {2, 0x2360650}, {1, 0x2375f98}, {1, 0x23861af}, {1, 0x23a0fb0},
        {1, 0x23ab420}, {1, 0x23b7c33}, {1, 0x23bb973}, {5, 0x23d2293},
        {2, 0x23f1bbf}, {1, 0x2409bf6}, {1, 0x240ad74}, {1, 0x2427711},
        {2, 0x2428557}, {2, 0x2431a18},
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
