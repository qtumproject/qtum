#!/bin/sh
# Copyright (c) 2013-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

set -e
srcdir="$(dirname $0)"
cd "$srcdir"
if [ -z ${LIBTOOLIZE} ] && GLIBTOOLIZE="`which glibtoolize 2>/dev/null`"; then
  LIBTOOLIZE="${GLIBTOOLIZE}"
  export LIBTOOLIZE
fi
which autoreconf >/dev/null || \
  (echo "configuration failed, please install autoconf first" && exit 1)
autoreconf --install --force --warnings=all

cd src/cpp-ethereum
if [ $# -ne 3 ]
then
    git submodule update --init
    ./scripts/install_deps.sh
    cmake .
    make -j `nproc`
else
    cmake -DCMAKE_SYSTEM_NAME=$1 -DCMAKE_C_COMPILER=/usr/bin/$2-gcc-posix -DCMAKE_CXX_COMPILER=/usr/bin/$2-g++-posix -DCMAKE_RC_COMPILER=/usr/bin/$2-windres -DCMAKE_FIND_ROOT_PATH=$3 -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY -DPLATFORM=$2 .
    make -j `nproc`
fi
