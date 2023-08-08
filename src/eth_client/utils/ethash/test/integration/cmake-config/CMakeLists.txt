# Copyright 2018 Pawel Bylica.
# Licensed under the Apache License, Version 2.0. See the LICENSE file.

cmake_minimum_required(VERSION 3.5)

project(ethash-cmake-config-test)

find_package(ethash CONFIG REQUIRED)

add_executable(ethash-cmake-config-test cmake_config_test.cpp)
target_link_libraries(ethash-cmake-config-test ethash::ethash)
set_target_properties(ethash-cmake-config-test PROPERTIES CXX_STANDARD 11 CXX_EXTENSIONS OFF)
