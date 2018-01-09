#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.address import *
from test_framework.qtum import *
from test_framework.blocktools import *

from test_framework.address import *
import sys
import random
import time
import pdb

class QtumCallContractsTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        self.is_network_split = False
        self.node = self.nodes[0]
        connect_nodes_bi(self.nodes, 0, 1)

    def contract_manipulations(self):
        contract_data = self.node.createcontract("6060604052341561000f57600080fd5b60f48061001d6000396000f300606060405260043610603f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680631a695230146041575b005b606b600480803573ffffffffffffffffffffffffffffffffffffffff16906020019091905050606d565b005b8073ffffffffffffffffffffffffffffffffffffffff166108fc60023073ffffffffffffffffffffffffffffffffffffffff163181151560a957fe5b049081150290604051600060405180830381858888f1935050505050505600a165627a7a723058202876952ea503d0ef5b70800643c4ac7c8c420796cd3738c6e6351fa1878d3cf70029")
        contract_address = contract_data['address']
        hash1 = contract_data['hash160']
        sender1 = contract_data['sender']
        self.node.generate(1)
        
        sender2 = self.node.createcontract("6060604052341561000f57600080fd5b60f48061001d6000396000f300606060405260043610603f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680631a695230146041575b005b606b600480803573ffffffffffffffffffffffffffffffffffffffff16906020019091905050606d565b005b8073ffffffffffffffffffffffffffffffffffffffff166108fc60023073ffffffffffffffffffffffffffffffffffffffff163181151560a957fe5b049081150290604051600060405180830381858888f1935050505050505600a165627a7a723058202876952ea503d0ef5b70800643c4ac7c8c420796cd3738c6e6351fa1878d3cf70029")['sender']        
        self.node.generate(1)
        
        self.node.sendtoaddress(sender1, 50000)
        self.node.sendtoaddress(sender2, 50000)
       
        for i in range(10):
            self.node.sendtocontract(contract_address, "1a695230"+hash1.zfill(64), 13, 100000, 0.000001, sender2)
            self.node.generate(1)
            
            self.node.sendtocontract(contract_address, "1a695230"+hash1.zfill(64), 13, 10000, 0.000001, sender1)
            self.node.generate(1)

            self.node.sendtocontract(contract_address, "1a6952300000000000000000000000001111111111111111111111111111111111111111", 13, 100000, 0.000001, sender2)
            self.node.generate(3)

        time.sleep(5)
                
    def run_test(self):
        self.node.generate(10)
        self.nodes[1].generate(510)        
        self.sync_all()
        
        self.contract_manipulations()
        print(self.node.getblockcount(),self.nodes[1].getblockcount())
        assert_equal(self.node.getblockcount(),self.nodes[1].getblockcount())
        

if __name__ == '__main__':
    QtumCallContractsTest().main()
