cmake_minimum_required(VERSION 3.15)
project(blst_wrapper C) # Language is externally handled via script

# Determine build script and output location
if(MSVC)
    set(BLST_BUILD_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/build.bat")
    set(BLST_OUTPUT_TEMP "${CMAKE_CURRENT_SOURCE_DIR}/blst.lib")
    set(BLST_OUTPUT "${CMAKE_BINARY_DIR}/lib/blst.lib")
else()
    # Build CC invocation with optional macOS flags

    # cmake pack the compiler and its flags into a list separated by semicolons
    # replace the semicolons with empty space to produce compiler path with flags
    if(NOT DEFINED BLST_CC)
        set(BLST_CC "${CMAKE_C_COMPILER}")
    endif()

    string(REPLACE ";" " " CC "${BLST_CC}")
    set(BLST_CC "${CC}")

    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        set(BLST_CC "${BLST_CC} -Wno-unused-command-line-argument")
    endif()

    if(CMAKE_OSX_SYSROOT)
        set(BLST_CC "${BLST_CC} -isysroot ${CMAKE_OSX_SYSROOT}")
    endif()

    if(CMAKE_OSX_DEPLOYMENT_TARGET)
        set(BLST_CC "${BLST_CC} -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
    endif()

    if(CMAKE_C_FLAGS)
        set(BLST_CC "${BLST_CC} ${CMAKE_C_FLAGS}")
    endif()

    set(BLST_BUILD_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/build.sh")
    set(BLST_OUTPUT_TEMP "${CMAKE_CURRENT_SOURCE_DIR}/libblst.a")
    set(BLST_OUTPUT "${CMAKE_BINARY_DIR}/lib/libblst.a")
endif()

# Shared build flags
set(BLST_BUILD_SCRIPT_FLAGS "-D__BLST_PORTABLE__")

# Custom build step
if(MSVC)
    # Windows: run batch script directly
    add_custom_command(
        OUTPUT ${BLST_OUTPUT_TEMP}
        COMMAND ${CMAKE_COMMAND} -E echo "[+] building blst via build.bat"
        COMMAND ${BLST_BUILD_SCRIPT} ${BLST_BUILD_SCRIPT_FLAGS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${BLST_BUILD_SCRIPT}
    )
else()
    # Unix-like: use env to set CC and AR
    add_custom_command(
        OUTPUT ${BLST_OUTPUT_TEMP}
        COMMAND ${CMAKE_COMMAND} -E echo "[+] building blst via build.sh"
        COMMAND ${CMAKE_COMMAND} -E env
                CC=${BLST_CC}
                AR=${CMAKE_AR}
                ${BLST_BUILD_SCRIPT} ${BLST_BUILD_SCRIPT_FLAGS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${BLST_BUILD_SCRIPT}
    )
endif()

# Moving blst to install libdir
add_custom_command(
    OUTPUT ${BLST_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E rename
        ${BLST_OUTPUT_TEMP}
        ${BLST_OUTPUT}
    DEPENDS ${BLST_OUTPUT_TEMP}
)

# Trigger the build step
add_custom_target(blst_build ALL DEPENDS ${BLST_OUTPUT})

# Register the external static library as an imported target
add_library(blst STATIC IMPORTED GLOBAL)
set_target_properties(blst PROPERTIES
    IMPORTED_LOCATION ${BLST_OUTPUT}
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/bindings"
)

add_dependencies(blst blst_build)

# Optional clean step
add_custom_target(blst_clean
    COMMAND ${CMAKE_COMMAND} -E echo "[+] cleaning blst artifacts"
    COMMAND ${CMAKE_COMMAND} -E rm -f ${BLST_OUTPUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
