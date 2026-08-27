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
                      const dev::u256& _blockNumber,
                      const std::string& gasSuffix = std::string()):
        chainParams(_chainParams),
        blockNumber(_blockNumber),
        callName(_name)
    {
        // Get the executor and gas pricer for the precompiled contract
        exec = dev::eth::PrecompiledRegistrar::executor(_name);
        cost = dev::eth::PrecompiledRegistrar::pricer(_name);
        gasName = "Gas" + gasSuffix;
    }

    /**
     * @brief performTests Perform tests for the precompiled contract
     * @param jsondata List of tests to perform
     */
    void performTests(const std::string_view& jsondata)
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
                int gas = tv[gasName].getInt<int>();
                bool result = true;
                if(tv.exists("Result"))
                    result = tv["Result"].get_bool();

                dev::bytes in = dev::fromHex(strInput);
                dev::bytes expected = dev::fromHex(strExpected);

                // Check the precompiled contract
                pricerTest(in, gas, strName);
                executorTest(in, expected, strName, result);
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

    void executorTest(const dev::bytes& in, const dev::bytes& expected, const std::string& testName, bool result)
    {
        auto res = exec(dev::bytesConstRef(in.data(), in.size()), chainParams, blockNumber);
        BOOST_CHECK(res.first == result);
        BOOST_CHECK_EQUAL_COLLECTIONS(res.second.begin(), res.second.end(), expected.begin(), expected.end());
        BOOST_CHECK_MESSAGE(res.second == expected, strprintf("Output not correct for precompiled contract %s in test %s", callName, testName));
    }

    UniValue read_json(const std::string_view& jsondata)
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
    std::string gasName;
};

#define RunPrecompiledTestsBase(contract, data, params, blockNumber, gasSuffix)\
    do {\
        std::string name = #contract;\
        PrecompiledTester tester(name, params, blockNumber, gasSuffix);\
        tester.performTests(json_tests::data);\
    } while(false)

#define RunPrecompiledTests(contract, data, params, blockNumber) RunPrecompiledTestsBase(contract, data, params, blockNumber, "")

#define RunOldPrecompiledTests(contract, data, params, blockNumber) RunPrecompiledTestsBase(contract, data, params, blockNumber, "Old")

#define RunNewPrecompiledTests(contract, data, params, blockNumber) RunPrecompiledTestsBase(contract, data, params, blockNumber, "New")

#endif // QTUMTESTS_PRECOMPILED_UTILS_H
