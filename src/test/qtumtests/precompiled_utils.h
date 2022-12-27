#ifndef QTUMTESTS_PRECOMPILED_UTILS_H
#define QTUMTESTS_PRECOMPILED_UTILS_H

#include <boost/test/unit_test.hpp>
#include <univalue.h>
#include <libethcore/Precompiled.h>

/**
 * @brief The PrecompiledTester class Tester for precompiled ETH contracts
 */
class PrecompiledTester
{
public:
    /**
     * @brief PrecompiledTester Constructor
     * @param _name Precompiled contract name
     * @param _chainParams ETH chain parameters
     * @param _blockNumber Current block number
     */
    PrecompiledTester(const std::string& _name,
                      const dev::eth::ChainOperationParams& _chainParams,
                      const dev::u256& _blockNumber):
        chainParams(_chainParams),
        blockNumber(_blockNumber),
        callName(_name)
    {
        // Get the executor and gas pricer for the precompiled contract
        exec = dev::eth::PrecompiledRegistrar::executor(_name);
        cost = dev::eth::PrecompiledRegistrar::pricer(_name);
    }

    /**
     * @brief performTests Perform tests for the precompiled contract
     * @param jsondata List of tests to perform
     */
    void performTests(const std::string& jsondata)
    {
        // Read tests
        UniValue json_tests = read_json(jsondata);

        // Check the executor and gas pricer
        BOOST_CHECK_MESSAGE(exec, strprintf("Executor not found for precompiled contract %s", callName));
        BOOST_CHECK_MESSAGE(cost, strprintf("Pricer not found for precompiled contract %s", callName));

        if(exec && cost)
        {
            // Perform the tests
            for (unsigned int idx = 0; idx < json_tests.size(); idx++)
            {
                // Get the test data
                const UniValue& tv = json_tests[idx];
                std::string strInput = tv["Input"].get_str();
                std::string strExpected = tv["Expected"].get_str();
                std::string strName = tv["Name"].get_str();
                int gas = tv["Gas"].get_int();

                dev::bytes in = dev::fromHex(strInput);
                dev::bytes expected = dev::fromHex(strExpected);

                // Check the precompiled contract
                pricerTest(in, gas, strName);
                executorTest(in, expected, strName);
            }
        }
    }

private:
    void pricerTest(const dev::bytes& in, const int& gas, const std::string& testName)
    {
        auto res = cost(dev::bytesConstRef(in.data(), in.size()), chainParams, blockNumber);
        BOOST_CHECK_EQUAL(static_cast<int>(res), gas);
        BOOST_CHECK_MESSAGE(static_cast<int>(res) == gas, strprintf("Gas not correct for precompiled contract %s in test %s", callName, testName));
    }

    void executorTest(const dev::bytes& in, const dev::bytes& expected, const std::string& testName)
    {
        auto res = exec(dev::bytesConstRef(in.data(), in.size()));
        BOOST_CHECK(res.first);
        BOOST_CHECK_EQUAL_COLLECTIONS(res.second.begin(), res.second.end(), expected.begin(), expected.end());
        BOOST_CHECK_MESSAGE(res.second == expected, strprintf("Output not correct for precompiled contract %s in test %s", callName, testName));
    }

    UniValue read_json(const std::string& jsondata)
    {
        UniValue v;

        if (!v.read(jsondata) || !v.isArray())
        {
            BOOST_ERROR("Parse error.");
            return UniValue(UniValue::VARR);
        }
        return v.get_array();
    }

    dev::eth::PrecompiledExecutor exec;
    dev::eth::PrecompiledPricer cost;
    dev::eth::ChainOperationParams chainParams;
    dev::u256 blockNumber;
    std::string callName;
};

#define RunPrecompiledTests(contract, data, params, blockNumber)\
    do {\
        std::string name = #contract;\
        PrecompiledTester tester(name, params, blockNumber);\
        std::string jsondata = std::string(json_tests::data, json_tests::data + sizeof(json_tests::data));\
        tester.performTests(jsondata);\
    } while(false)

#endif // QTUMTESTS_PRECOMPILED_UTILS_H
