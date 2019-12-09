#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

travis_retry sudo apt-get update -qq
travis_retry sudo apt-get install -qq --no-install-recommends --no-upgrade $PACKAGES $DOCKER_PACKAGES
travis_retry sudo add-apt-repository -y ppa:bitcoin/bitcoin
travis_retry sudo apt-get update -qq
travis_retry sudo apt-get install -qq libdb4.8++-dev
