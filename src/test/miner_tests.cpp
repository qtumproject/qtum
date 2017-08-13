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
        {4, 0x00022d0e}, {2, 0x00016747}, {1, 0x000236f8}, {1, 0x0002467c},
        {2, 0x0004db73}, {2, 0x00047295}, {1, 0x0004c551}, {2, 0x0006abd9},
        {2, 0x0007b3ad}, {1, 0x00087881}, {1, 0x000ada76}, {2, 0x000a9176},
        {2, 0x000d510b}, {1, 0x000cf304}, {2, 0x000bfb9f}, {2, 0x000fd2c4},
        {1, 0x000e6f9f}, {2, 0x000dfef1}, {1, 0x000ea173}, {1, 0x00112fe0},
        {3, 0x000f6c8c}, {2, 0x0012b7dd}, {2, 0x001269d9}, {1, 0x0014aade},
        {2, 0x00176537}, {1, 0x00151bc7}, {2, 0x0016618f}, {2, 0x00177dda},
        {2, 0x0017be5e}, {2, 0x001a8c54}, {2, 0x001753e3}, {2, 0x001b0c6e},
        {1, 0x001edc94}, {2, 0x001ab194}, {2, 0x001acd6a}, {1, 0x001c625d},
        {2, 0x001fba70}, {1, 0x00207542}, {2, 0x0020d6ab}, {1, 0x0023fe8a},
        {1, 0x002244d0}, {3, 0x00261db8}, {2, 0x0025c54d}, {5, 0x0026594d},
        {1, 0x00285270}, {5, 0x002879ed}, {1, 0x002a7756}, {1, 0x002b1d6f},
        {1, 0x002b0e75}, {2, 0x002cff96}, {1, 0x002e099a}, {1, 0x003101fc},
        {1, 0x003156e4}, {1, 0x00319811}, {5, 0x003287c4}, {5, 0x003311af},
        {1, 0x0035bb87}, {1, 0x0036266f}, {6, 0x003bf3a8}, {2, 0x003a885c},
        {2, 0x003c2703}, {1, 0x003ba392}, {1, 0x003ce1d8}, {1, 0x003c517c},
        {2, 0x003d9aa8}, {2, 0x003d8192}, {1, 0x003f59a7}, {1, 0x0042737c},
        {1, 0x0043589d}, {5, 0x004b0373}, {5, 0x004ae26b}, {1, 0x004bb30b},
        {1, 0x004d78b1}, {2, 0x004ea7e1}, {2, 0x004e4f9d}, {1, 0x004f0c9c},
        {2, 0x004f4385}, {1, 0x00505cf6}, {2, 0x0050006f}, {2, 0x00541254},
        {1, 0x00541899}, {1, 0x00560982}, {1, 0x00558d03}, {5, 0x00576d3f},
        {1, 0x0058fe4d}, {1, 0x0059bf0e}, {1, 0x005bdacc}, {1, 0x005ca9e2},
        {1, 0x005dcbc4}, {1, 0x005e4852}, {1, 0x005f1c5e}, {2, 0x00614f91},
        {0, 0x00612582}, {1, 0x00620d7c}, {2, 0x00628970}, {2, 0x0064b263},
        {2, 0x0064e78d}, {1, 0x00662fd6}, {1, 0x0068f9db}, {1, 0x006a9f38},
        {1, 0x006a429a}, {1, 0x006b26af}, {1, 0x006c80b6}, {5, 0x006d4be1},
        {2, 0x006d34e0}, {1, 0x006e14ae}, {1, 0x006e360f}, {1, 0x006f1e1c},
        {2, 0x0070f604}, {2, 0x0072ea42}, {4, 0x00753da8}, {2, 0x0075c9ec},
        {1, 0x00788d33}, {1, 0x0078be4c}, {2, 0x0079fdd1}, {2, 0x007d59e0},
        {1, 0x007a9ce1}, {2, 0x007b0673}, {2, 0x007daafe}, {1, 0x007c012b},
        {1, 0x007c151e}, {2, 0x0080d215}, {2, 0x007eea50}, {1, 0x007f7d3e},
        {2, 0x007fce09}, {2, 0x00825db9}, {1, 0x008285f4}, {2, 0x00832b04},
        {1, 0x00861cb0}, {1, 0x0083d3f9}, {3, 0x008436f3}, {2, 0x008b65bd},
        {2, 0x008976d8}, {1, 0x008a35df}, {2, 0x0089e4a9}, {1, 0x008ec20b},
        {2, 0x008ff1c0}, {2, 0x008f87ee}, {2, 0x008ebb52}, {2, 0x00923eee},
        {2, 0x0096bff3}, {2, 0x00954088}, {1, 0x0094be9a}, {2, 0x0094e3e8},
        {2, 0x00987e29}, {1, 0x0096ad0e}, {2, 0x0095ed06}, {1, 0x0096690b},
        {2, 0x0097628e}, {1, 0x00980806}, {1, 0x009a1240}, {3, 0x009b57a3},
        {2, 0x009b8e71}, {5, 0x009af390}, {1, 0x009b7da1}, {5, 0x009dae8f},
        {1, 0x009d00ae}, {1, 0x009ecd06}, {1, 0x009dc991}, {2, 0x009ff159},
        {1, 0x009f8427}, {1, 0x009f7099}, {1, 0x00a2258b}, {1, 0x00a03299},
        {5, 0x00a71aa4}, {5, 0x00a20b2a}, {1, 0x00a73502}, {1, 0x00a277f0},
        {6, 0x00a59f39}, {2, 0x00a76ab7}, {2, 0x00a751de}, {1, 0x00aab2d3},
        {1, 0x00aab618}, {1, 0x00ada6e9}, {2, 0x00ae708d}, {2, 0x00b2ee36},
        {1, 0x00b1cc1c}, {1, 0x00b3ba3e}, {1, 0x00b729b2}, {5, 0x00b5b9b9},
        {5, 0x00b703c6}, {1, 0x00b73cc4}, {1, 0x00b8e5a5}, {2, 0x00b973d8},
        {2, 0x00b9f6fa}, {1, 0x00c3e2fb}, {2, 0x00c2c925}, {1, 0x00c2f68b},
        {2, 0x00c35cbe}, {2, 0x00c5ce0a}, {1, 0x00c64c59}, {1, 0x00c9cab7},
        {1, 0x00c80c2e}, {5, 0x00c7990b}, {1, 0x00c808ce}, {1, 0x00cba79f},
        {1, 0x00cb540d}, {1, 0x00cc5c46}, {1, 0x00cc792f}, {1, 0x00ccd701},
        {1, 0x00cf2beb}, {2, 0x00cde27a}, {0, 0x00cf19d7}, {1, 0x00d03bf7},
        {2, 0x00d24e39}, {2, 0x00d19387}, {2, 0x00d01889}, {1, 0x00d1226e},
        {1, 0x00d1c5f0}, {1, 0x00d14275}, {1, 0x00d2f784}, {1, 0x00d21fa7},
        {1, 0x00d32ccc}, {5, 0x00d29b31}, {2, 0x00d42dbd}, {1, 0x00d57387},
        {1, 0x00d63b4f}, {1, 0x00dbe5be}, {2, 0x00d8cbcc}, {2, 0x00d8c641},
        {4, 0x00d9b1ee}, {2, 0x00d904a6}, {1, 0x00db4670}, {1, 0x00e03bc1},
        {2, 0x00e0de32}, {2, 0x00e00011}, {1, 0x00e1eb16}, {2, 0x00e16783},
        {2, 0x00e912ff}, {1, 0x00e50697}, {1, 0x00e5a4bb}, {2, 0x00e836ce},
        {2, 0x00e6ca38}, {1, 0x00e74ad7}, {2, 0x00e8bc31}, {2, 0x00ecc7cd},
        {1, 0x00ee8ccd}, {2, 0x00ee99b5}, {1, 0x00f0f71b}, {1, 0x00f4f26e},
        {3, 0x00f5c2cf}, {2, 0x00f74ea0}, {2, 0x00fa247b}, {1, 0x00fb9dfa},
        {2, 0x00fb0413}, {1, 0x00fc3be6}, {2, 0x00fcede3}, {2, 0x00fd458a},
        {2, 0x00fe6f7a}, {2, 0x0101100f}, {2, 0x01012e7c}, {2, 0x0101e67c},
        {1, 0x0101c819}, {2, 0x0103d741}, {2, 0x0105a3e5}, {1, 0x0108c157},
        {2, 0x01082d30}, {1, 0x0109e25c}, {2, 0x010cbb0c}, {1, 0x010bd106},
        {1, 0x010dd721}, {3, 0x010f68c6}, {2, 0x010ec3c3}, {5, 0x010f6630},
        {1, 0x0111722a}, {5, 0x011789df}, {1, 0x011292c1}, {1, 0x0113163a},
        {1, 0x01172a9f}, {2, 0x01176710}, {1, 0x011649c0}, {1, 0x0119914b},
        {1, 0x011caa5e}, {1, 0x011c19f4}, {5, 0x011d37c8}, {5, 0x011d2b95},
        {1, 0x011df415}, {1, 0x01205e67}, {6, 0x01203b89}, {2, 0x0121a131},
        {2, 0x0123b1c2}, {1, 0x01255090}, {1, 0x01269732}, {1, 0x012b52cf},
        {2, 0x01271828}, {2, 0x0128263e}, {1, 0x0128bca9}, {1, 0x012a9c52},
        {1, 0x012afa66}, {5, 0x012c6b62}, {5, 0x01314fda}, {1, 0x01302894},
        {1, 0x0130603c}, {2, 0x01339a1f}, {2, 0x0133ad3f}, {1, 0x0134684e},
        {2, 0x0135470c}, {1, 0x0136e97f}, {2, 0x0136e869}, {2, 0x0138b5d5},
        {1, 0x01388042}, {1, 0x013bc93e}, {1, 0x013aabd2}, {5, 0x013b0525},
        {1, 0x013b8ce0}, {1, 0x013e419c}, {1, 0x013e4606}, {1, 0x013e3b36},
        {1, 0x013dbe47}, {1, 0x013dece2}, {1, 0x013f185a}, {2, 0x013f45f6},
        {0, 0x01436c21}, {1, 0x01445ac1}, {2, 0x0144b4de}, {2, 0x0145301a},
        {2, 0x0146c31e}, {1, 0x0147b410}, {1, 0x0147cba8}, {1, 0x014aa8ba},
        {1, 0x014bc576}, {1, 0x014f07cd}, {1, 0x014f64c6}, {5, 0x014f94cc},
        {2, 0x0150e547}, {1, 0x0150862c}, {1, 0x015100ea}, {1, 0x0152a7b7},
        {2, 0x01516239}, {2, 0x0153461b}, {4, 0x0154543e}, {2, 0x0153d344},
        {1, 0x01540154}, {1, 0x01540048}, {2, 0x01578e20}, {2, 0x01575105},
        {1, 0x0157ea23}, {2, 0x0158a0e4}, {2, 0x015bc390}, {1, 0x015de8cf},
        {1, 0x015cf844}, {2, 0x015cf9c5}, {2, 0x015d60e4}, {1, 0x015e3acc},
        {2, 0x015ed533}, {2, 0x0162708f}, {1, 0x0162d69f}, {2, 0x01629d07},
        {1, 0x0160480e}, {1, 0x0161656c}, {3, 0x0161fffa}, {2, 0x0162e2ea},
        {2, 0x0163dc60}, {1, 0x0167887b}, {2, 0x01674104}, {1, 0x01679376},
        {2, 0x0168fce2}, {2, 0x0168e998}, {2, 0x0169c6c3}, {2, 0x01696b27},
        {2, 0x016d520b}, {2, 0x016cb656}, {1, 0x016e2a0a}, {2, 0x016eaf5a},
        {2, 0x016fe59c}, {1, 0x016faed2}, {2, 0x0170151e}, {1, 0x01713b17},
        {2, 0x0172e388}, {1, 0x017715a5}, {1, 0x017440db}, {3, 0x017653be},
        {2, 0x0177acb4}, {5, 0x01777198}, {1, 0x01771347}, {5, 0x0178b48c},
        {1, 0x017a7281}, {1, 0x017ad627}, {1, 0x017a1ec4}, {2, 0x0179d96e},
        {1, 0x017a3477}, {1, 0x017a623e}, {1, 0x017cc9dc}, {1, 0x01812604},
        {5, 0x017e4515}, {5, 0x017e87bc}, {1, 0x017ed024}, {1, 0x017eae68},
        {6, 0x017ed630}, {2, 0x017fcfd5}, {2, 0x01814452}, {1, 0x018086e0},
        {1, 0x0181a9c1}, {1, 0x0184be6c}, {2, 0x01845998}, {2, 0x01848018},
        {1, 0x0185565a}, {1, 0x01859fa8}, {1, 0x01884b1f}, {5, 0x0187385f},
        {5, 0x0188c951}, {1, 0x0189d3ad}, {1, 0x018bd0c7}, {2, 0x018cc5a8},
        {2, 0x018c946f}, {1, 0x018d91d7}, {2, 0x018dc3c2}, {1, 0x0190508a},
        {2, 0x01903b87}, {2, 0x018fa16f}, {1, 0x019072e9}, {1, 0x0192bc94},
        {1, 0x0193d2b1}, {5, 0x019824dd}, {1, 0x0198c673}, {1, 0x0197f8ff},
        {1, 0x01983ce4}, {1, 0x0198444c}, {1, 0x01984b81}, {1, 0x019927da},
        {1, 0x0199dc7d}, {2, 0x019ad0e4}, {0, 0x019dc38f}, {1, 0x019cbe15},
        {2, 0x019d61be}, {2, 0x019fb687}, {2, 0x019f7433}, {1, 0x01a04185},
        {1, 0x01a3826d}, {1, 0x01a25c0e}, {1, 0x01a302bb}, {1, 0x01a382bb},
        {1, 0x01a3b809}, {5, 0x01a485cf}, {2, 0x01a43f91}, {1, 0x01a44d89},
        {1, 0x01a44e20}, {1, 0x01aaa6bd}, {2, 0x01a7c389}, {2, 0x01a7c7ba},
        {4, 0x01a8d87c}, {2, 0x01aa6ca2}, {1, 0x01aa2049}, {1, 0x01ace9f1},
        {2, 0x01aa61b8}, {2, 0x01aef8f3}, {1, 0x01af647a}, {2, 0x01aebda1},
        {2, 0x01af450e}, {1, 0x01b221c4}, {1, 0x01b178ed}, {2, 0x01b30a04},
        {2, 0x01b484d4}, {1, 0x01b5b356}, {2, 0x01b51d52}, {2, 0x01b52611},
        {1, 0x01b57c7e}, {2, 0x01b5cf07}, {1, 0x01b89350}, {1, 0x01b81893},
        {3, 0x01b76027}, {2, 0x01b7e6ac}, {2, 0x01b90429}, {1, 0x01b9a97f},
        {2, 0x01bb28c5}, {1, 0x01bea79a}, {2, 0x01bdad54}, {2, 0x01bf100f},
        {2, 0x01be9fd0}, {2, 0x01bf3eea}, {2, 0x01bf11eb}, {2, 0x01c35f92},
        {1, 0x01c3a05b}, {2, 0x01c70126}, {2, 0x01c6555e}, {1, 0x01c7389a},
        {2, 0x01c7d77d}, {1, 0x01c76249}, {2, 0x01c7f789}, {1, 0x01c9447d},
        {1, 0x01cf7569}, {3, 0x01cf4ebb}, {2, 0x01cf3205}, {5, 0x01d4165e},
        {1, 0x01d4f1c6}, {5, 0x01d501a5}, {1, 0x01d59360}, {1, 0x01d828f9},
        {1, 0x01dae341}, {2, 0x01db8837}, {1, 0x01de882b}, {1, 0x01e1bdd6},
        {1, 0x01e31653}, {1, 0x01e1f845}, {5, 0x01e41899}, {5, 0x01e633d0},
        {1, 0x01e84414}, {1, 0x01ea047e}, {6, 0x01e97cc4}, {2, 0x01e9e164},
        {2, 0x01e8f021}, {1, 0x01e92bdc}, {1, 0x01ee2f01}, {1, 0x01eb6791},
        {2, 0x01ed774f}, {2, 0x01edb795}, {1, 0x01ee22eb}, {1, 0x01edf2b7},
        {1, 0x01ee02e0}, {5, 0x01f08b67}, {5, 0x01f7e761}, {1, 0x01f37578},
        {1, 0x01f5e2bc}, {2, 0x01f5eac7}, {2, 0x01f67d76}, {1, 0x01f58a3a},
        {2, 0x01f740d3}, {1, 0x01f8345d}, {2, 0x01fd60b4}, {2, 0x01fcbd5a},
        {1, 0x0201471f}, {1, 0x0205dda1}, {1, 0x020446a4}, {5, 0x02060c38},
        {1, 0x020773be}, {1, 0x0206e35e}, {1, 0x02078298}, {1, 0x02085251},
        {1, 0x0208ec39}, {1, 0x0208c7b6}, {1, 0x020af655}, {2, 0x020e93b9},
        {0, 0x0210925c}, {1, 0x021377e8}, {2, 0x0212d3a4}, {2, 0x0217396b},
        {2, 0x021a0da9}, {1, 0x02197848}, {1, 0x021c37cb}, {1, 0x021ab0ab},
        {1, 0x021bf967}, {1, 0x021db7fb}, {1, 0x021ea6e4}, {5, 0x02211438},
        {2, 0x02216c35}, {1, 0x0222d605}, {1, 0x0223102c}, {1, 0x0225ef5c},
        {2, 0x022648cd}, {2, 0x02277fbf},
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
