#!/bin/sh

# Setup Qtum build environment for Ubuntu 16

sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install autoconf automake binutils bison bsdmainutils ca-certificates curl faketime g++-8 gcc-8 git libtool patch pkg-config python3 python3-pip cmake libcurl4-openssl-dev libgmp-dev libmicrohttpd-dev libminiupnpc-dev -y
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 80 --slave /usr/bin/g++ g++ /usr/bin/g++-8 --slave /usr/bin/gcov gcov /usr/bin/gcov-8
