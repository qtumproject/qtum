#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.address import *
from test_framework.qtum import *

class QtumEVMConstantinopleActivationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-txindex=1', '-logevents=1', '-constantinopleheight=552']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.nodes[0].generate(COINBASE_MATURITY+50)
        self.node = self.nodes[0]

        """
            pragma solidity ^0.5;

            contract PostConstantinopleOpCodesTest {
                constructor() public {
                    uint256 x;
                    assembly {
                        x := shl(2, 2)
                    }
                }
            }
        """
        bytecode = "6080604052348015600f57600080fd5b5060006002801b90505060358060266000396000f3fe6080604052600080fdfea165627a7a7230582050b41bb4079fe4c4ec2ab007d83304ad1680585dd519d188151a4ff4af2f4ded0029"
        for i in range(5):
            self.node.createcontract(bytecode)['address']
            self.node.generate(1)

if __name__ == '__main__':
    QtumEVMConstantinopleActivationTest().main()
