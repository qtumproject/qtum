# Ethash

[![readme style standard](https://img.shields.io/badge/readme%20style-standard-brightgreen.svg?style=flat-square)](https://github.com/RichardLitt/standard-readme)
[![GitPOAP Badge](https://public-api.gitpoap.io/v1/repo/chfast/ethash/badge)](https://www.gitpoap.io/gh/chfast/ethash)

> C/C++ implementation of Ethash – the Ethereum Proof of Work algorithm


## Table of Contents

- [Install](#install)
- [Usage](#usage)
- [Test vectors](#test-vectors)
- [Optimizations](#optimizations)
- [Maintainer](#maintainer)
- [License](#license)


## Install

Build from source using CMake.

```sh
mkdir build
cd build
cmake ..
cmake --build .
```

## Usage

See [ethash.hpp] for a list of exported functions and documentation.


## Optimizations

This section describes the optimizations, modifications and tweaks applied
in this library in relation to [Ethash reference implementation].

The library contains a set of micro-benchmarks.Build and run the `ethash-bench`
tool.

### Seed hash is computed on the fly.

Seed hash is a sequence of keccak256 hashes applied the epoch number of times.
Time needed to compute seed hash is negligible compared to time needed to build
light cache. Computing seed hash for epoch 10000 takes ~ 5 ms, building light
cache for epoch 1 takes ~ 500 ms.

### Dataset size is computed on the fly

Computing the size of the full dataset and light cache requires finding the largest
prime number given an upper bound. For similar reasons as with seed hash, this
is computed on the fly. The procedure used is quite naive and forks well only
up to 40-bit number, so some additional improvement can be done in the future.


## Maintainer

Paweł Bylica [@chfast]

## License

Licensed under the [Apache License, Version 2.0].


[@chfast]: https://github.com/chfast
[Apache License, Version 2.0]: LICENSE
[ethash.hpp]: include/ethash/ethash.hpp
[Ethash reference implementation]: https://github.com/ethereum/wiki/wiki/Ethash
