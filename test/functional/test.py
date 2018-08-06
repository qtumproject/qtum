#!/usr/bin/env python3
# Copyright (c) 2014-2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the wallet accounts properly when there are cloned transactions with malleated scriptsigs."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.qtumconfig import INITIAL_BLOCK_REWARD

class TxnMallTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4

    def add_options(self, parser):
        parser.add_option("--mineblock", dest="mine_block", default=False, action="store_true",
                          help="Test double-spend of 1-confirmed transaction")
        parser.add_option("--segwit", dest="segwit", default=False, action="store_true",
                          help="Test behaviour with SegWit txn (which should fail")

    def setup_network(self):
        # Start with split network:
        super(TxnMallTest, self).setup_network()
        disconnect_nodes(self.nodes[1], 2)
        disconnect_nodes(self.nodes[2], 1)

    def run_test(self):
        if self.options.segwit:
            output_type="p2sh-segwit"
        else:
            output_type="legacy"
        # All nodes should start with 1,250 BTC:
        starting_balance = 25*INITIAL_BLOCK_REWARD
        for i in range(4):
            assert_equal(self.nodes[i].getbalance(), starting_balance)
            self.nodes[i].getnewaddress("")  # bug workaround, coins generated assigned to first getnewaddress!

        node0_address_foo = self.nodes[0].getnewaddress("", output_type)
        fund_foo_txid = self.nodes[0].sendtoaddress(node0_address_foo, 1219)
        fund_foo_tx = self.nodes[0].gettransaction(fund_foo_txid)

        node1_address = self.nodes[1].getnewaddress("from0", output_type)
        self.nodes[0].generate(1)

        # Send tx1, and another transaction tx2 that won't be cloned 
        txid1 = self.nodes[0].sendtoaddress(node1_address, 1218)
        self.nodes[0].getrawtransaction(txid1, True)


if __name__ == '__main__':
    TxnMallTest().main()

