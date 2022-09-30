#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.address import *
from test_framework.qtum import *
import sys
import random
import time

class QtumPrioritizeCreateOverCallTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def restart_node(self):
        self.stop_nodes()
        self.start_nodes()
        self.node = self.nodes[0]


    def run_test(self):
        self.node = self.nodes[0]
        creator_address = self.node.getnewaddress()
        self.node.generate(1)
        sender_address = self.node.getnewaddress()
        self.node.generate(1)
        self.node.generate(COINBASE_MATURITY)
        self.node.sendtoaddress(creator_address, 100)
        self.node.sendtoaddress(sender_address, 101)
        fork_block_hash = self.node.generate(1)[0]
        fork_tip = self.node.getblock(fork_block_hash)

        """
        pragma solidity ^0.5;
        contract Test {
            function() payable external {
            }
        }
        """
        bytecode = "6080604052348015600f57600080fd5b50603280601d6000396000f3fe608060405200fea165627a7a72305820c90cd8221110d815df9b74f6910f10c84b24658eb89ef1413dab6a845b3e3c2a0029"
        self.contract_address = self.node.createcontract(bytecode, 1000000, 0.0000004, creator_address)['address']
        self.node.generate(1)
        # We set a higher fee for the call tx so that if it and the create tx is evaluated it will have a higher fee.
        self.node.sendtocontract(self.contract_address, "00", 1, 1000000, 0.000001, sender_address)
        self.node.generate(1)
        assert_equal(self.node.listcontracts()[self.contract_address], 1)

        # Create a longer fork that will cause a reorg
        alternate_tip_block_hash = fork_block_hash
        for i in range(3):
            block = create_block(int(alternate_tip_block_hash, 16), create_coinbase(self.node.getblockcount()+i-1), int(fork_tip['time']+1+i))
            block.solve()
            self.node.submitblock(bytes_to_hex_str(block.serialize()))
            alternate_tip_block_hash = block.hash

        # The contract should not exist in the fork's state
        assert(self.contract_address not in self.node.listcontracts())
        # remine the create and call txs
        self.node.generate(1)[0]
        # Make sure that the create tx is executed first
        assert_equal(self.node.listcontracts()[self.contract_address], 1)

if __name__ == '__main__':
    QtumPrioritizeCreateOverCallTest().main()
