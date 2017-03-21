// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "key.h"

#include "base58.h"
#include "script/script.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"
#include "test/test_bitcoin.h"

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

static const std::string strSecret1     ("L4uiizxakj6yRjaZXDWA5y5qJPPKMmxgnVm5LySMEQkggjtwXirZ");
static const std::string strSecret2     ("L2gKFWaWyvN9p5JqNPK28WPLBkurTcDoX7eJMmaPbXP1mz7swGKj");
static const std::string strSecret1C    ("L2Avtt6YF5YiASxp6EYpb5GEWdMDHYfpkzc1ZErodpRwQsNsZ79L");
static const std::string strSecret2C    ("Kx9i5xWyzmCmmiUBQGF6kJsV7xCKxDaB8BK7spKAAGZsjrtbCMda");
static const CBitcoinAddress addr1 ("QaDifbnvzdabrjvqccSWgmh4rrY8tWUdaA");
static const CBitcoinAddress addr2 ("QQCf3y1KsDYxTozX1RDK1rh8fw7t1Xy9iA");
static const CBitcoinAddress addr1C("QeAzprSjgSYTvYqwZ6HRKdTYyZKS1RZdi5");
static const CBitcoinAddress addr2C("QTiDYVrCLhNnYsvCbCMuakadQH8Kgx4xEw");


static const std::string strAddressBad("QHV9Lc3sNHZxwj4Zk6fB38tEmBryq2cBiF");


#ifdef KEY_TESTS_DUMPINFO
void dumpKeyInfo(uint256 privkey)
{
    CKey key;
    key.resize(32);
    memcpy(&secret[0], &privkey, 32);
    std::vector<unsigned char> sec;
    sec.resize(32);
    memcpy(&sec[0], &secret[0], 32);
    printf("  * secret (hex): %s\n", HexStr(sec).c_str());

    for (int nCompressed=0; nCompressed<2; nCompressed++)
    {
        bool fCompressed = nCompressed == 1;
        printf("  * %s:\n", fCompressed ? "compressed" : "uncompressed");
        CBitcoinSecret bsecret;
        bsecret.SetSecret(secret, fCompressed);
        printf("    * secret (base58): %s\n", bsecret.ToString().c_str());
        CKey key;
        key.SetSecret(secret, fCompressed);
        std::vector<unsigned char> vchPubKey = key.GetPubKey();
        printf("    * pubkey (hex): %s\n", HexStr(vchPubKey).c_str());
        printf("    * address (base58): %s\n", CBitcoinAddress(vchPubKey).ToString().c_str());
    }
}
#endif


BOOST_FIXTURE_TEST_SUITE(key_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(key_test1)
{

}

BOOST_AUTO_TEST_SUITE_END()
