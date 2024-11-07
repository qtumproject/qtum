# ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
# Copyright 2018 Pawel Bylica.
# Licensed under the Apache License, Version 2.0.

include(GoogleTest)

hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)

add_executable(
    ethash-test
    helpers.hpp
    test_cases.hpp
    test_difficulty.cpp
    test_ethash.cpp
    test_keccak.cpp
    test_global_context.cpp
    test_primes.cpp
    test_version.cpp
)

set_source_files_properties(test_version.cpp PROPERTIES COMPILE_DEFINITIONS TEST_PROJECT_VERSION="${PROJECT_VERSION}")

target_link_libraries(ethash-test PRIVATE ethash::global-context ethash::experimental GTest::gtest_main)
target_include_directories(ethash-test PRIVATE ${ETHASH_PRIVATE_INCLUDE_DIR})
set_target_properties(ethash-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ..)

if(SANITIZE AND CMAKE_SYSTEM_NAME STREQUAL Linux AND CMAKE_CXX_FLAGS MATCHES -m32)
    # Workaround for linking with phread when building for 32-bit and with sanitizer.
    target_link_libraries(ethash-test PRIVATE pthread)
endif()

gtest_discover_tests(ethash-test TEST_PREFIX ${PROJECT_NAME}/unittests/)
