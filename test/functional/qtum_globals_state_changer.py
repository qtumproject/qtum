#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.address import *

class QtumGlobalsStateChangerTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.node = self.nodes[0]
        self.nodes[0].generate(10 + COINBASE_MATURITY)

        """
        pragma solidity ^0.8.0;
        
        contract Test {
            uint public stateChanger;
            
            function blkblockhash() public payable {
                stateChanger = uint(blockhash(block.number-1));
            }
        
            function blkcoinbase() public payable {
                stateChanger = uint(uint160(address(block.coinbase)));
            }
        
            function blkdifficulty() public payable {
                stateChanger = block.difficulty;
            }
        
            function blkgaslimit() public payable {
                stateChanger = block.gaslimit;
            }
        
            function blknumber() public payable {
                stateChanger = block.number;
            }
            
            function blktimestamp() public payable {
                stateChanger = block.timestamp;
            }
        
            function blkbasefee() public payable {
                stateChanger = block.basefee;
            }
        
            function blkchainid() public payable {
                stateChanger = block.chainid;
            }
        }
        Function signatures:
        2436c165: blkbasefee()
        73c9330d: blkblockhash()
        12a7e88c: blkchainid()
        5facaf8e: blkcoinbase()
        009e8657: blkdifficulty()
        49b573d4: blkgaslimit()
        6bde2d73: blknumber()
        c1a5257b: blktimestamp()
        6c7804b5: stateChanger()
        """
        contract_bytecode = "608060405234801561001057600080fd5b50610246806100206000396000f3fe6080604052600436106100855760003560e01c80635facaf8e116100595780635facaf8e146100b25780636bde2d73146100bc5780636c7804b5146100c657806373c9330d146100f1578063c1a5257b146100fb57610085565b80629e86571461008a57806312a7e88c146100945780632436c1651461009e57806349b573d4146100a8575b600080fd5b610092610105565b005b61009c61010e565b005b6100a6610117565b005b6100b0610120565b005b6100ba610129565b005b6100c4610148565b005b3480156100d257600080fd5b506100db610151565b6040516100e89190610192565b60405180910390f35b6100f9610157565b005b610103610170565b005b44600081905550565b46600081905550565b48600081905550565b45600081905550565b4173ffffffffffffffffffffffffffffffffffffffff16600081905550565b43600081905550565b60005481565b60014361016491906101dc565b4060001c600081905550565b42600081905550565b6000819050919050565b61018c81610179565b82525050565b60006020820190506101a76000830184610183565b92915050565b7f4e487b7100000000000000000000000000000000000000000000000000000000600052601160045260246000fd5b60006101e782610179565b91506101f283610179565b925082821015610205576102046101ad565b5b82820390509291505056fea264697066735822122050fc4f59959598b20e40c132b87c4c613b711382aeb5aa25db716706ba7010b264736f6c634300080d0033"
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
        
        print("State change based on block.basefee")
        block_count = self.node.getblockcount()
        self.node.sendtocontract(contract_address, "2436c165", 0, 1000000)
        self.node.generate(1)
        assert_equal(self.node.getblockcount(), block_count+1)
        
        print("State change based on block.chainid")
        block_count = self.node.getblockcount()
        self.node.sendtocontract(contract_address, "12a7e88c", 0, 1000000)
        self.node.generate(1)
        assert_equal(self.node.getblockcount(), block_count+1)

if __name__ == '__main__':
    QtumGlobalsStateChangerTest().main()
