# ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
# Copyright 2021 Pawel Bylica.
# Licensed under the Apache License, Version 2.0.

add_library(experimental STATIC)
add_library(ethash::experimental ALIAS experimental)
target_link_libraries(experimental PUBLIC ethash::ethash)
target_sources(
    experimental PRIVATE
    difficulty.cpp
    difficulty.h
)
