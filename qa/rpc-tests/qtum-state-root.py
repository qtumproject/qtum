#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
import sys

class StateRootTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        self.is_network_split = False
        self.node = self.nodes[0]

    # verify that the state hash is not 0 on genesis
    def verify_not_null_test(self):
        block_hash = self.node.getblockhash(0)
        block = self.node.getblock(block_hash)
        assert(int(block['hashStateRoot'], 16) != 0)

    # verify that the state hash changes on contract creation
    def verify_state_hash_changes(self):
        amount = 20000*COIN
        self.node.generate(COINBASE_MATURITY+50)
        block_hash_a = self.node.getblockhash(COINBASE_MATURITY+50)
        block_a = self.node.getblock(block_hash_a)
        """
        pragma solidity ^0.4.10;
        contract Example {
            function () payable {}
        }
        """
        self.node.createcontract("60606040523415600b57fe5b5b60398060196000396000f30060606040525b600b5b5b565b0000a165627a7a7230582092926a9814888ff08700cbd86cf4ff8c50052f5fd894e794570d9551733591d60029")
        self.node.generate(1)
        block_hash_b = self.node.getblockhash(COINBASE_MATURITY+51)
        block_b = self.node.getblock(block_hash_b)
        assert(block_a['hashStateRoot'] != block_b['hashStateRoot'])

    # verify that the state hash remains the same on restart
    def verify_state_hash_remains_on_restart(self):
        block_hash_a = self.node.getblockhash(COINBASE_MATURITY+51)
        block_a = self.node.getblock(block_hash_a)
        stop_nodes(self.nodes)
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        self.node = self.nodes[0]
        self.node.generate(1)
        block_hash_b = self.node.getblockhash(COINBASE_MATURITY+52)
        block_b = self.node.getblock(block_hash_b)
        assert(block_a['hashStateRoot'] == block_b['hashStateRoot'])

    def run_test(self):
        self.verify_not_null_test()
        self.verify_state_hash_changes()
        self.verify_state_hash_remains_on_restart()

if __name__ == '__main__':
    StateRootTest().main()
