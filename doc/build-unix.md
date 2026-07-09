UNIX BUILD NOTES
====================
Some notes on how to build Qtum Core in Unix.

(For BSD specific instructions, see `build-*bsd.md` in this directory.)

To Build
---------------------

```bash
cmake -B build
```
Run `cmake -B build -LH` to see the full list of available options.

```bash
cmake --build build    # Append "-j N" for N parallel jobs
cmake --install build  # Optional
```

See below for instructions on how to [install the dependencies on popular Linux
distributions](#linux-distribution-specific-instructions), or the
[dependencies](#dependencies) section for a complete overview.

## Memory Requirements

C++ compilers are memory-hungry. It is recommended to have at least 1.5 GB of
memory available when compiling Qtum Core. On systems with less, gcc can be
tuned to conserve memory with additional `CMAKE_CXX_FLAGS`:


    cmake -B build -DCMAKE_CXX_FLAGS="--param ggc-min-expand=1 --param ggc-min-heapsize=32768"

Alternatively, or in addition, debugging information can be skipped for compilation.
For the default build type `RelWithDebInfo`, the default compile flags are
`-O2 -g`, and can be changed with:

    cmake -B build -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O2 -g0"

Finally, clang (often less resource hungry) can be used instead of gcc, which is used by default:

    cmake -B build -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang

## Linux Distribution Specific Instructions

### Ubuntu & Debian

#### Dependency Build Instructions

Build requirements for the latest Debian "stable" release, or the latest Ubuntu LTS release:

    sudo apt-get install build-essential cmake pkgconf python3 libgmp3-dev

For Debian "oldstable", or earlier Ubuntu LTS releases, you may need to pick a
later compiler version, according to the [dependencies](/doc/dependencies.md)
documentation.

Now, you can either build from self-compiled [depends](#dependencies) or install the required dependencies:

    sudo apt-get install libevent-dev libboost-all-dev

SQLite is required for the wallet:

    sudo apt install libsqlite3-dev

To build Qtum Core without the wallet, see [*Disable-wallet mode*](#disable-wallet-mode)

Cap'n Proto is needed for IPC functionality (see [multiprocess.md](multiprocess.md)):

    sudo apt-get install libcapnp-dev capnproto

Compile with `-DENABLE_IPC=OFF` if you do not need IPC functionality.

ZMQ-enabled binaries are compiled with `-DWITH_ZMQ=ON` and require the following dependency:

    sudo apt-get install libzmq3-dev

User-Space, Statically Defined Tracing (USDT) dependencies:

    sudo apt install systemtap-sdt-dev

GUI dependencies:

Qtum Core includes a GUI built with the cross-platform Qt Framework. To compile the GUI, we need to install
the necessary parts of Qt, the libqrencode and pass `-DBUILD_GUI=ON`. Skip if you don't intend to use the GUI.

    sudo apt-get install qt6-base-dev qt6-tools-dev qt6-l10n-tools qt6-tools-dev-tools libgl-dev

Additionally, to support Wayland protocol for modern desktop environments:

    sudo apt install qt6-wayland

The GUI will be able to encode addresses in QR codes unless this feature is explicitly disabled. To install libqrencode, run:

    sudo apt-get install libqrencode-dev

Otherwise, if you don't need QR encoding support, use the `-DWITH_QRENCODE=OFF` option to disable this feature in order to compile the GUI.


### Fedora

#### Dependency Build Instructions

Build requirements:

    sudo dnf install gcc-c++ cmake make python3 gmp-devel

Now, you can either build from self-compiled [depends](#dependencies) or install the required dependencies:

    sudo dnf install libevent-devel boost-devel

SQLite is required for the wallet:

    sudo dnf install sqlite-devel

To build Qtum Core without the wallet, see [*Disable-wallet mode*](#disable-wallet-mode)

ZMQ-enabled binaries are compiled with `-DWITH_ZMQ=ON` and require the following dependency:

    sudo dnf install zeromq-devel

User-Space, Statically Defined Tracing (USDT) dependencies:

    sudo dnf install systemtap-sdt-devel

Cap'n Proto is needed for IPC functionality (see [multiprocess.md](multiprocess.md)):

    sudo dnf install capnproto capnproto-devel

Compile with `-DENABLE_IPC=OFF` if you do not need IPC functionality.

GUI dependencies:

Qtum Core includes a GUI built with the cross-platform Qt Framework. To compile the GUI, we need to install
the necessary parts of Qt, the libqrencode and pass `-DBUILD_GUI=ON`. Skip if you don't intend to use the GUI.

    sudo dnf install qt6-qtbase-devel qt6-qttools-devel

Additionally, to support Wayland protocol for modern desktop environments:

    sudo dnf install qt6-qtwayland

The GUI will be able to encode addresses in QR codes unless this feature is explicitly disabled. To install libqrencode, run:

    sudo dnf install qrencode-devel

Otherwise, if you don't need QR encoding support, use the `-DWITH_QRENCODE=OFF` option to disable this feature in order to compile the GUI.

Dependency Build Instructions: CentOS
-------------------------------------

You need to build boost manually, and if it's not in standard library paths, you need to add `/path/to/boost/lib` into `LD_LIBRARY_PATH` env when building Qtum.

Build requirements:

    sudo yum install epel-release cmake
    sudo yum install gcc-c++ libevent-devel sqlite-devel capnproto capnproto-devel openssl-devel gmp-devel
    
To build with Qt 6 (recommended) you need the following:

    sudo yum install qt6-qtbase-devel qt6-qttools-devel qrencode-devel

### Ubuntu 16
#### Dependency Build Instructions
Build requirements:
```
./qtum/contrib/script/setup-ubuntu16.sh
```

#### Build Installation Package
Build Qtum:
```
cd qtum/contrib/script
./build-qtum-linux.sh -j2
```
The home folder for the installation package need to be `qtum/contrib/script`.
After the build finish, the installation package is present into `qtum/contrib/script`.
Installation package example: `qtum-22.1-x86_64-pc-linux-gnu.tar.gz`

#### Dependencies Installation Package

The package has the following dependencies when used on Ubuntu 16 machine that is not used for building Qtum:

`qtum-qt` require `libxcb-xinerama0` to be installed on Ubuntu 16 (both 32 and 64 bit versions):
```
sudo apt-get install libxcb-xinerama0 -y
```

Qtum require `GCC 7` standard library be installed for Ubuntu 16 only on 32 bit version:
```
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install gcc-7 g++-7 -y
```

### CentOS 7
#### Dependency Build Instructions
Build requirements:
```
su
./qtum/contrib/script/setup-centos7.sh
```
The operating system might restart after finish with installing the build requirements.

#### Build Installation Package
Build Qtum:
```
cd qtum/contrib/script
./build-qtum-linux.sh -j2
```
The home folder for the installation package need to be `qtum/contrib/script`.
After the build finish, the installation package is present into `qtum/contrib/script`.
Installation package example: `qtum-22.1-x86_64-pc-linux-gnu.tar.gz`

### Alpine

#### Dependency Build Instructions

Build requirements:

    apk add build-base cmake linux-headers pkgconf python3

Now, you can either build from self-compiled [depends](#dependencies) or install the required dependencies:

    apk add libevent-dev boost-dev

SQLite is required for the wallet:

    apk add sqlite-dev

To build Qtum Core without the wallet, see [*Disable-wallet mode*](#disable-wallet-mode)

Cap'n Proto is needed for IPC functionality (see [multiprocess.md](multiprocess.md)):

    apk add capnproto capnproto-dev

Compile with `-DENABLE_IPC=OFF` if you do not need IPC functionality.

ZMQ dependencies (provides ZMQ API):

    apk add zeromq-dev

User-Space, Statically Defined Tracing (USDT) is not supported or tested on Alpine Linux at this time.

GUI dependencies:

Qtum Core includes a GUI built with the cross-platform Qt Framework. To compile the GUI, we need to install
the necessary parts of Qt, the libqrencode and pass `-DBUILD_GUI=ON`. Skip if you don't intend to use the GUI.

    apk add qt6-qtbase-dev  qt6-qttools-dev

The GUI will be able to encode addresses in QR codes unless this feature is explicitly disabled. To install libqrencode, run:

    apk add libqrencode-dev

Otherwise, if you don't need QR encoding support, use the `-DWITH_QRENCODE=OFF` option to disable this feature in order to compile the GUI.

## Dependencies

See [dependencies.md](dependencies.md) for a complete overview, and
[depends](/depends/README.md) on how to compile them yourself, if you wish to
not use the packages of your Linux distribution.

Disable-wallet mode
--------------------
When the intention is to only run a P2P node, without a wallet, Qtum Core can
be compiled in disable-wallet mode with:

    cmake -B build -DENABLE_WALLET=OFF

In this case there is no dependency on SQLite.

Mining is also possible in disable-wallet mode using the `getblocktemplate` RPC call.

Setup and Build Example: Arch Linux
-----------------------------------
This example lists the steps necessary to setup and build a command line only distribution of the latest changes on Arch Linux:

    pacman --sync --needed capnproto cmake boost gcc git libevent make python sqlite gmp
    git clone https://github.com/qtumproject/qtum --recursive
    cd qtum/
    cmake -B build
    cmake --build build
    ctest --test-dir build
    ./build/bin/qtumd
    ./build/bin/qtum help

