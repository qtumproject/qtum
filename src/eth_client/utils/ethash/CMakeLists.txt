# ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
# Copyright 2018 Pawel Bylica.
# Licensed under the Apache License, Version 2.0.

cmake_minimum_required(VERSION 3.16.2...3.25)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(CableCompilerSettings)
include(CMakeDependentOption)
include(CMakePackageConfigHelpers)

option(ETHASH_INSTALL_CMAKE_CONFIG "Install CMake configuration scripts for find_package(CONFIG)" ON)
option(ETHASH_BUILD_ETHASH "Build ethash::ethash library (if NO only ethash::keccak is built)" YES)
cmake_dependent_option(ETHASH_BUILD_GLOBAL_CONTEXT "Build ethash::global-context library" YES "ETHASH_BUILD_ETHASH" NO)
option(ETHASH_TESTING "Build unit tests" NO)

if(ETHASH_TESTING)
    include(cmake/Hunter/init.cmake)
endif()

project(ethash)
set(PROJECT_VERSION 1.1.0)

cable_configure_compiler(NO_STACK_PROTECTION)
if(CABLE_COMPILER_GNULIKE)
    add_compile_options(
        # -Wcast-align  #TODO: Build fails on mips64
        -Wcast-qual
        -Wmissing-declarations
        -Wsign-conversion
        -Wundef
        -Wunreachable-code
        $<$<COMPILE_LANGUAGE:C>:-Wmissing-prototypes>
        $<$<CXX_COMPILER_ID:Clang>:-Wduplicate-enum>
        $<$<CXX_COMPILER_ID:Clang>:-Wnewline-eof>
        $<$<CXX_COMPILER_ID:Clang>:-Wunreachable-code-aggressive>
        $<$<CXX_COMPILER_ID:GNU>:-Wno-attributes>
    )

    if(MSVC) # clang-cl
        add_compile_options(
            -Wno-exit-time-destructors
            -Wno-global-constructors
            $<$<COMPILE_LANGUAGE:CXX>:-Wno-c++98-compat-pedantic>
            $<$<COMPILE_LANGUAGE:CXX>:-Wno-old-style-cast>
        )
    endif()
elseif(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    # For Win32 builds allow allocating more than 2 GB of memory.
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Og")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og")
    set(CMAKE_C_FLAGS_COVERAGE "${CMAKE_C_FLAGS_DEBUG} -fprofile-instr-generate -fcoverage-mapping")
    set(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-instr-generate -fcoverage-mapping")
elseif(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Og")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og")
    set(CMAKE_C_FLAGS_COVERAGE "${CMAKE_C_FLAGS_DEBUG} --coverage")
    set(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
endif()

set(include_dir ${PROJECT_SOURCE_DIR}/include)

add_subdirectory(lib)

if(ETHASH_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()


install(
    DIRECTORY
    ${include_dir}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(ETHASH_INSTALL_CMAKE_CONFIG)
    write_basic_package_version_file(ethashConfigVersion.cmake COMPATIBILITY SameMajorVersion)
    configure_package_config_file(cmake/Config.cmake.in ethashConfig.cmake INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ethash)

    install(
        EXPORT ethashTargets
        NAMESPACE ethash::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ethash
    )
    install(
        FILES
        ${CMAKE_CURRENT_BINARY_DIR}/ethashConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/ethashConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ethash
    )
endif()
