#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Bitcoin Core developers
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
BECH32_VALID_CAPITALS = convert_btc_bech32_address_to_qtum('BCRT1QPLMTZKC2XHARPPZDLNPAQL78RSHJ68U33RAH7R')
BECH32_VALID_MULTISIG = convert_btc_bech32_address_to_qtum('bcrt1qdg3myrgvzw7ml9q0ejxhlkyxm7vl9r56yzkfgvzclrf4hkpx9yfqhpsuks')

BECH32_TOO_LONG = convert_btc_bech32_address_to_qtum('bcrt1q049edschfnwystcqnsvyfpj23mpsg3jcedq9xv049edschfnwystcqnsvyfpj23mpsg3jcedq9xv049edschfnwystcqnsvyfpj23m')
BECH32_ONE_ERROR = convert_btc_bech32_address_to_qtum('bcrt1q049edschfnwystcqnsvyfpj23mpsg3jcedq9xv')
BECH32_ONE_ERROR_CAPITALS = convert_btc_bech32_address_to_qtum('BCRT1QPLMTZKC2XHARPPZDLNPAQL78RSHJ68U32RAH7R')
BECH32_TWO_ERRORS = convert_btc_bech32_address_to_qtum('bcrt1qax9suht3qv95sw33xavx8crpxduefdrsvgsklu') # should be bcrt1qax9suht3qv95sw33wavx8crpxduefdrsvgsklx
BECH32_NO_SEPARATOR = convert_btc_bech32_address_to_qtum('bcrtq049ldschfnwystcqnsvyfpj23mpsg3jcedq9xv')
BECH32_INVALID_CHAR = convert_btc_bech32_address_to_qtum('bcrt1q04oldschfnwystcqnsvyfpj23mpsg3jcedq9xv')
BECH32_MULTISIG_TWO_ERRORS = convert_btc_bech32_address_to_qtum('bcrt1qdg3myrgvzw7ml8q0ejxhlkyxn7vl9r56yzkfgvzclrf4hkpx9yfqhpsuks')
BECH32_WRONG_VERSION = convert_btc_bech32_address_to_qtum('bcrt1ptmp74ayg7p24uslctssvjm06q5phz4yrxucgnv')

BASE58_VALID = convert_btc_address_to_qtum('mipcBbFg9gMiCh81Kj8tqqdgoZub1ZJRfn')
BASE58_INVALID_PREFIX = convert_btc_address_to_qtum('17VZNX1SN5NtKa8UQFxwQbFeFc3iqRYhem')
BASE58_INVALID_CHECKSUM = convert_btc_address_to_qtum('mipcBbFg9gMiCh81Kj8tqqdgoZub1ZJJfn')
BASE58_INVALID_LENGTH = convert_btc_address_to_qtum('2VKf7XKMrp4bVNVmuRbyCewkP8FhGLP2E54LHDPakr9Sq5mtU2')

INVALID_ADDRESS = 'asfah14i8fajz0123f'
INVALID_ADDRESS_2 = convert_btc_address_to_qtum('1q049ldschfnwystcqnsvyfpj23mpsg3jcedq9xv')

class InvalidAddressErrorMessageTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def check_valid(self, addr):
        info = self.nodes[0].validateaddress(addr)
        assert info['isvalid']
        assert 'error' not in info
        assert 'error_locations' not in info

    def check_invalid(self, addr, error_str, error_locations=None):
        res = self.nodes[0].validateaddress(addr)
        assert not res['isvalid']
        assert_equal(res['error'], error_str)
        if error_locations:
            assert_equal(res['error_locations'], error_locations)
        else:
            assert_equal(res['error_locations'], [])

    def test_validateaddress(self):
        # Invalid Bech32
        self.check_invalid(BECH32_INVALID_SIZE, 'Invalid Bech32 address data size')
        self.check_invalid(BECH32_INVALID_PREFIX, 'Not a valid Bech32 or Base58 encoding')
        self.check_invalid(BECH32_INVALID_BECH32, 'Version 1+ witness address must use Bech32m checksum')
        self.check_invalid(BECH32_INVALID_BECH32M, 'Version 0 witness address must use Bech32 checksum')
        self.check_invalid(BECH32_INVALID_VERSION, 'Invalid Bech32 address witness version')
        self.check_invalid(BECH32_INVALID_V0_SIZE, 'Invalid Bech32 v0 address data size')
        self.check_invalid(BECH32_TOO_LONG, 'Bech32 string too long', list(range(90, 108)))
        self.check_invalid(BECH32_ONE_ERROR, 'Invalid Bech32 checksum', [9])
        self.check_invalid(BECH32_TWO_ERRORS, 'Invalid Bech32 checksum', [22, 43])
        self.check_invalid(BECH32_ONE_ERROR_CAPITALS, 'Invalid Bech32 checksum', [38])
        self.check_invalid(BECH32_NO_SEPARATOR, 'Missing separator')
        self.check_invalid(BECH32_INVALID_CHAR, 'Invalid Base 32 character', [8])
        self.check_invalid(BECH32_MULTISIG_TWO_ERRORS, 'Invalid Bech32 checksum', [19, 30])
        self.check_invalid(BECH32_WRONG_VERSION, 'Invalid Bech32 checksum', [5])

        # Valid Bech32
        self.check_valid(BECH32_VALID)
        self.check_valid(BECH32_VALID_CAPITALS)
        self.check_valid(BECH32_VALID_MULTISIG)

        # Invalid Base58
        self.check_invalid(BASE58_INVALID_PREFIX, 'Invalid prefix for Base58-encoded address')
        self.check_invalid(BASE58_INVALID_CHECKSUM, 'Invalid checksum or length of Base58 address')
        self.check_invalid(BASE58_INVALID_LENGTH, 'Invalid checksum or length of Base58 address')

        # Valid Base58
        self.check_valid(BASE58_VALID)

        # Invalid address format
        self.check_invalid(INVALID_ADDRESS, 'Not a valid Bech32 or Base58 encoding')
        self.check_invalid(INVALID_ADDRESS_2, 'Not a valid Bech32 or Base58 encoding')

    def test_getaddressinfo(self):
        node = self.nodes[0]

        assert_raises_rpc_error(-5, "Invalid Bech32 address data size", node.getaddressinfo, BECH32_INVALID_SIZE)

        assert_raises_rpc_error(-5, "Not a valid Bech32 or Base58 encoding", node.getaddressinfo, BECH32_INVALID_PREFIX)

        assert_raises_rpc_error(-5, "Invalid prefix for Base58-encoded address", node.getaddressinfo, BASE58_INVALID_PREFIX)

        assert_raises_rpc_error(-5, "Not a valid Bech32 or Base58 encoding", node.getaddressinfo, INVALID_ADDRESS)

    def run_test(self):
        self.test_validateaddress()

        if self.is_wallet_compiled():
            self.init_wallet(node=0)
            self.test_getaddressinfo()


if __name__ == '__main__':
    InvalidAddressErrorMessageTest().main()
