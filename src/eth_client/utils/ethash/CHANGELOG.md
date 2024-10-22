# Changelog

## [1.0.1] — 2023-04-03

- CMake and build improvements.
  [#230](https://github.com/chfast/ethash/pull/230)
  [#231](https://github.com/chfast/ethash/pull/231)

## [1.0.0] — 2022-08-25

- Removed: Deprecated function `ethash_verify()` has been removed.
  Use `ethash_verify_against_boundary()`.
  [#226](https://github.com/chfast/ethash/pull/226)

## [0.9.0] — 2022-04-05

- Added: The constant `ETHASH_MAX_EPOCH_NUMBER = 32639` representing
  the maximum supported Ethash epoch.
  [#212](https://github.com/chfast/ethash/pull/212)
- Removed: Previously deprecated ProgPoW implementation.
  [#207](https://github.com/chfast/ethash/pull/207)
  [#209](https://github.com/chfast/ethash/pull/209)
  [#210](https://github.com/chfast/ethash/pull/210)
- Removed: Support for using custom hash function for light cache generation.
  [#213](https://github.com/chfast/ethash/pull/213)
- Removed: The internal header builtins.h.
  [#208](https://github.com/chfast/ethash/pull/208)

## [0.8.0] — 2021-11-09

- Added: The new set of functions which verify Ethash hashes against difficulty
  has been added to the library: `verify_against_difficulty()` and
  `verify_final_hash_against_difficulty()`. These functions improve usability
  by omitting previously required difficulty to boundary conversion.
  The old verification function is still available under
  the `verify_against_boundary()` name but its usage is discouraged.
  [#195](https://github.com/chfast/ethash/pull/195)
- Added: The dedicated procedure to convert difficulty to boundary
  `ethash_difficulty_to_boundary()` has been added to the testing infrastructure
  but can be promoted to the public API if requested.
  [#191](https://github.com/chfast/ethash/pull/191)
- Deprecated: `verify()` has been deprecated and replaced with
  `verify_against_boundary()`.
  [#193](https://github.com/chfast/ethash/pull/193)
- Deprecated: `verify_final_hash()` has been replaced by
  `verify_final_hash_against_difficulty()`.
  [#200](https://github.com/chfast/ethash/pull/200)
- Deprecated: The ProgPoW support has been deprecated and will be removed 
  in the next release.
  [#201](https://github.com/chfast/ethash/pull/201)
- Changed: The internal hash type comparison operators have been 
  optimized and the dependency on the `memcmp()` function has been dropped.
  [#186](https://github.com/chfast/ethash/pull/186)
- Changed: Verification functions now return error codes instead of `bool`
  with additional information about which verification check has failed.
  [#187](https://github.com/chfast/ethash/pull/187)
  - In C API error codes are represented by the `ethash_errc` enum,
  - In C++ the error codes are wrapped into `std::error_code`.
- Changed: Some build requirements have been increased.
  - C11 is required (previously C99),
    [#188](https://github.com/chfast/ethash/pull/188)
  - C++14 is required (previously C++11),
    [#189](https://github.com/chfast/ethash/pull/189)
  - CMake 3.16 is required (previously 3.13),
    [#205](https://github.com/chfast/ethash/pull/205)
  - Python wheels are now built with `manylinux_2_24`.
    [#194](https://github.com/chfast/ethash/pull/194)


## [0.7.1] — 2021-08-26

 - Added: Support for building with clang-cl (LLVM-based toolchain for Visual Studio 2019).
   [#179](https://github.com/chfast/ethash/pull/179)
   [#180](https://github.com/chfast/ethash/pull/180)
   [#182](https://github.com/chfast/ethash/pull/182)
   [#183](https://github.com/chfast/ethash/pull/183)
   [#184](https://github.com/chfast/ethash/pull/184)

## [0.7.0] — 2021-05-26

 - Changed: The global context API (aka "managed" API) has been moved to
   separate library `ethash::global-context` and `ethash/global_context.hpp`
   header.
   [#175](https://github.com/chfast/ethash/pull/175)
 - Added: CMake options to disable building of individual libraries:
   - `-DETHASH_BUILD_GLOBAL_CONTEXT=NO` disables building
     `ethash::global-context`.
   - `-DETHASH_BUILD_ETHASH=NO` disables building `ethash::ethash` and
     `ethash::global-context`. Only `ethash::keccak` is built then.
 - Added: Basic support for building to WebAssembly
   [#175](https://github.com/chfast/ethash/pull/175)
   
## [0.6.0] — 2020-12-15

 - Added: The ethash::keccak library received the optimized Keccak implementation
   which uses BMI and BMI2 x86_64 extensions. This implementation is automatically
   selected at startup provided the used extensions are available in the hardware.
   [#162](https://github.com/chfast/ethash/pull/162)
   [#168](https://github.com/chfast/ethash/pull/168)

## [0.5.2] — 2020-08-03

 - Fixed: Fix compilation with MSVC/C++17.
   [#154](https://github.com/chfast/ethash/issues/154)

## [0.5.1] — 2020-01-30

 - Added: Experimental Python bindings — [ethash][pypi-ethash] package.
   [#123](https://github.com/chfast/ethash/pull/123)
   [#138](https://github.com/chfast/ethash/pull/138)
 - Added: More functions exposed in C API.
   [#136](https://github.com/chfast/ethash/pull/136)
 - Changed: ProgPoW implementation updated to revision [0.9.3][ProgPoW-changelog].
   [#151](https://github.com/chfast/ethash/pull/151)

## [0.5.0] — 2019-06-07

 - Changed:
   The Keccak implementation has been moved to separate library "keccak", 
   available as ethash::keccak target in the ethash CMake package.
   [#131](https://github.com/chfast/ethash/pull/131)

## [0.4.4] — 2019-02-26

 - Fixed:
   Fix compilation on PowerPC architectures (-mtune=generic not supported there).
   [#125](https://github.com/chfast/ethash/pull/125)

## [0.4.3] — 2019-02-19

 - Added:
   The public `version.h` header with information about the ethash library version.
   [#121](https://github.com/chfast/ethash/pull/121)
 - Added:
   Ethash and ProgPoW revisions exposed as `{ethash,progpow}::revision` constants.
   [#121](https://github.com/chfast/ethash/pull/121)

## [0.4.2] — 2019-01-24

 - Fixed: The `progpow.cpp` file encoding changed from utf-8 to ascii.

## [0.4.1] — 2018-12-14

 - Added: [KISS99 PRNG](https://en.wikipedia.org/wiki/KISS_(algorithm) distribution tester tool.
 - Changed: ProgPoW implementation updated to revision [0.9.2][ProgPoW-changelog].
 - Changed: ProgPoW implementation optimizations.

## [0.4.0] — 2018-12-04

 - Added: Experimental support for [ProgPoW] [0.9.1][ProgPoW-changelog].


[1.0.1]: https://github.com/chfast/ethash/releases/tag/v1.0.1
[1.0.0]: https://github.com/chfast/ethash/releases/tag/v1.0.0
[0.9.0]: https://github.com/chfast/ethash/releases/tag/v0.9.0
[0.8.0]: https://github.com/chfast/ethash/releases/tag/v0.8.0
[0.7.1]: https://github.com/chfast/ethash/releases/tag/v0.7.1
[0.7.0]: https://github.com/chfast/ethash/releases/tag/v0.7.0
[0.6.0]: https://github.com/chfast/ethash/releases/tag/v0.6.0
[0.5.2]: https://github.com/chfast/ethash/releases/tag/v0.5.2
[0.5.1]: https://github.com/chfast/ethash/releases/tag/v0.5.1
[0.5.0]: https://github.com/chfast/ethash/releases/tag/v0.5.0
[0.4.4]: https://github.com/chfast/ethash/releases/tag/v0.4.4
[0.4.3]: https://github.com/chfast/ethash/releases/tag/v0.4.3
[0.4.2]: https://github.com/chfast/ethash/releases/tag/v0.4.2
[0.4.1]: https://github.com/chfast/ethash/releases/tag/v0.4.1
[0.4.0]: https://github.com/chfast/ethash/releases/tag/v0.4.0

[ProgPoW]: https://github.com/ifdefelse/ProgPOW/blob/master/README.md
[ProgPoW-changelog]: https://github.com/ifdefelse/ProgPOW#change-history
[pypi-ethash]: https://pypi.org/project/ethash/
