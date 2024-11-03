# ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
# Copyright 2018-2019 Pawel Bylica.
# Licensed under the Apache License, Version 2.0.

include(GNUInstallDirs)

add_library(ethash)
add_library(ethash::ethash ALIAS ethash)
target_compile_features(ethash PUBLIC c_std_11 cxx_std_14)
set_target_properties(ethash PROPERTIES C_EXTENSIONS OFF CXX_EXTENSIONS OFF)
target_link_libraries(ethash PRIVATE ethash::keccak)
target_include_directories(ethash PUBLIC $<BUILD_INTERFACE:${include_dir}>$<INSTALL_INTERFACE:include>)
target_sources(ethash PRIVATE
    endianness.hpp
    ${include_dir}/ethash/ethash.h
    ${include_dir}/ethash/ethash.hpp
    ethash-internal.hpp
    ethash.cpp
    ${include_dir}/ethash/hash_types.h
    primes.h
    primes.c
)


if(CABLE_COMPILER_GNULIKE AND NOT MSVC AND NOT SANITIZE MATCHES undefined)
    target_compile_options(ethash PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
endif()

install(
    TARGETS ethash
    EXPORT ethashTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
