#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test MempoolBroadcast code
#

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

amounts = [3.00600019, 0.0652724, 0.0652724, 0.0652724, 0.0652724, 0.0652724, 0.0652724, 0.0652468, 0.0652468,
           0.0652468, 0.0652468, 0.0652468, 0.0652468, 0.0652468, 0.0652468, 0.0652468, 0.0652468, 0.0652468,
           0.0652468, 0.0652468, 0.0652468, 0.0652468, 0.0592724, 0.0592468, 0.0592468, 0.0592468, 0.0592468,
           0.0592468, 0.0592468, 0.0592468, 0.0592468, 0.0592468, 0.0592468, 0.0592468, 0.0592468, 0.0592468,
           0.0592468, 0.0592468, 0.0592468, 0.0592468, 0.0592468, 0.0592468, 0.0592468, 0.0592468, 0.0592468,
           0.0592468, 0.0592468, 0.0592468, 0.0592468, 0.0592468, 0.0592468, 0.0592468, 0.00516649]

class MempoolBroadcastTest(BitcoinTestFramework):
        
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self):
        self.nodes = []
        self.is_network_split = False 
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug"]))

    def send_money_to_node(self, address):
        for amount in amounts:
            self.nodes[0].sendtoaddress(address, amount)
            self.nodes[0].generate(1)

    def get_tx_inputs(self):
        tx_inputs = []
        listunspent = self.nodes[1].listunspent()
        tx_inputs=[{"txid": unspent['txid'], "vout": unspent['vout']} for unspent in listunspent]
        return tx_inputs

    def run_test(self):
        print("Mine 501 blocks on Node 0")
        self.nodes[0].generate(501)
        
        print("Connect Node 0 and Node 1")
        connect_nodes(self.nodes[0],1)
        
        addr2 = self.nodes[1].getnewaddress()
        print("Send money to Node 1 from Node 0")
        self.send_money_to_node(addr2)

        tx_inputs = []
        tx_inputs = self.get_tx_inputs()

        addr1 = self.nodes[0].getnewaddress()
        tx_vouts = {
            '{}'.format(addr1): 6.15000009, 
            '{}'.format(addr2): 0.00333239
        }

        rawtx = self.nodes[1].createrawtransaction(tx_inputs, tx_vouts)
        signedtx = self.nodes[1].signrawtransaction(rawtx)
        
        print("Send money to Node 0 from Node 1")
        try:
            self.nodes[1].sendrawtransaction(signedtx['hex'])
        except JSONRPCException as error:
            print("Error", error)
        self.nodes[1].generate(1)
        time.sleep(5)
        
        print("Mempool of second node:", self.nodes[1].getrawmempool())
        print("Mempool of first node:", self.nodes[0].getrawmempool())    
        assert(self.nodes[0].getrawmempool()==self.nodes[1].getrawmempool())

if __name__ == '__main__':
    MempoolBroadcastTest().main()
