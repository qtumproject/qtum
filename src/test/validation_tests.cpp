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
    consensusParams.nReduceBlocktimeHeight = 0x7fffffff;
    TestBlockSubsidyHalvings(consensusParams);
}

BOOST_AUTO_TEST_CASE(block_subsidy_test)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    Consensus::Params consensusParams = chainParams->GetConsensus();
    consensusParams.nReduceBlocktimeHeight = 0x7fffffff; // Check for the halving before fork for target spacing
    TestBlockSubsidyHalvings(consensusParams); // As in main
    TestBlockSubsidyHalvings(150); // As in regtest
    TestBlockSubsidyHalvings(1000); // Just another interval
}

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    Consensus::Params consensusParams = chainParams->GetConsensus();
    consensusParams.nReduceBlocktimeHeight = 800000; // Check for the halving after fork for target spacing
    int nMaxHeight = 14000000 * consensusParams.nBlocktimeDownscaleFactor;
    CAmount nSum = 0;
    for (int nHeight = 1; nHeight < nMaxHeight; nHeight++) {
        CAmount nSubsidy = GetBlockSubsidy(nHeight, consensusParams);
        int nSubsidyHalvingWeight = consensusParams.SubsidyHalvingWeight(nHeight);
        int nSubsidyHalvingInterval = consensusParams.SubsidyHalvingInterval(nHeight);
        int nBlocktimeDownscaleFactor = consensusParams.BlocktimeDownscaleFactor(nHeight);

        if(nSubsidyHalvingWeight <= 0){
            BOOST_CHECK_EQUAL(nSubsidy, (20000 * COIN));
        }
        else if(nSubsidyHalvingWeight <= nSubsidyHalvingInterval){
            BOOST_CHECK_EQUAL(nSubsidy, 4 * COIN / nBlocktimeDownscaleFactor);
        }
        else if(nSubsidyHalvingWeight <= nSubsidyHalvingInterval*2){
            BOOST_CHECK_EQUAL(nSubsidy, 2 * COIN / nBlocktimeDownscaleFactor);
        }
        else if(nSubsidyHalvingWeight <= nSubsidyHalvingInterval*3){
            BOOST_CHECK_EQUAL(nSubsidy, 1 * COIN / nBlocktimeDownscaleFactor);
        }
        else if(nSubsidyHalvingWeight <= nSubsidyHalvingInterval*4){
            BOOST_CHECK_EQUAL(nSubsidy, 0.5 * COIN / nBlocktimeDownscaleFactor);
        }
        else if(nSubsidyHalvingWeight <= nSubsidyHalvingInterval*5){
            BOOST_CHECK_EQUAL(nSubsidy, 0.25 * COIN / nBlocktimeDownscaleFactor);
        }
        else if(nSubsidyHalvingWeight <= nSubsidyHalvingInterval*6){
            BOOST_CHECK_EQUAL(nSubsidy, 0.125 * COIN / nBlocktimeDownscaleFactor);
        }
        else if(nSubsidyHalvingWeight <= nSubsidyHalvingInterval*7){
            BOOST_CHECK_EQUAL(nSubsidy, 0.0625 * COIN / nBlocktimeDownscaleFactor);
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
