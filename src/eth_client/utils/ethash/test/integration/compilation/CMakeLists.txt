# ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
# Copyright 2018 Pawel Bylica.
# Licensed under the Apache License, Version 2.0. See the LICENSE file.

file(GLOB c_sources ${PROJECT_SOURCE_DIR}/lib/ethash/*.c)

foreach(c_std 0 90 99 11)
    set(target test-compile-c${c_std})
    add_library(${target} STATIC ethash_header_test.c ${c_sources})
    target_link_libraries(${target} PRIVATE ethash)
    if(c_std)
        set_target_properties(${target} PROPERTIES C_STANDARD ${c_std})
    endif()
    set_target_properties(${target} PROPERTIES C_EXTENSIONS OFF)
endforeach()
