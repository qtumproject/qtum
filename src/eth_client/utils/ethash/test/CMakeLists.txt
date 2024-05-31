# ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
# Copyright 2018 Pawel Bylica.
# Licensed under the Apache License, Version 2.0. See the LICENSE file.

add_subdirectory(benchmarks)
add_subdirectory(fakeminer)
add_subdirectory(integration)
add_subdirectory(tools)
add_subdirectory(unittests)

if(ETHASH_FUZZING)
    add_subdirectory(fuzzing)
endif()
