Qtum Core
=========

https://qtum.org

What is Qtum?
-------------

Qtum is a new blockchain based on Bitcoin Core that integrates Ethereum based smart contracts. It implements an extensible design which is capable of adding more VMs, enabled primarily through the Account Abstraction Layer, which allows for an account based virtual machine to function on a UTXO based blockchain. 


Quickstart
----------
### Build on Ubuntu

This is a quick start script for compiling Qtum on  Ubuntu


    sudo apt-get install build-essential libtool autotools-dev automake pkg-config libssl-dev libevent-dev bsdmainutils git cmake libboost-all-dev
    sudo apt-get install software-properties-common
    sudo add-apt-repository ppa:bitcoin/bitcoin
    sudo apt-get update
    sudo apt-get install libdb4.8-dev libdb4.8++-dev

    # If you want to build the Qt GUI:
    sudo apt-get install libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler

    git clone https://github.com/qtumproject/qtum
    cd qtum
    # Update cpp-ethereum submodule
    git submodule update --init --recursive 

    # Note autogen will prompt to install some more dependencies if needed
    ./autogen.sh
    ./configure 
    make -j2

### Additional instructions for Ubuntu 14.04

Ubuntu 14.04 has many packages that are out of date by default, so before building Qtum you must update these:

    #update cmake from source
    sudo apt-get install build-essential
    wget http://www.cmake.org/files/v3.5/cmake-3.5.2.tar.gz
    tar xf cmake-3.5.2.tar.gz
    cd cmake-3.5.2
    ./bootstrap --system-curl
    make
    sudo apt-get install checkinstall

    # just push enter at any prompts for checkinstall
    sudo checkinstall

    #update gcc
    sudo add-apt-repository ppa:ubuntu-toolchain-r/test
    sudo apt-get update
    sudo apt-get install gcc-5 g++-5
    #update default compiler
    sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 1
    sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-5 1

    #install custom libleveldb (resolve -fPIC error) 
    #note: this is a bit of a hack, be cautious doing this on a critical machine
    git clone https://github.com/google/leveldb.git
    cd leveldb/
    make OPT="-fPIC -O2 -DNDEBUG"
    sudo cp out-static/lib* out-shared/lib* /usr/local/lib/
    cd include
    sudo cp -r leveldb /usr/local/include/
    sudo mkdir /usr/local/include/leveldb/helpers
    cd ..
    sudo cp helpers/memenv/memenv.h /usr/local/include/leveldb/helpers
    sudo ldconfig

Additionally when using `./configure` you may need to use if you encounter errors with libmemenv.a 

    ./configure --with-miniupnpc=no

### Build on OSX

The commands in this guide should be executed in a Terminal application.
The built-in one is located in `/Applications/Utilities/Terminal.app`.

#### Preparation

Install the OS X command line tools:

`xcode-select --install`

When the popup appears, click `Install`.

Then install [Homebrew](https://brew.sh).

#### Dependencies

    brew install cmake automake berkeley-db4 libtool boost --c++11 --without-single --without-static miniupnpc openssl pkg-config protobuf qt5 libevent imagemagick --with-librsvg

NOTE: Building with Qt4 is still supported, however, could result in a broken UI. Building with Qt5 is recommended.

#### Build Qtum Core

1. Clone the qtum source code and cd into `qtum`

        git clone --recursive https://github.com/qtumproject/qtum.git
        cd qtum
        git submodule update --init --recursive

2.  Build qtum-core:

    Configure and build the headless qtum binaries as well as the GUI (if Qt is found).

    You can disable the GUI build by passing `--without-gui` to configure.

        ./autogen.sh
        ./configure
        make

3.  It is recommended to build and run the unit tests:

        make check

### Run

Then you can either run the command-line daemon using `src/qtumd` and `src/qtum-cli`, or you can run the Qt GUI using `src/qt/qtum-qt`

2Gb of RAM is recommended for building Qtum. If you encounter internal compiler errors or out of memory errors in autogen, then you can modify the last line of autogen to be something like:

    make -j1

This will disable multicore building of some cpp-ethereum dependencies

For in-depth description of Sparknet and how to use Qtum for interacting with contracts, please see [sparknet-guide](doc/sparknet-guide.md).


License
-------

Qtum is GPLv3 licensed.


Development Process
-------------------

The `master` branch is regularly built and tested, but is not guaranteed to be
completely stable. [Tags](https://github.com/qtumproject/qtum/tags) are created
regularly to indicate new official, stable release versions of Qtum.

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md).

Developer IRC can be found on Freenode at #qtum-dev.

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write [unit tests](src/test/README.md) for new code, and to
submit new unit tests for old code. Unit tests can be compiled and run
(assuming they weren't disabled in configure) with: `make check`. Further details on running
and extending unit tests can be found in [/src/test/README.md](/src/test/README.md).

There are also [regression and integration tests](/qa) of the RPC interface, written
in Python, that are run automatically on the build server.
These tests can be run (if the [test dependencies](/qa) are installed) with: `qa/pull-tester/rpc-tests.py`

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.

