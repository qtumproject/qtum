// Copyright (c) 2014-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <net.h>
#include <validation.h>

#include <test/util/setup_common.h>

#include <boost/signals2/signal.hpp>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(validation_tests, TestingSetup)

static void TestBlockSubsidyHalvings(const Consensus::Params& consensusParams)
{
    int maxHalvings = 7;
    CAmount nInitialSubsidy = 4 * COIN;

    CAmount nPreviousSubsidy = nInitialSubsidy * 2 ; // for height == LastPoWBlock + 1
    BOOST_CHECK_EQUAL(nPreviousSubsidy, nInitialSubsidy * 2);
    for (int nHalvings = 0; nHalvings < maxHalvings; nHalvings++) {
        int nHeight = nHalvings * consensusParams.nSubsidyHalvingInterval + consensusParams.nLastBigReward + 1;
        CAmount nSubsidy = GetBlockSubsidy(nHeight, consensusParams);
        BOOST_CHECK(nSubsidy <= nInitialSubsidy);
        BOOST_CHECK_EQUAL(nSubsidy, nPreviousSubsidy / 2);
        nPreviousSubsidy = nSubsidy;
    }
    BOOST_CHECK_EQUAL(GetBlockSubsidy(maxHalvings * consensusParams.nSubsidyHalvingInterval + consensusParams.nLastBigReward + 1, consensusParams), 0);
}

static void TestBlockSubsidyHalvings(int nSubsidyHalvingInterval)
{
    Consensus::Params consensusParams;
    consensusParams.nSubsidyHalvingInterval = nSubsidyHalvingInterval;
    TestBlockSubsidyHalvings(consensusParams);
}

BOOST_AUTO_TEST_CASE(block_subsidy_test)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    TestBlockSubsidyHalvings(chainParams->GetConsensus()); // As in main
    TestBlockSubsidyHalvings(150); // As in regtest
    TestBlockSubsidyHalvings(1000); // Just another interval
}

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params& consensusParams = chainParams->GetConsensus();
    CAmount nSum = 0;
    for (int nHeight = 1; nHeight < 14000000; nHeight++) {
        CAmount nSubsidy = GetBlockSubsidy(nHeight, consensusParams);

        if(nHeight <= consensusParams.nLastPOWBlock){
            BOOST_CHECK_EQUAL(nSubsidy, (20000 * COIN));
        }
        else if(nHeight-consensusParams.nLastPOWBlock <= consensusParams.nSubsidyHalvingInterval){
            BOOST_CHECK_EQUAL(nSubsidy, 4 * COIN);
        }
        else if(nHeight-consensusParams.nLastPOWBlock <= consensusParams.nSubsidyHalvingInterval*2){
            BOOST_CHECK_EQUAL(nSubsidy, 2 * COIN);
        }
        else if(nHeight-consensusParams.nLastPOWBlock <= consensusParams.nSubsidyHalvingInterval*3){
            BOOST_CHECK_EQUAL(nSubsidy, 1 * COIN);
        }
        else if(nHeight-consensusParams.nLastPOWBlock <= consensusParams.nSubsidyHalvingInterval*4){
            BOOST_CHECK_EQUAL(nSubsidy, 0.5 * COIN);
        }
        else if(nHeight-consensusParams.nLastPOWBlock <= consensusParams.nSubsidyHalvingInterval*5){
            BOOST_CHECK_EQUAL(nSubsidy, 0.25 * COIN);
        }
        else if(nHeight-consensusParams.nLastPOWBlock <= consensusParams.nSubsidyHalvingInterval*6){
            BOOST_CHECK_EQUAL(nSubsidy, 0.125 * COIN);
        }
        else if(nHeight-consensusParams.nLastPOWBlock <= consensusParams.nSubsidyHalvingInterval*7){
            BOOST_CHECK_EQUAL(nSubsidy, 0.0625 * COIN);
        }
        else{
            BOOST_CHECK_EQUAL(nSubsidy, 0);
        }
        nSum += nSubsidy;
        BOOST_CHECK(MoneyRange(nSum));
    }
    BOOST_CHECK_EQUAL(nSum, 10782240625000000ULL);
}

static bool ReturnFalse() { return false; }
static bool ReturnTrue() { return true; }

BOOST_AUTO_TEST_CASE(test_combiner_all)
{
    boost::signals2::signal<bool (), CombinerAll> Test;
    BOOST_CHECK(Test());
    Test.connect(&ReturnFalse);
    BOOST_CHECK(!Test());
    Test.connect(&ReturnTrue);
    BOOST_CHECK(!Test());
    Test.disconnect(&ReturnFalse);
    BOOST_CHECK(Test());
    Test.disconnect(&ReturnTrue);
    BOOST_CHECK(Test());
}
BOOST_AUTO_TEST_SUITE_END()
