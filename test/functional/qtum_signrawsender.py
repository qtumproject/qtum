#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.blocktools import *
from test_framework.p2p import *
from test_framework.address import *
from test_framework.qtum import *

class QtumSignRawSenderTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [['-txindex', '-minmempoolgaslimit=10000']]*2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        coinbase_txes = []
        block_hashes = generatesynchronized(self.nodes[0], 100+COINBASE_MATURITY, None, self.nodes)
        for block_hash in block_hashes[:100]:
            coinbase_txid = self.nodes[0].getblock(block_hash)['tx'][0]
            coinbase_txes.append(coinbase_txid)

        self.sync_all()

        # Create a contract with a sender address that we CANT sign with
        tx = self.nodes[0].createrawtransaction([{"txid": coinbase_txes.pop(), "vout": 0}], [{"contract": {"bytecode": "00", "senderAddress": "qWNXxpNL6GwNCAFCC9RsmUXuXg4ogFX6ay"}}])
        res = self.nodes[0].signrawsendertransactionwithwallet(tx)
        assert(not res['complete'])
        tx = res['hex']
        assert_raises_rpc_error(-8, "Missing contract sender signature,use signrawsendertransactionwithwallet or signrawsendertransactionwithkey to sign the outputs", self.nodes[0].signrawtransactionwithwallet, tx)

        # Create a contract with a sender address that we CAN sign with
        tx = self.nodes[0].createrawtransaction([{"txid": coinbase_txes.pop(), "vout": 0}], [{"contract": {"bytecode": "00", "senderAddress": self.nodes[0].getnewaddress()}}, {self.nodes[0].getnewaddress(): 19999}])
        res = self.nodes[0].signrawsendertransactionwithwallet(tx)
        assert(res['complete'])
        tx = res['hex']
        res = self.nodes[0].signrawtransactionwithwallet(tx)
        assert(res['complete'])
        tx = res['hex']
        self.nodes[1].sendrawtransaction(tx)
        self.sync_all()
        self.nodes[0].generate(1)
        self.sync_all()

        # Create a contract with a sender address that we CANT sign with and one that we CAN sign
        tx = self.nodes[0].createrawtransaction([{"txid": coinbase_txes.pop(), "vout": 0}], [
            {"contract": {"bytecode": "00", "senderAddress": self.nodes[1].getnewaddress()}}, 
            {"contract": {"bytecode": "00", "senderAddress": self.nodes[0].getnewaddress()}}])
        res = self.nodes[0].signrawsendertransactionwithwallet(tx)
        assert(not res['complete'])
        tx = res['hex']
        assert_raises_rpc_error(-8, "Missing contract sender signature,use signrawsendertransactionwithwallet or signrawsendertransactionwithkey to sign the outputs", self.nodes[0].signrawtransactionwithwallet, tx)

        # Create a contract with two sender addresses that we CAN sign with
        tx = self.nodes[0].createrawtransaction([{"txid": coinbase_txes.pop(), "vout": 0}], [
            {"contract": {"bytecode": "00", "senderAddress": self.nodes[0].getnewaddress()}}, 
            {self.nodes[0].getnewaddress(): 19998}, 
            {"contract": {"bytecode": "6080604052348015600f57600080fd5b50603b80601d6000396000f3fe608060405200fea265627a7a7231582052868d1493e53ba277b4dce2476627b2ce233812344bc32a2e016a978601379764736f6c634300050b0032", "senderAddress": self.nodes[0].getnewaddress()}}])
        
        res = self.nodes[0].signrawsendertransactionwithwallet(tx)
        assert(res['complete'])
        tx = res['hex']
        res = self.nodes[0].signrawtransactionwithwallet(tx)
        assert(res['complete'])
        tx = res['hex']
        self.nodes[1].sendrawtransaction(tx)
        self.sync_all()
        contract_addresses = set(self.nodes[0].listcontracts().keys())
        self.nodes[0].generate(1)
        self.sync_all()

        # Now send some qtum to both addresses (one contract will increase its balance and the other one will throw),
        # sign using signrawsendertransactionwithkey using a node that does not hold the keys in its keystore
        contract_addresses = list(set(self.nodes[0].listcontracts().keys()) - contract_addresses)
        senders = [self.nodes[0].getnewaddress() for i in range(2)]
        tx = self.nodes[0].createrawtransaction([{"txid": coinbase_txes.pop(), "vout": 0}], [
            {"contract": {"data": "00", "contractAddress": contract_addresses[1], "amount": "1", "senderAddress": senders[0], "data": "00"}}, 
            {self.nodes[0].getnewaddress(): 19997}, 
            {"contract": {"data": "00", "contractAddress": contract_addresses[0], "amount": "1", "senderAddress": senders[1], "data": "00"}}])
        res = self.nodes[1].signrawsendertransactionwithkey(tx, [self.nodes[0].dumpprivkey(sender) for sender in senders])
        assert(res['complete'])
        tx = res['hex']
        res = self.nodes[0].signrawtransactionwithwallet(tx)
        assert(res['complete'])
        tx = res['hex']
        self.nodes[1].sendrawtransaction(tx, 0)
        self.sync_all()
        block_hash = self.nodes[0].generate(1)[0]
        self.sync_all()
        payable_contract_addresses = [x[0] for x in self.nodes[0].listcontracts().items() if x[1] == 1]
        assert_equal(len(payable_contract_addresses), 1)
        payable_contract_address = payable_contract_addresses[0]
        assert_equal(self.nodes[0].listcontracts()[payable_contract_address], 1)
        payable_contract_address = payable_contract_addresses[0]


        # Make sure that a tx with a too low gaslimit fails
        tx = self.nodes[0].createrawtransaction([{"txid": coinbase_txes.pop(), "vout": 0}], [
            {self.nodes[0].getnewaddress(): 19998},  
            {"contract": {"data": "00", "contractAddress": payable_contract_address, "amount": "1", "senderAddress": senders[1], "data": "00", "gasLimit": 11000}}])
        res = self.nodes[0].signrawsendertransactionwithwallet(tx)
        assert(res['complete'])
        tx = res['hex']
        res = self.nodes[0].signrawtransactionwithwallet(tx)
        assert(res['complete'])
        tx = res['hex']
        self.nodes[1].sendrawtransaction(tx, 0)
        self.sync_all()
        self.sync_all()
        assert_equal(self.nodes[0].listcontracts()[payable_contract_address], 1)

if __name__ == '__main__':
    QtumSignRawSenderTest().main()
