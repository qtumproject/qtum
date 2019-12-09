#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

TRAVIS_COMMIT_LOG=$(git log --format=fuller -1)
export TRAVIS_COMMIT_LOG

QTUM_CONFIG_ALL=""
if [ -z "$NO_DEPENDS" ]; then
  ccache --max-size=$CCACHE_SIZE
fi

BEGIN_FOLD autogen
./autogen.sh
END_FOLD

BEGIN_FOLD configure
./configure --cache-file=../config.cache $QTUM_CONFIG_ALL $QTUM_CONFIG || ( cat config.log && false)
END_FOLD

BEGIN_FOLD build
sudo make $MAKEJOBS $GOAL || ( echo "Build failure. Verbose build follows." && make $GOAL V=1 ; false )
END_FOLD
