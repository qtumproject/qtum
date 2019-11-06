What is Qtum?
-------------

Qtum is a decentralized blockchain project built on Bitcoin's UTXO model, with support for Ethereum Virtual Machine based smart contracts, and secured by a proof of stake consensus model. It achieves this through the revolutionary Account Abstraction Layer which allows the EVM to communicate with Qtum's Bitcoin-like UTXO blockchain. For more general information about Qtum as well as links to join our community, go to https://qtum.org

Welcome to the Qtum Ignition Main Network. This is the main network where the tokens hold value and should be guarded very carefully. If you are testing the network, or developing unstable software on Qtum, we highly recommend using either testnet or regtest mode. 

The major features of the Qtum network include:

1. Compatibility with the Ethereum Virtual Machine, which allows for compatibility with most existing Solidity based smart contracts. No special solidity compiler is required to deploy your smart contract to Qtum. 
2. A Proof of Stake consensus system which is optimized for Qtum's contract model. Any user can stake and help to secure the network. There is no voting, master nodes, or minimum amount required. There have been transactions as small as 2 Qtum that have created blocks in the past. Staking from smart contracts is under development.
3. The Decentralized Governance Protocol is completely implemented and functional, which allows certain network parameters to be modified without a fork or other network disruption. This currently controls parameters like block size, gas prices, etc. 
4. Uses the UTXO transaction model and is compatible with Bitcoin, allowing for existing tooling and workflows to be used with Qtum. This allows for the infamous SPV protocol to be used which is ideal for light wallets on mobile phones and IoT devices.

Note: Qtum Core is considered beta software. We make no warranties or guarantees of its security or stability.

Qtum Documentation and Usage Resources
---------------

These are some resources that might be helpful in understanding Qtum.

Basic usage resources:

* [User guide](https://docs.qtum.site/en/)
* [Qtum-Qt Wallet Tutorial](https://github.com/qtumproject/documents/tree/master/en/Qtum-Wallet-Tutorial#qtum-qt-wallet-tutorial)
* [Qtum staking tutorial](https://github.com/qtumproject/documents/tree/master/en/How-to-Stake-with-Qtum)
* [Qtum tutorial on Raspberry Pi](https://github.com/qtumproject/documents/tree/master/en/Qtum-on-Raspberry-Pi)
* [Running Qtum on Amazon AWS using Qtum AMI](https://github.com/qtumproject/documents/tree/master/en/Qtum-AWS)
* [Encrypting and unlocking wallet](https://github.com/qtumproject/documents/tree/master/en/Encrypt-and-Unlock-Qtum-Wallet)
* [Testnet guide](https://github.com/qtumproject/documents/blob/master/en/Testnet-User-Guide.md)
* [Block explorer](https://qtum.info), [Older explorer](https://explorer.qtum.org)


Development resources:

* [Qtum Book - A Developer's Guide To QTUM](http://book.qtum.site/en/)
* [Qtum contract usage](https://docs.qtum.site/en/QRC20-Token-Introduce.html)
* [Deploying a custom token to Qtum](https://blog.qtum.org/qtum-custom-token-walkthrough-467d725fa27d)
* [QtumJS document](https://qtumproject.github.io/qtumjs-doc/)
* [Smart contract ICO example](http://book.qtum.site/en/part3/ico.htm)
* [Early example faucet contract](http://earlz.net/view/2017/06/30/2144/the-qtum-sparknet-faucet)

General Info about Qtum:

* [CIRCLERESEARCH Qtum Primer](https://www.circle.com/marketing/pdfs/research/circle-research-qtum.pdf)
* [Qtum's PoS vs CASPER](https://www.reddit.com/r/Qtum/comments/788oa5/qtums_pos_vs_casper_and_the_nothingatstake_problem/)
* [Technical article explaining Qtum's PoS model in depth](http://earlz.net/view/2017/07/27/1904/the-missing-explanation-of-proof-of-stake-version)


Developer's Tools
-----------------

* Smart contract deployment tool
  * https://github.com/qtumproject/solar
* DApp JavaScript Library
  * https://github.com/qtumproject/qtumjs
* A toolkit for building qtum light wallets
  * https://github.com/qtumproject/qtumjs-wallet
* CORS qtumd RPC proxy for DApp
  * https://github.com/qtumproject/qtumportal
* Docker images for running qtum services
  * https://github.com/qtumproject/qtum-docker
* qtum.info API that powers the block explorer and the QTUM Web wallet
  * https://github.com/qtumproject/qtuminfo-api#qtuminfo-api-documentation


What is Qtum Core?
------------------

Qtum Core is our primary mainnet wallet. It implements a full node and is capable of storing, validating, and distributing all history of the Qtum network. Qtum Core is considered the reference implementation for the Qtum network. 

Qtum Core currently implements the following:

* Sending/Receiving QTUM coins
* Sending/Receiving QRC20 tokens on the Qtum network
* Staking and creating blocks for the Qtum network
* Creating and interacting with smart contracts
* Running a full node for distributing the blockchain to other users
* "Prune" mode, which minimizes disk usage
* Regtest mode, which enables developers to very quickly build their own private Qtum network for Dapp testing
* Testnet mode, using the public Qtum Testnet, with faucet available
* Compatibility with the Bitcoin Core set of RPC commands and APIs
* Full SegWit capability with p2sh-segwit (legacy) and bech32 (native) addresses

Alternative Wallets
-------------------

Qtum Core uses a full node model, and thus requires downloading the entire blockchain. If you do not need the entire blockchain, and do not intend on developing smart contracts, it may be more ideal to use an alternative wallet such as one of our light wallets that can be synchronized in a matter of seconds. 

### Qtum Web Wallet

A browser wallet that supports the Ledger hardware wallet, offline cold wallet, and restoration from mobile wallets.

Web site https://qtumwallet.org

User documentation https://github.com/qtumproject/documents/tree/master/en/QTUM-WebWallet-usage

### Qtum Electrum

A light wallet that supports the Ledger and Trezor hardware wallets and is based on the well-known Electrum wallet software. 

Download: https://github.com/qtumproject/qtum-electrum/releases

### iOS and Android Wallets

These wallets run on mobile devices and synchronize quickly. 

Android Download: https://play.google.com/store/apps/details?id=org.qtum.wallet

iOS Download: https://github.com/qtumproject/qtum-ios (open source, we are still working with Apple to get approval for their app store)

### Ledger Chrome Wallet

This light wallet runs in your Chrome browser as a browser extension. This wallet requires a Ledger device to use.

How to install: https://ledger.zendesk.com/hc/en-us/articles/115003776913-How-to-install-and-use-Qtum-with-Ledger


### Community Resources

Make sure to check out these resources as well for more information and to keep up to date with all the latest news about Qtum. At least 1 developer is always around, so if you're developing on Qtum and need help, we'd love to welcome you to our community.

*	@QtumOfficial on Twitter https://twitter.com/qtumofficial
* Qtum blog https://blog.qtum.org/
*	Qtum Telegram Group https://t.me/qtumofficial, other languages available
* Qtum Discord https://discordapp.com/invite/wRfmkQ9
*	/r/Qtum on Reddit https://www.reddit.com/r/Qtum/
*	Qtum.org https://qtum.org
*	Qtum on Facebook https://www.facebook.com/QtumOfficial/
*	Qtum Forum https://forum.qtum.org

### Qtum Smart Contract Limitations

*	EVM smart contracts cannot receive coins from or send coins to any address type other than pay-to-pubkeyhash (starts with Q) addresses. This is due to a limitation in the EVM
*	Contracts are not allowed to create contracts with an initial endowment of coins. The contract must first be created, and then be sent coins in a separate transaction. Humans are also not allowed to create contracts with an initial endowment of coins.
*	Although all of the infrastructure is present, Qtum Core does not currently parse Solidity event data. You must parse this yourself using either searchlogs or -record-log-opcodes features.
*	It is not possible to send a contract coins without also executing the contract. This is also the case of Ethereum. This was promised in earlier discussions and technically does work, but due to lack of time for testing this feature was disabled. We hope to reenable this feature with release of the x86 virtual machine in 2020.
*	In Qtum there can be multiple addresses used to create a proof-of-stake block. However, the EVM can only see the first output using the coinbase operation in Solidity (this address is also the one registered for the continuous staker rewards after 500 blocks).

----------

# Building Qtum Core

### Validate and Reproduce Binaries

Qtum uses a tool called Gitian to make reproducible builds that can be verified by anyone. Instructions on setting up a Gitian VM and building Qtum are provided in [Gitan Building](https://github.com/qtumproject/qtum/blob/master/doc/gitian-building.md)

### Build on Ubuntu

This is a quick start script for compiling Qtum on Ubuntu


    sudo apt-get install build-essential libtool autotools-dev automake pkg-config libssl-dev libevent-dev bsdmainutils git cmake libboost-all-dev libgmp3-dev
    sudo apt-get install software-properties-common
    sudo add-apt-repository ppa:bitcoin/bitcoin
    sudo apt-get update
    sudo apt-get install libdb4.8-dev libdb4.8++-dev

    # If you want to build the Qt GUI:
    sudo apt-get install libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler qrencode

    git clone https://github.com/qtumproject/qtum --recursive
    cd qtum

    # Note autogen will prompt to install some more dependencies if needed
    ./autogen.sh
    ./configure 
    make -j2
    
### Build on CentOS

Here is a brief description for compiling Qtum on CentOS, for more details please refer to [the specific document](https://github.com/qtumproject/qtum/blob/master/doc/build-unix.md)

    # Compiling boost manually
    sudo yum install python-devel bzip2-devel
    git clone https://github.com/boostorg/boost.git
    cd boost
    git checkout boost-1.66.0
    git submodule update --init --recursive
    ./bootstrap.sh --prefix=/usr --libdir=/usr/lib64
    ./b2 headers
    sudo ./b2 -j4 install
    
    # Installing Dependencies for Qtum
    sudo yum install epel-release
    sudo yum install libtool libdb4-cxx-devel openssl-devel libevent-devel gmp-devel
    
    # If you want to build the Qt GUI:
    sudo yum install qt5-qttools-devel protobuf-devel qrencode-devel
    
    # Building Qtum
    git clone --recursive https://github.com/qtumproject/qtum.git
    cd qtum
    ./autogen.sh
    ./configure
    make -j4

### Build on Mac OS

The commands in this guide should be executed in a Terminal application.
The built-in one is located in `/Applications/Utilities/Terminal.app`.

#### Preparation

Install the Mac OS command line tools:

`xcode-select --install`

When the popup appears, click `Install`.

Then install [Homebrew](https://brew.sh).

#### Dependencies

    brew install cmake automake berkeley-db4 libtool boost miniupnpc openssl pkg-config protobuf qt5 libevent imagemagick librsvg qrencode gmp

NOTE: Building with Qt4 is still supported, however, could result in a broken UI. Building with Qt5 is recommended.

#### Build Qtum Core

1. Clone the qtum source code and cd into `qtum`

        git clone --recursive https://github.com/qtumproject/qtum.git
        cd qtum

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

License
-------

Qtum is GPLv3 licensed.


Development Process
-------------------

The `master` branch is regularly built and tested, but is not guaranteed to be
completely stable. [Tags](https://github.com/qtumproject/qtum/tags) are created
regularly to indicate new official, stable release versions of Qtum.

The contribution workflow is described in [CONTRIBUTING.md](https://github.com/qtumproject/qtum/blob/master/CONTRIBUTING.md).


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

There are also [regression and integration tests](/test), written
in Python, that are run automatically on the build server.
These tests can be run (if the [test dependencies](/test) are installed) with: `test/functional/test_runner.py`

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.
