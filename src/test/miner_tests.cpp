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
        {4, 0xf81a}, {2, 0x1440e}, {1, 0x1ada2}, {1, 0x2020d},
        {2, 0x3827d}, {2, 0x39670}, {1, 0x437ff}, {2, 0x57fe1},
        {2, 0x763e9}, {1, 0x827b0}, {1, 0xa43f8}, {2, 0xa5ea6},
        {2, 0xaaeb4}, {1, 0xaf2f8}, {2, 0xb8bd7}, {2, 0xce74b},
        {1, 0xd5a15}, {2, 0xd79e1}, {1, 0xe03b0}, {1, 0xe4ee9},
        {3, 0xee2a7}, {2, 0x1100f6}, {2, 0x111d64}, {1, 0x13b160},
        {2, 0x13e159}, {1, 0x14c6eb}, {2, 0x15c669}, {2, 0x15d4d2},
        {2, 0x17197d}, {2, 0x172fa2}, {2, 0x174120}, {2, 0x1990c6},
        {1, 0x19de31}, {2, 0x1a7fc8}, {2, 0x1aa3c1}, {1, 0x1c60dd},
        {2, 0x1f707a}, {1, 0x1f7f67}, {2, 0x20a48d}, {1, 0x221b94},
        {1, 0x2238d2}, {3, 0x2450a6}, {2, 0x25b167}, {5, 0x261fba},
        {1, 0x27043a}, {5, 0x2849f5}, {1, 0x286279}, {1, 0x28f5bd},
        {1, 0x2a58c1}, {2, 0x2a8cd2}, {1, 0x2e04f1}, {1, 0x2ef7d5},
        {1, 0x2f765b}, {1, 0x314ecc}, {5, 0x31ac93}, {5, 0x32d0f6},
        {1, 0x35ab43}, {1, 0x35dbe0}, {6, 0x390ef4}, {2, 0x3a8212},
        {2, 0x3a8276}, {1, 0x3b3015}, {1, 0x3bfa74}, {1, 0x3c2cf7},
        {2, 0x3c6709}, {2, 0x3ca8dd}, {1, 0x3e9e3b}, {1, 0x40ae81},
        {1, 0x419d56}, {5, 0x49b7ff}, {5, 0x4a4410}, {1, 0x4b519f},
        {1, 0x4d32a0}, {2, 0x4e4b71}, {2, 0x4e4c57}, {1, 0x4e95b3},
        {2, 0x4ef9bc}, {1, 0x4f13b2}, {2, 0x4fd2d6}, {2, 0x509608},
        {1, 0x53c8a2}, {1, 0x546569}, {1, 0x5579b6}, {5, 0x56f30b},
        {1, 0x57f86e}, {1, 0x59a139}, {1, 0x5b38f4}, {1, 0x5c0c9f},
        {1, 0x5d17bf}, {1, 0x5e0a0a}, {1, 0x5e5865}, {2, 0x60507d},
        {0, 0x60dc39}, {1, 0x61868e}, {2, 0x6187af}, {2, 0x643913},
        {2, 0x64d5ad}, {1, 0x656141}, {1, 0x68cbf2}, {1, 0x69b0d5},
        {1, 0x6a3e35}, {1, 0x6ab69c}, {1, 0x6c6ba6}, {5, 0x6cb13d},
        {2, 0x6d23e8}, {1, 0x6dc88a}, {1, 0x6de199}, {1, 0x6eba7d},
        {2, 0x70ab9b}, {2, 0x72bb74}, {4, 0x74c7c1}, {2, 0x74dd36},
        {1, 0x7837d6}, {1, 0x7863ad}, {2, 0x78a1d1}, {2, 0x7a74b9},
        {1, 0x7a9485}, {2, 0x7a984f}, {2, 0x7aeb03}, {1, 0x7b4709},
        {1, 0x7be67b}, {2, 0x7d9dc7}, {2, 0x7e9cad}, {1, 0x7f07ba},
        {2, 0x7f4932}, {2, 0x81b9f8}, {1, 0x825e3a}, {2, 0x826e25},
        {1, 0x82d132}, {1, 0x8396c9}, {3, 0x8428c9}, {2, 0x88e8fa},
        {2, 0x89598e}, {1, 0x897af2}, {2, 0x89cead}, {1, 0x8cbe88},
        {2, 0x8d7830}, {2, 0x8d95a6}, {2, 0x8e12d8}, {2, 0x91aaf3},
        {2, 0x93916a}, {2, 0x94115a}, {1, 0x949a24}, {2, 0x949d3f},
        {2, 0x9536fe}, {1, 0x954a24}, {2, 0x958289}, {1, 0x95b8a9},
        {2, 0x966340}, {1, 0x97dc30}, {1, 0x9967ce}, {3, 0x99fb4f},
        {2, 0x9a43c3}, {5, 0x9a7da7}, {1, 0x9abe56}, {5, 0x9b08a0},
        {1, 0x9c6385}, {1, 0x9d3c55}, {1, 0x9db3e2}, {2, 0x9f0cb0},
        {1, 0x9f4077}, {1, 0x9f5fee}, {1, 0x9fe7c3}, {1, 0xa01619},
        {5, 0xa07860}, {5, 0xa0ee6f}, {1, 0xa13cc5}, {1, 0xa259d1},
        {6, 0xa414f6}, {2, 0xa72948}, {2, 0xa72bd1}, {1, 0xa9284c},
        {1, 0xaa36b2}, {1, 0xac195a}, {2, 0xada5e8}, {2, 0xb108c3},
        {1, 0xb19a57}, {1, 0xb24096}, {1, 0xb51351}, {5, 0xb55a5c},
        {5, 0xb56650}, {1, 0xb6d959}, {1, 0xb6fe9f}, {2, 0xb94599},
        {2, 0xb993f8}, {1, 0xc21bc8}, {2, 0xc239ad}, {1, 0xc27ab1},
        {2, 0xc29fb7}, {2, 0xc4a184}, {1, 0xc5bbcf}, {1, 0xc6390a},
        {1, 0xc6f9a9}, {5, 0xc73bfc}, {1, 0xc76dce}, {1, 0xc9f2ac},
        {1, 0xcac33e}, {1, 0xcbdca7}, {1, 0xcc6058}, {1, 0xcc6d3f},
        {1, 0xcd67c1}, {2, 0xcde181}, {0, 0xce2828}, {1, 0xcf227e},
        {2, 0xcf77cc}, {2, 0xcf829a}, {2, 0xcfa3c0}, {1, 0xd05991},
        {1, 0xd0b15a}, {1, 0xd125bc}, {1, 0xd169da}, {1, 0xd1d852},
        {1, 0xd22bbb}, {5, 0xd27280}, {2, 0xd2f9c0}, {1, 0xd51d65},
        {1, 0xd600c9}, {1, 0xd7545e}, {2, 0xd88056}, {2, 0xd8c0c0},
        {4, 0xd8d3bb}, {2, 0xd8fef8}, {1, 0xdaa561}, {1, 0xdf7ed9},
        {2, 0xdfc7d4}, {2, 0xdfe137}, {1, 0xe039c2}, {2, 0xe0ef9b},
        {2, 0xe3a7e4}, {1, 0xe40c42}, {1, 0xe41088}, {2, 0xe52bdd},
        {2, 0xe5d417}, {1, 0xe5ea03}, {2, 0xe7c0af}, {2, 0xeca79b},
        {1, 0xed8453}, {2, 0xee771a}, {1, 0xf09a2b}, {1, 0xf3f002},
        {3, 0xf5a19b}, {2, 0xf64552}, {2, 0xf9020f}, {1, 0xfa3e9f},
        {2, 0xfa9fd8}, {1, 0xfb9e3b}, {2, 0xfc9a59}, {2, 0xfd2b97},
        {2, 0xfd75b1}, {2, 0xfe822b}, {2, 0xfff7fa}, {2, 0x100045b},
        {1, 0x1017fa2}, {2, 0x10352f5}, {2, 0x1045024}, {1, 0x107a09a},
        {2, 0x107dd5e}, {1, 0x1092474}, {2, 0x10a520d}, {1, 0x10a84db},
        {1, 0x10d3332}, {3, 0x10e59a8}, {2, 0x10eabe2}, {5, 0x10edf9a},
        {1, 0x1111260}, {5, 0x1119fe2}, {1, 0x112237a}, {1, 0x1128947},
        {1, 0x114f5f5}, {2, 0x1161aed}, {1, 0x1163508}, {1, 0x11954c1},
        {1, 0x11b6ee5}, {1, 0x11c199e}, {5, 0x11d0fa3}, {5, 0x11d134e},
        {1, 0x11d99a3}, {1, 0x11f0797}, {6, 0x11f1b64}, {2, 0x1207356},
        {2, 0x1238716}, {1, 0x1241ba6}, {1, 0x124b18a}, {1, 0x125644e},
        {2, 0x125f53a}, {2, 0x1276792}, {1, 0x1280b0d}, {1, 0x129d109},
        {1, 0x12aefc5}, {5, 0x12c2e69}, {5, 0x12f0a1a}, {1, 0x12fac65},
        {1, 0x1304cca}, {2, 0x132110f}, {2, 0x1335f7f}, {1, 0x1337511},
        {2, 0x13522a2}, {1, 0x136b694}, {2, 0x136c2c2}, {2, 0x13808aa},
        {1, 0x1381332}, {1, 0x1399bc9}, {1, 0x13a0f93}, {5, 0x13b03e2},
        {1, 0x13b3b8d}, {1, 0x13c9106}, {1, 0x13cf389}, {1, 0x13d1b1e},
        {1, 0x13d3ed4}, {1, 0x13d8785}, {1, 0x13e13ae}, {2, 0x13e5d16},
        {0, 0x1420440}, {1, 0x142a547}, {2, 0x1435038}, {2, 0x1438e6b},
        {2, 0x144b4b5}, {1, 0x1458252}, {1, 0x146d939}, {1, 0x14a6b1a},
        {1, 0x14b71f4}, {1, 0x14c1abe}, {1, 0x14d73dc}, {5, 0x14f1a09},
        {2, 0x14f2134}, {1, 0x14f2b4e}, {1, 0x14fd0b2}, {1, 0x1505a12},
        {2, 0x1510a69}, {2, 0x151309b}, {4, 0x152b380}, {2, 0x1530d69},
        {1, 0x1537571}, {1, 0x153781a}, {2, 0x156477c}, {2, 0x156e5af},
        {1, 0x1575a3e}, {2, 0x15864d1}, {2, 0x159a91d}, {1, 0x15ca9a0},
        {1, 0x15cc54c}, {2, 0x15cc587}, {2, 0x15d4af9}, {1, 0x15dd501},
        {2, 0x15e2969}, {2, 0x15f523f}, {1, 0x15f9764}, {2, 0x1601668},
        {1, 0x16032b3}, {1, 0x160e2a9}, {3, 0x161a92c}, {2, 0x16276a7},
        {2, 0x16389fd}, {1, 0x1666a27}, {2, 0x166aadf}, {1, 0x1671c17},
        {2, 0x16721dc}, {2, 0x168dabc}, {2, 0x16912c3}, {2, 0x1695695},
        {2, 0x16bf594}, {2, 0x16c8f3e}, {1, 0x16c9dd2}, {2, 0x16e2eeb},
        {2, 0x16e7f32}, {1, 0x16f526e}, {2, 0x16fb17d}, {1, 0x170f58d},
        {2, 0x172b411}, {1, 0x1738975}, {1, 0x173c58a}, {3, 0x174379c},
        {2, 0x175e034}, {5, 0x1762f50}, {1, 0x176c8be}, {5, 0x178a9f8},
        {1, 0x1791b21}, {1, 0x1796b6a}, {1, 0x1798af7}, {2, 0x179c8c3},
        {1, 0x17a0aa2}, {1, 0x17a1dd5}, {1, 0x17c105f}, {1, 0x17ce8b1},
        {5, 0x17d1fd9}, {5, 0x17d8ed1}, {1, 0x17e84dc}, {1, 0x17e93d9},
        {6, 0x17e99ad}, {2, 0x17f557c}, {2, 0x1806beb}, {1, 0x1806d14},
        {1, 0x1814699}, {1, 0x182a5ea}, {2, 0x183e00f}, {2, 0x1841c85},
        {1, 0x1848e9d}, {1, 0x1858794}, {1, 0x1859121}, {5, 0x18630ea},
        {5, 0x1888345}, {1, 0x18930f5}, {1, 0x18b0c4d}, {2, 0x18bb47e},
        {2, 0x18c064e}, {1, 0x18d3d9d}, {2, 0x18d49f0}, {1, 0x18e2056},
        {2, 0x18f7a3a}, {2, 0x18f8845}, {1, 0x18ffe3c}, {1, 0x191f9e8},
        {1, 0x192f937}, {5, 0x193ad7a}, {1, 0x195a2ce}, {1, 0x1974768},
        {1, 0x1982407}, {1, 0x198243f}, {1, 0x1983e64}, {1, 0x198f250},
        {1, 0x199963a}, {2, 0x199d5ae}, {0, 0x19c1f4b}, {1, 0x19c7889},
        {2, 0x19d6199}, {2, 0x19e7cfc}, {2, 0x19f3e7f}, {1, 0x1a03d61},
        {1, 0x1a161ed}, {1, 0x1a19f34}, {1, 0x1a24142}, {1, 0x1a28328},
        {1, 0x1a3a5e0}, {5, 0x1a3cbde}, {2, 0x1a3d35c}, {1, 0x1a3f7d8},
        {1, 0x1a42c55}, {1, 0x1a71f26}, {2, 0x1a7374a}, {2, 0x1a73cc7},
        {4, 0x1a813d7}, {2, 0x1a9fc73}, {1, 0x1aa1f78}, {1, 0x1aa3bd3},
        {2, 0x1aa4963}, {2, 0x1ad97d6}, {1, 0x1adac10}, {2, 0x1ade42d},
        {2, 0x1ae918f}, {1, 0x1aef3ed}, {1, 0x1b14485}, {2, 0x1b17c28},
        {2, 0x1b3d608}, {1, 0x1b413ae}, {2, 0x1b49410}, {2, 0x1b4b205},
        {1, 0x1b50597}, {2, 0x1b5ba99}, {1, 0x1b6853b}, {1, 0x1b69db0},
        {3, 0x1b6d25c}, {2, 0x1b7da4d}, {2, 0x1b8383c}, {1, 0x1b96f3d},
        {2, 0x1baf0ce}, {1, 0x1bd6719}, {2, 0x1bd9180}, {2, 0x1bdfcbc},
        {2, 0x1be067a}, {2, 0x1be837d}, {2, 0x1be91f6}, {2, 0x1c31904},
        {1, 0x1c3a008}, {2, 0x1c564f5}, {2, 0x1c5da8d}, {1, 0x1c65620},
        {2, 0x1c65d44}, {1, 0x1c6ec50}, {2, 0x1c6f825}, {1, 0x1c8db4f},
        {1, 0x1cb60d7}, {3, 0x1ce8ce2}, {2, 0x1cee4ff}, {5, 0x1d06fed},
        {1, 0x1d2e398}, {5, 0x1d45baa}, {1, 0x1d53e88}, {1, 0x1d7c227},
        {1, 0x1d8a7b5}, {2, 0x1dae73d}, {1, 0x1dd925d}, {1, 0x1dfad0f},
        {1, 0x1dfed6e}, {1, 0x1e1b7c9}, {5, 0x1e30171}, {5, 0x1e5b1e8},
        {1, 0x1e82cdb}, {1, 0x1e879d2}, {6, 0x1e8e2c0}, {2, 0x1e8e819},
        {2, 0x1e8ec11}, {1, 0x1e91465}, {1, 0x1e976d4}, {1, 0x1eb1997},
        {2, 0x1ec62b8}, {2, 0x1ed5a98}, {1, 0x1ed60bb}, {1, 0x1ed9a82},
        {1, 0x1edcf41}, {5, 0x1ef058b}, {5, 0x1f27ea5}, {1, 0x1f3535c},
        {1, 0x1f3fd43}, {2, 0x1f4c492}, {2, 0x1f53da0}, {1, 0x1f58693},
        {2, 0x1f62074}, {1, 0x1f82be7}, {2, 0x1fc0958}, {2, 0x1fc9f2a},
        {1, 0x1fee60b}, {1, 0x2032e6d}, {1, 0x203e9a1}, {5, 0x205b168},
        {1, 0x205fc72}, {1, 0x206cb21}, {1, 0x20737cd}, {1, 0x207e19b},
        {1, 0x20857fb}, {1, 0x20882d0}, {1, 0x20a9e4b}, {2, 0x20c8584},
        {0, 0x20df5e5}, {1, 0x211549f}, {2, 0x212cf22}, {2, 0x21330ee},
        {2, 0x2181f30}, {1, 0x218bf6c}, {1, 0x2195fc9}, {1, 0x21a5e60},
        {1, 0x21b48bb}, {1, 0x21b8143}, {1, 0x21cb4ad}, {5, 0x2206053},
        {2, 0x2209614}, {1, 0x220addc}, {1, 0x2230757}, {1, 0x2256241},
        {2, 0x225b576}, {2, 0x226345a},
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
