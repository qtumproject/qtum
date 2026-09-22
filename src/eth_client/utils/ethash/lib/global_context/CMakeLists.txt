# ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
# Copyright 2021 Pawel Bylica.
# Licensed under the Apache License, Version 2.0.

include(GNUInstallDirs)

add_library(global-context STATIC)
add_library(ethash::global-context ALIAS global-context)
target_link_libraries(global-context PUBLIC ethash::ethash)
target_include_directories(global-context PUBLIC $<BUILD_INTERFACE:${include_dir}>$<INSTALL_INTERFACE:include>)
set_target_properties(global-context PROPERTIES OUTPUT_NAME ethash-global-context)
target_sources(global-context PRIVATE
    ${include_dir}/ethash/global_context.h
    ${include_dir}/ethash/global_context.hpp
    global_context.cpp
)

if(CABLE_COMPILER_GNULIKE AND NOT MSVC AND NOT SANITIZE MATCHES undefined)
    target_compile_options(global-context PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
endif()

install(
    TARGETS global-context
    EXPORT ethashTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
