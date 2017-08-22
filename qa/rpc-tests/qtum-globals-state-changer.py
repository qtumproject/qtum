#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.address import *

class QtumGlobalsStateChangerTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        self.is_network_split = False
        self.node = self.nodes[0]

    def run_test(self):
        self.nodes[0].generate(10 + COINBASE_MATURITY)

        """
        pragma solidity ^0.4.12;

        contract Test {
            uint public stateChanger;
            
            function blkblockhash() payable {
                stateChanger = uint(block.blockhash(block.number-1));
            }

            function blkcoinbase() payable {
                stateChanger = uint(block.coinbase);
            }

            function blkdifficulty() payable {
                stateChanger = block.difficulty;
            }

            function blkgaslimit() payable {
                stateChanger = block.gaslimit;
            }

            function blknumber() payable {
                stateChanger = block.number;
            }
            
            function blktimestamp() payable {
                stateChanger = block.timestamp;
            }
        }
        73c9330d blkblockhash()
        5facaf8e blkcoinbase()
        009e8657 blkdifficulty()
        49b573d4 blkgaslimit()
        6bde2d73 blknumber()
        c1a5257b blktimestamp()
        6c7804b5 stateChanger()
        """
        contract_bytecode = "6060604052341561000f57600080fd5b5b6101738061001f6000396000f30060606040523615610080576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680629e86571461008557806349b573d41461008f5780635facaf8e146100995780636bde2d73146100a35780636c7804b5146100ad57806373c9330d146100d6578063c1a5257b146100e0575b600080fd5b61008d6100ea565b005b6100976100f4565b005b6100a16100fe565b005b6100ab61011e565b005b34156100b857600080fd5b6100c0610128565b6040518082815260200191505060405180910390f35b6100de61012e565b005b6100e861013d565b005b446000819055505b565b456000819055505b565b4173ffffffffffffffffffffffffffffffffffffffff166000819055505b565b436000819055505b565b60005481565b4340600190046000819055505b565b426000819055505b5600a165627a7a72305820a4033d29ccf12dc58fda987b64dec57272d205023628eb3be8ba613585453d770029"
        contract_address = self.nodes[0].createcontract(contract_bytecode)['address']
        self.node.generate(1)

        print("State change based on block.blockhash")
        block_count = self.node.getblockcount()
        self.node.sendtocontract(contract_address, "73c9330d", 0, 1000000)
        self.node.generate(1)
        assert_equal(self.node.getblockcount(), block_count+1)

        print("State change based on block.coinbase")
        block_count = self.node.getblockcount()
        self.node.sendtocontract(contract_address, "5facaf8e", 0, 1000000)
        self.node.generate(1)
        assert_equal(self.node.getblockcount(), block_count+1)

        print("State change based on block.difficulty")
        block_count = self.node.getblockcount()
        self.node.sendtocontract(contract_address, "009e8657", 0, 1000000)
        self.node.generate(1)
        assert_equal(self.node.getblockcount(), block_count+1)

        print("State change based on block.gaslimit")
        block_count = self.node.getblockcount()
        self.node.sendtocontract(contract_address, "49b573d4", 0, 1000000)
        self.node.generate(1)
        assert_equal(self.node.getblockcount(), block_count+1)

        print("State change based on block.number")
        block_count = self.node.getblockcount()
        self.node.sendtocontract(contract_address, "6bde2d73", 0, 1000000)
        self.node.generate(1)
        assert_equal(self.node.getblockcount(), block_count+1)

        print("State change based on block.timestamp")
        block_count = self.node.getblockcount()
        self.node.sendtocontract(contract_address, "c1a5257b", 0, 1000000)
        self.node.generate(1)
        assert_equal(self.node.getblockcount(), block_count+1)

if __name__ == '__main__':
    QtumGlobalsStateChangerTest().main()
