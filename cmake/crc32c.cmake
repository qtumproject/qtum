# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# This file handles compiler feature detection for hardware-accelerated CRC32c.

# Include necessary modules for checking compiler features.
include(CheckCXXSourceCompiles)
include(CheckSourceCompilesWithFlags)

# -----------------------------------------------------------------------------
# 1. Prefetch Support Checks
# -----------------------------------------------------------------------------

# Check for __builtin_prefetch support (GCC/Clang built-in).
check_cxx_source_compiles("
  int main() {
    char data = 0;
    const char* address = &data;
    // Check for the ability to issue a data prefetch instruction.
    __builtin_prefetch(address, 0, 0);
    return 0;
  }
  " HAVE_BUILTIN_PREFETCH
)

# Check for _mm_prefetch support (Intel/MSVC intrinsic).
check_cxx_source_compiles("
  #if defined(_MSC_VER)
  #include <intrin.h>
  #else
  #include <xmmintrin.h>
  #endif

  int main() {
    char data = 0;
    const char* address = &data;
    // Check for the specific x86/MSVC intrinsic. _MM_HINT_NTA is a common hint type.
    _mm_prefetch(address, _MM_HINT_NTA);
    return 0;
  }
  " HAVE_MM_PREFETCH
)

# -----------------------------------------------------------------------------
# 2. SSE4.2 Support Check (x86/x64)
# -----------------------------------------------------------------------------

# Set flags required to enable SSE4.2 intrinsics.
if(MSVC)
  # /arch:AVX generally enables all instruction sets up to AVX, which includes SSE4.2.
  set(SSE42_CXXFLAGS /arch:AVX)
else()
  # Standard flag for GCC/Clang to enable SSE4.2.
  set(SSE42_CXXFLAGS -msse4.2)
endif()

# Check for SSE4.2 support by compiling code that uses the CRC32 instruction intrinsic.
check_cxx_source_compiles_with_flags("
  #include <cstdint>
  #if defined(_MSC_VER)
  #include <intrin.h>
  #elif defined(__GNUC__) && defined(__SSE4_2__)
  #include <nmmintrin.h>
  #endif

  int main() {
    uint64_t l = 0;
    // _mm_crc32_u64 is a reliable way to check for SSE4.2 compiler support.
    l = _mm_crc32_u8(l, 0);
    l = _mm_crc32_u32(l, 0);
    l = _mm_crc32_u64(l, 0);
    return l;
  }
  " HAVE_SSE42
  CXXFLAGS ${SSE42_CXXFLAGS}
)

# -----------------------------------------------------------------------------
# 3. ARMv8 CRC/CRYPTO Support Check (AArch64)
# -----------------------------------------------------------------------------

# Set required flags for ARMv8 hardware acceleration extensions.
set(ARM64_CRC_CXXFLAGS -march=armv8-a+crc+crypto)

# Check for ARMv8 support by testing CRC32C and NEON intrinsics.
check_cxx_source_compiles_with_flags("
  #include <arm_acle.h>
  #include <arm_neon.h>

  int main() {
  #ifdef __aarch64__
    // Test for CRC32C intrinsics (byte, half-word, word, double-word)
    __crc32cb(0, 0); __crc32ch(0, 0); __crc32cw(0, 0); __crc32cd(0, 0);
    // Test for CRYPTO extension intrinsic (vmull_p64 used for polynomial multiplication)
    vmull_p64(0, 0);
  #else
  #error crc32c library does not support hardware acceleration on 32-bit ARM
  #endif
    return 0;
  }
  " HAVE_ARM64_CRC32C
  CXXFLAGS ${ARM64_CRC_CXXFLAGS}
)

# -----------------------------------------------------------------------------
# 4. Library Definition and Configuration
# -----------------------------------------------------------------------------

# Define the static library with portable implementation files.
add_library(crc32c STATIC EXCLUDE_FROM_ALL
  ${PROJECT_SOURCE_DIR}/src/crc32c/src/crc32c.cc
  ${PROJECT_SOURCE_DIR}/src/crc32c/src/crc32c_portable.cc
)

# Set compile definitions using Generator Expressions for conditional feature flags.
target_compile_definitions(crc32c PRIVATE
  HAVE_BUILTIN_PREFETCH=$<BOOL:${HAVE_BUILTIN_PREFETCH}>
  HAVE_MM_PREFETCH=$<BOOL:${HAVE_MM_PREFETCH}>
  # Note: HAVE_STRONG_GETAUXVAL check is assumed to be performed elsewhere.
  HAVE_STRONG_GETAUXVAL=$<BOOL:${HAVE_STRONG_GETAUXVAL}>
  BYTE_ORDER_BIG_ENDIAN=$<STREQUAL:${CMAKE_CXX_BYTE_ORDER},BIG_ENDIAN>
  HAVE_SSE42=$<BOOL:${HAVE_SSE42}>
  HAVE_ARM64_CRC32C=$<BOOL:${HAVE_ARM64_CRC32C}>
)

# Set public include directory. $<BUILD_INTERFACE> prevents the include path
# from leaking when the library is installed.
target_include_directories(crc32c
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/crc32c/include>
)

# Link against the core interface library (assuming it provides utilities/interfaces).
target_link_libraries(crc32c PRIVATE core_interface)
# Disable writing compile commands for this utility target.
set_target_properties(crc32c PROPERTIES EXPORT_COMPILE_COMMANDS OFF)

# -----------------------------------------------------------------------------
# 5. Conditional Source File Inclusion
# -----------------------------------------------------------------------------

# Include and apply specific flags for the SSE4.2 implementation if supported.
if(HAVE_SSE42)
  set(_crc32_src ${PROJECT_SOURCE_DIR}/src/crc32c/src/crc32c_sse42.cc)
  target_sources(crc32c PRIVATE ${_crc32_src})
  # Apply the aggressive SSE4.2 compilation flags only to the SSE-specific file.
  set_property(SOURCE ${_crc32_src} PROPERTY COMPILE_OPTIONS ${SSE42_CXXFLAGS})
endif()

# Include and apply specific flags for the ARMv8 implementation if supported.
if(HAVE_ARM64_CRC32C)
  set(_crc32_src ${PROJECT_SOURCE_DIR}/src/crc32c/src/crc32c_arm64.cc)
  target_sources(crc32c PRIVATE ${_crc32_src})
  # Apply the ARMv8 CRC/CRYPTO compilation flags only to the ARM-specific file.
  set_property(SOURCE ${_crc32_src} PROPERTY COMPILE_OPTIONS ${ARM64_CRC_CXXFLAGS})
endif()

# Clean up the temporary variable.
unset(_crc32_src)
