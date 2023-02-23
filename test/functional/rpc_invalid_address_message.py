#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test error messages for 'getaddressinfo' and 'validateaddress' RPC commands."""

from test_framework.test_framework import BitcoinTestFramework

from test_framework.qtum import convert_btc_bech32_address_to_qtum, convert_btc_address_to_qtum
from test_framework.segwit_addr import Encoding
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

BECH32_VALID = convert_btc_bech32_address_to_qtum('bcrt1qtmp74ayg7p24uslctssvjm06q5phz4yrxucgnv')
BECH32_INVALID_BECH32 = convert_btc_bech32_address_to_qtum('bcrt1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqdmchcc')
BECH32_INVALID_BECH32M = convert_btc_bech32_address_to_qtum('bcrt1qw508d6qejxtdg4y5r3zarvary0c5xw7k35mrzd', encoding=Encoding.BECH32M)
BECH32_INVALID_VERSION = convert_btc_bech32_address_to_qtum('bcrt130xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqynjegk')
BECH32_INVALID_SIZE = convert_btc_bech32_address_to_qtum('bcrt1s0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7v8n0nx0muaewav25430mtr')
BECH32_INVALID_V0_SIZE = convert_btc_bech32_address_to_qtum('bcrt1qw508d6qejxtdg4y5r3zarvary0c5xw7kqqq5k3my')
BECH32_INVALID_PREFIX = 'qc1q79ley3n84mj5pz8n879fzdqfz92qayjp74hrx6'

BASE58_VALID = convert_btc_address_to_qtum('mipcBbFg9gMiCh81Kj8tqqdgoZub1ZJRfn')
BASE58_INVALID_PREFIX = '17VZNX1SN5NtKa8UQFxwQbFeFc3iqRYhem'

INVALID_ADDRESS = 'asfah14i8fajz0123f'

class InvalidAddressErrorMessageTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_validateaddress(self):
        node = self.nodes[0]

        # Bech32
        info = node.validateaddress(BECH32_INVALID_SIZE)
        assert not info['isvalid']
        assert_equal(info['error'], 'Invalid Bech32 address data size')

        info = node.validateaddress(BECH32_INVALID_PREFIX)
        assert not info['isvalid']
        assert_equal(info['error'], 'Not a valid Bech32 or Base58 encoding')

        info = node.validateaddress(BECH32_INVALID_BECH32)
        assert not info['isvalid']
        assert_equal(info['error'], 'Version 1+ witness address must use Bech32m checksum')

        info = node.validateaddress(BECH32_INVALID_BECH32M)
        assert not info['isvalid']
        assert_equal(info['error'], 'Version 0 witness address must use Bech32 checksum')

        info = node.validateaddress(BECH32_INVALID_V0_SIZE)
        assert not info['isvalid']
        assert_equal(info['error'], 'Invalid Bech32 v0 address data size')

        info = node.validateaddress(BECH32_VALID)
        assert info['isvalid']
        assert 'error' not in info

        # Base58
        info = node.validateaddress(BASE58_INVALID_PREFIX)
        assert not info['isvalid']
        assert_equal(info['error'], 'Invalid prefix for Base58-encoded address')

        info = node.validateaddress(BASE58_VALID)
        assert info['isvalid']
        assert 'error' not in info

        # Invalid address format
        info = node.validateaddress(INVALID_ADDRESS)
        assert not info['isvalid']
        assert_equal(info['error'], 'Not a valid Bech32 or Base58 encoding')

    def test_getaddressinfo(self):
        node = self.nodes[0]

        assert_raises_rpc_error(-5, "Invalid Bech32 address data size", node.getaddressinfo, BECH32_INVALID_SIZE)

        assert_raises_rpc_error(-5, "Not a valid Bech32 or Base58 encoding", node.getaddressinfo, BECH32_INVALID_PREFIX)

        assert_raises_rpc_error(-5, "Invalid prefix for Base58-encoded address", node.getaddressinfo, BASE58_INVALID_PREFIX)

        assert_raises_rpc_error(-5, "Not a valid Bech32 or Base58 encoding", node.getaddressinfo, INVALID_ADDRESS)

    def run_test(self):
        self.test_validateaddress()
        self.test_getaddressinfo()


if __name__ == '__main__':
    InvalidAddressErrorMessageTest().main()
