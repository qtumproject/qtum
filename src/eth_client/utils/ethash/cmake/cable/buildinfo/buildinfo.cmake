# Cable: CMake Bootstrap Library.
# Copyright 2018 Pawel Bylica.
# Licensed under the Apache License, Version 2.0. See the LICENSE file.

string(TOLOWER "${SYSTEM_NAME}" SYSTEM_NAME)
string(TOLOWER "${SYSTEM_PROCESSOR}" SYSTEM_PROCESSOR)
string(TOLOWER "${COMPILER_ID}" COMPILER_ID)
string(TOLOWER "${BUILD_TYPE}" BUILD_TYPE)
string(TIMESTAMP TIMESTAMP)

# Read the git info from a file. The gitinfo is suppose to update the file
# only if the information has changed.
file(STRINGS ${OUTPUT_DIR}/gitinfo.txt gitinfo)
list(LENGTH gitinfo gitinfo_len)
if(gitinfo_len LESS 3)
    message(WARNING "Git info not available")
else()
    list(GET gitinfo 0 describe)
    list(GET gitinfo 1 GIT_BRANCH)
    list(GET gitinfo 2 GIT_ORIGIN_URL)
endif()

# The output of `git describe --always --long --tags --match=v*`.
string(REGEX MATCH "(v(.+)-([0-9]+)-g)?([0-9a-f]+)(-dirty)?" match "${describe}")

if(DEFINED describe AND NOT match)
    message(WARNING "Cannot parse git describe: ${describe}")
endif()

set(GIT_LATEST_PROJECT_VERSION ${CMAKE_MATCH_2})
set(GIT_LATEST_PROJECT_VERSION_DISTANCE ${CMAKE_MATCH_3})
set(GIT_COMMIT_HASH ${CMAKE_MATCH_4})
if(CMAKE_MATCH_5)
    set(GIT_DIRTY TRUE)
    set(dirty_msg " (dirty)")
else()
    set(GIT_DIRTY FALSE)
endif()

if(GIT_COMMIT_HASH)
    string(SUBSTRING ${GIT_COMMIT_HASH} 0 8 abbrev)
    set(version_commit "+commit.${abbrev}")
    if(GIT_DIRTY)
        set(version_commit "${version_commit}.dirty")
    endif()
endif()

if(NOT PROJECT_VERSION)
    message(WARNING "PROJECT_VERSION not specified")
endif()

if(PROJECT_VERSION STREQUAL GIT_LATEST_PROJECT_VERSION)
    if(${GIT_LATEST_PROJECT_VERSION_DISTANCE} GREATER 0)
        set(PROJECT_VERSION "${PROJECT_VERSION}-${GIT_LATEST_PROJECT_VERSION_DISTANCE}${version_commit}")
    endif()
else()
    if(GIT_LATEST_PROJECT_VERSION)
        message(WARNING "Git project version mismatch: '${GIT_LATEST_PROJECT_VERSION}' vs '${PROJECT_VERSION}'")
    endif()
    set(PROJECT_VERSION "${PROJECT_VERSION}${version_commit}")
endif()

if(PROJECT_VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
    set(PROJECT_VERSION_IS_PRERELEASE false)
else()
    set(PROJECT_VERSION_IS_PRERELEASE true)
    set(prerelease_comment " (prerelease)")
endif()

message(
    "       Project Version:  ${PROJECT_VERSION}${prerelease_comment}\n"
    "       System Name:      ${SYSTEM_NAME}\n"
    "       System Processor: ${SYSTEM_PROCESSOR}\n"
    "       Compiler ID:      ${COMPILER_ID}\n"
    "       Compiler Version: ${COMPILER_VERSION}\n"
    "       Build Type:       ${BUILD_TYPE}\n"
    "       Git Info:         ${GIT_LATEST_PROJECT_VERSION} ${GIT_LATEST_PROJECT_VERSION_DISTANCE} ${GIT_COMMIT_HASH}${dirty_msg}\n"
    "       Timestamp:        ${TIMESTAMP}"
)

configure_file(${CMAKE_CURRENT_LIST_DIR}/buildinfo.c.in ${OUTPUT_DIR}/buildinfo.c)
configure_file(${CMAKE_CURRENT_LIST_DIR}/buildinfo.json.in ${OUTPUT_DIR}/buildinfo.json)
configure_file(${CMAKE_CURRENT_LIST_DIR}/buildinfo.sh.in ${OUTPUT_DIR}/buildinfo.sh)
configure_file(${CMAKE_CURRENT_LIST_DIR}/buildinfo.ps1.in ${OUTPUT_DIR}/buildinfo.ps1)
