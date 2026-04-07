# ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
# Copyright 2018-2019 Pawel Bylica.
# Licensed under the Apache License, Version 2.0.

find_package(Threads)

add_executable(ethash-fakeminer fakeminer.cpp)
target_link_libraries(ethash-fakeminer PRIVATE ethash::global-context Threads::Threads)
set_target_properties(ethash-fakeminer PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
