#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.address import *
from test_framework.qtum import *

class QtumBlockNumberCorruptionTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-txindex=1']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.nodes[0].generate(COINBASE_MATURITY+50)
        self.node = self.nodes[0]
        """
        pragma solidity ^0.5;
        contract Test {
            uint256[] private blockNumbers;
            
            function () payable external {
                blockNumbers.push(block.number);
            }
        }
        """
        bytecode = '6080604052348015600f57600080fd5b50605e80601d6000396000f3fe6080604052600043908060018154018082558091505090600182039060005260206000200160009091929091909150555000fea165627a7a72305820eb843d8a9f268dc45a6823f885215f9b11d9192710d7631d71a29d4e78ea21cb0029'
        contract_address = self.node.createcontract(bytecode)['address']
        self.node.generate(1)
        self.node.sendtocontract(contract_address, '00')
        self.node.generate(1)
        self.restart_node(0, ['-checklevel=4'])


if __name__ == '__main__':
    QtumBlockNumberCorruptionTest().main()