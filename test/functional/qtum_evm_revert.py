#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.qtum import *
from test_framework.qtumconfig import *
import sys
import io
import pprint

pp = pprint.PrettyPrinter()

class QtumEVMRevertTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-logevents', '-minmempoolgaslimit=21000', '-constantinopleheight=%d' % (204 + COINBASE_MATURITY), '-muirglacierheight=100000', '-londonheight=100000']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def assert_revert_state(self, node, gas, gas_price, gas_used, value, excepted):
        blockhash = node.getbestblockhash()
        block = node.getblock(blockhash)
        txids = block['tx']
        coinstake_tx = node.getrawtransaction(txids[0], True, blockhash)
        call_tx = node.getrawtransaction(txids[1], True, blockhash)
        input_tx = node.decoderawtransaction(node.gettransaction(call_tx['vin'][0]['txid'])['hex'])
        sender_utxo = input_tx['vout'][call_tx['vin'][0]['vout']]
        sender_address = sender_utxo['scriptPubKey']['address']

        for op_call_vout_index in range(len(call_tx['vout'])):
            if call_tx['vout'][op_call_vout_index]['scriptPubKey']['type'] == 'call' or call_tx['vout'][op_call_vout_index]['scriptPubKey']['type'] == 'call_sender':
                break



        # Check that the transaction receipt is correct
        receipt = node.gettransactionreceipt(call_tx['txid'])[0]
        assert_equal(receipt['gasUsed'], gas_used)
        assert_equal(receipt['cumulativeGasUsed'], gas_used)
        assert_equal(receipt['blockNumber'], block['height'])
        assert_equal(receipt['blockHash'], block['hash'])
        assert_equal(receipt['excepted'], excepted)
        assert_equal(receipt['exceptedMessage'], '')
        assert_equal(receipt['from'], p2pkh_to_hex_hash(sender_address))
        assert_equal(receipt['transactionIndex'], 1)
        assert_equal(receipt['transactionHash'], call_tx['txid'])
        assert_equal(receipt['log'], [])

        # If there is supposed to be a value refund tx, check that it:
        #  - exists
        #  - has the correct value
        #  - has the correct input
        #  - has the correct output
        if value > 0:
            refund_tx = node.getrawtransaction(txids[-1], True, blockhash)
            refund_utxo = refund_tx['vout'][0]
            assert_equal(len(refund_tx['vin']), 1)
            assert_equal(refund_tx['vin'][0]['txid'], call_tx['txid'])
            assert_equal(refund_tx['vin'][0]['vout'], op_call_vout_index)
            assert_equal(refund_utxo['value'], value)
            assert_equal(sender_utxo['scriptPubKey']['asm'], refund_utxo['scriptPubKey']['asm'])
        else:
            assert_equal(len(txids), 2)

        # Check that the coinstake contains a gas refund (if one should exist)
        if gas > gas_used:
            gas_refund_output = coinstake_tx['vout'][-2]
            assert_equal((gas_refund_output['value']*100000000)//10000000, ((gas-gas_used)*gas_price*100000000)//10000000)
            assert_equal(sender_utxo['scriptPubKey']['asm'], gas_refund_output['scriptPubKey']['asm'])
        else:
            assert_equal(len(coinstake_tx['vout']), 2)


        # Check that the state and utxo root remaing the same
        prevblock = node.getblock(block['previousblockhash'])
        assert_equal(block['hashStateRoot'], prevblock['hashStateRoot'])
        assert_equal(block['hashUTXORoot'], prevblock['hashUTXORoot'])

    def run_test(self):
        # Dummy address to generate blocks to
        dummy_key = ECKey()
        dummy_key.generate()
        dummy_address = hex_hash_to_p2pkh("12"*20)

        self.nodes[0].generatetoaddress(200, self.nodes[0].getnewaddress())
        self.nodes[0].generatetoaddress(COINBASE_MATURITY, dummy_address)
        """
            pragma solidity ^0.5.10;

            contract Test{
                event TestTest();
                function () payable external {
                    emit TestTest();
                    revert();
                    emit TestTest(); // check if revert() can prevent execution from continuing
                }
            }
        """
        bytecode = "6080604052348015600f57600080fd5b50606a80601d6000396000f3fe60806040527f902ab12fc657922f9e7e1085a23c967a546ad6f8a771c0b5c7db57f7aac0076e60405160405180910390a1600080fdfea265627a7a72305820fea288c1e5cdb52afd4c9f5be9081c6199bc47960599fcc1655dd7a900e39cdc64736f6c634300050a0032"
        contract_address = self.nodes[0].createcontract(bytecode)['address']
        self.nodes[0].generatetoaddress(1, dummy_address)

        # Run a normal revert tx, will cause a throw since REVERT is undefined before qip7/constantinople
        txid = self.nodes[0].sendtocontract(contract_address, "00", 10, 100000, 0.000001)['txid']
        self.nodes[0].generatetoaddress(1, dummy_address)
        self.assert_revert_state(self.nodes[0], gas=100000, gas_price=0.000001, gas_used=100000, value=10, excepted='BadInstruction')
        
        self.nodes[0].generatetoaddress(10, dummy_address)

        # Run a normal revert tx
        txid = self.nodes[0].sendtocontract(contract_address, "00", 10, 100000, 0.000001)['txid']
        self.nodes[0].generatetoaddress(1, dummy_address)
        self.assert_revert_state(self.nodes[0], gas=100000, gas_price=0.000001, gas_used=21805, value=10, excepted='Revert')

        # run a revert tx with too little gas
        txid = self.nodes[0].sendtocontract(contract_address, "00", 10, 21803, 0.000001)['txid']
        self.nodes[0].generatetoaddress(1, dummy_address)
        self.assert_revert_state(self.nodes[0], gas=21803, gas_price=0.000001, gas_used=21803, value=10, excepted='OutOfGas')

        # run a revert tx with too little gas
        txid = self.nodes[0].sendtocontract(contract_address, "00", 10, 21804, 0.000001)['txid']
        self.nodes[0].generatetoaddress(1, dummy_address)
        self.assert_revert_state(self.nodes[0], gas=21804, gas_price=0.000001, gas_used=21804, value=10, excepted='OutOfGas')

        # run a revert tx with just enough gas
        txid = self.nodes[0].sendtocontract(contract_address, "00", 10, 21805, 0.000001)['txid']
        self.nodes[0].generatetoaddress(1, dummy_address)
        self.assert_revert_state(self.nodes[0], gas=21805, gas_price=0.000001, gas_used=21805, value=10, excepted='Revert')

        # run a revert tx with 1 gas more than required
        txid = self.nodes[0].sendtocontract(contract_address, "00", 10, 21806, 0.000001)['txid']
        self.nodes[0].generatetoaddress(1, dummy_address)
        self.assert_revert_state(self.nodes[0], gas=21806, gas_price=0.000001, gas_used=21805, value=10, excepted='Revert')
        

if __name__ == '__main__':
    QtumEVMRevertTest().main()