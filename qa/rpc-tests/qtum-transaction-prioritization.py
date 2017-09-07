#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.address import *
from test_framework.qtum import *
import sys
import random
import time

class QtumTransactionPrioritizationTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [['-staking=1', '-rpcmaxgasprice=10000000']])
        self.is_network_split = False
        self.node = self.nodes[0]

    def restart_node(self):
        stop_nodes(self.nodes)
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [['-staking=1', '-rpcmaxgasprice=10000000']])
        self.node = self.nodes[0]

    def stake_or_mine(self, old_block_count=None, use_staking=False):
        # Since staking is switched on by default, if a block has been staked return that block's hash
        if self.node.getblockcount() > old_block_count:
            return self.node.getbestblockhash()

        if use_staking:
            if not old_block_count:
                old_block_count = self.node.getblockcount()
            while old_block_count == self.node.getblockcount():
                time.sleep(0.1)
            return self.node.getbestblockhash()
        else:
            return self.node.generate(1)[0]

    def send_transaction_with_fee(self, fee):
        for unspent in self.node.listunspent():
            if unspent['amount'] >= 10000:
                break
        addr = self.node.getnewaddress()
        haddr = p2pkh_to_hex_hash(addr)
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(unspent['txid'], 16), unspent['vout']), nSequence=0)]
        amount = int((float(str(unspent['amount'])) - fee)*COIN)
        tx.vout = [CTxOut(amount, scriptPubKey=CScript([OP_DUP, OP_HASH160, hex_str_to_bytes(haddr), OP_EQUALVERIFY, OP_CHECKSIG]))]
        tx_hex_signed = self.node.signrawtransaction(bytes_to_hex_str(tx.serialize()))['hex']
        return self.node.sendrawtransaction(tx_hex_signed)

    # Creates and op_call tx that calls the fallback function of the only contract that should be in existance
    def send_op_call_transaction_with_gas_price(self, contract_address, gas_price, spends_txid=None, spends_vout=None):
        gas_limit = 1000000
        if not spends_txid:
            unspent = self.node.listunspent()[0]
            spends_txid = unspent['txid']
            spends_vout = unspent['vout']

        # Fetch the amount of the vout of the txid that we are spending
        spends_tx = self.node.getrawtransaction(spends_txid, True)
        for output in spends_tx['vout']:
            if output['n'] == spends_vout:
                break
        else:
            # That output does not exist...
            assert(False)

        addr = self.node.getnewaddress()
        haddr = p2pkh_to_hex_hash(addr)
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(spends_txid, 16), spends_vout), nSequence=0)]
        tx.vout.append(CTxOut(0, scriptPubKey=CScript([b"\x04", CScriptNum(gas_limit), CScriptNum(int(gas_price*COIN)), b"\x00", hex_str_to_bytes(contract_address), OP_CALL])))
        change = int((float(str(output['value'])) - gas_price*gas_limit) * COIN)
        tx.vout.append(CTxOut(change, scriptPubKey=CScript([OP_DUP, OP_HASH160, hex_str_to_bytes(haddr), OP_EQUALVERIFY, OP_CHECKSIG])))
        tx_hex_signed = self.node.signrawtransaction(bytes_to_hex_str(tx.serialize()))['hex']
        return self.node.sendrawtransaction(tx_hex_signed)

    def send_op_call_outputs_with_gas_price(self, contract_address, gas_prices, spends_txid=None, spends_vout=None):
        gas_limit = 100000
        if not spends_txid:
            for unspent in self.node.listunspent():
                if unspent['amount'] == 20000:
                    spends_txid = unspent['txid']
                    spends_vout = unspent['vout']
                    break

        # Fetch the amount of the vout of the txid that we are spending
        spends_tx = self.node.getrawtransaction(spends_txid, True)
        for output in spends_tx['vout']:
            if output['n'] == spends_vout:
                break
        else:
            # That output does not exist...
            assert(False)

        addr = self.node.getnewaddress()
        haddr = p2pkh_to_hex_hash(addr)
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(spends_txid, 16), spends_vout), nSequence=0)]
        for gas_price in gas_prices:
            tx.vout.append(CTxOut(0, scriptPubKey=CScript([b"\x04", CScriptNum(gas_limit), CScriptNum(int(gas_price*COIN)), b"\x00", hex_str_to_bytes(contract_address), OP_CALL])))
        change = int((float(str(output['value'])) - sum(gas_prices)*gas_limit) * COIN)
        tx.vout.append(CTxOut(change, scriptPubKey=CScript([OP_DUP, OP_HASH160, hex_str_to_bytes(haddr), OP_EQUALVERIFY, OP_CHECKSIG])))
        tx_hex_signed = self.node.signrawtransaction(bytes_to_hex_str(tx.serialize()))['hex']
        return self.node.sendrawtransaction(tx_hex_signed)

    def verify_contract_txs_are_added_last_test(self, with_restart=False, use_staking=False):
        # Set the fee really high so that it should normally be added first if we only looked at the fee/size
        contract_txid = self.node.createcontract("00", 4*10**6, 0.0001)['txid']
        normal_txid = self.node.sendtoaddress(self.node.getnewaddress(), 1)

        old_block_count = self.node.getblockcount()
        if with_restart:
            self.restart_node()
        block_hash = self.stake_or_mine(old_block_count=old_block_count, use_staking=use_staking)

        block_txs = self.node.getblock(block_hash)['tx']
        if use_staking:
            block_txs.pop(1) # Ignore the coinstake tx so we can reuse the tests for both pow and pos
        assert_equal(len(block_txs), 3)
        assert_equal(block_txs.index(normal_txid), 1)
        assert_equal(block_txs.index(contract_txid), 2)

    # Verifies that contract transactions are correctly ordered by descending (minimum among outputs) gas price and ascending size
    # Sends 7 txs in total
    def verify_contract_txs_internal_order_test(self, with_restart=False, use_staking=False):
        contract_address = list(self.node.listcontracts().keys())[0]
        sender = self.node.getnewaddress()
        tx4 = self.send_op_call_outputs_with_gas_price(contract_address, [0.0001])
        tx5 = self.send_op_call_outputs_with_gas_price(contract_address, [0.0001, 0.0001])
        tx3 = self.send_op_call_outputs_with_gas_price(contract_address, [0.00010001])
        tx6 = self.send_op_call_outputs_with_gas_price(contract_address, [0.0001, 0.00010001, 0.00010001])
        tx2 = self.send_op_call_outputs_with_gas_price(contract_address, [0.002])
        tx1 = self.node.sendtoaddress(sender, 1)
        tx7 = self.node.sendtocontract(contract_address, "00", 0, 100000, 0.000001, sender)['txid']
        old_block_count = self.node.getblockcount()
        if with_restart:
            self.restart_node()
        # Ordering based on gas_price should now be
        block_hash = self.stake_or_mine(old_block_count=old_block_count, use_staking=use_staking)
        block = self.node.getblock(block_hash)
        block_txs = block['tx']
        if use_staking:
            block_txs.pop(1) # Ignore the coinstake tx so we can reuse the tests for both pow and pos

        assert_equal(block_txs[1:], [tx1, tx2, tx3, tx4, tx5, tx6, tx7])



    # In the case of an ancestor chain in the mempool such that a contract tx spends another normal tx that is in the mempool
    # the contract tx should still be added last while the tx it spends should be added based on it's fee ordering.
    # In this test we create 4 txs.
    # 1. a normal tx has a fee > tx2 and tx3
    # 2. a ancestor normal tx that will be spent by the contract tx has a fee < tx1 and tx3 >
    # 3. a normal tx with a fee < tx2 and tx3
    # 4. a op call contract tx spending tx2.
    # Expected transaction ordering in the block should thus be tx1, tx2, tx3, tx4
    def verify_ancestor_chain_with_contract_txs_test(self, with_restart=False, use_staking=False):
        contract_address = list(self.node.listcontracts().keys())[0]
        tx1 = self.send_transaction_with_fee(0.01)
        tx2 = self.send_transaction_with_fee(0.005)
        tx3 = self.send_transaction_with_fee(0.001)

        # Create a contract tx (4) that spends tx3
        tx4 = self.send_op_call_transaction_with_gas_price(contract_address, 0.001, spends_txid=tx2, spends_vout=0)
        # Make sure that all txs are in the mempool
        assert_equal(len(self.node.getrawmempool()), 4)

        old_block_count = self.node.getblockcount()
        if with_restart:
            self.restart_node()
        block_hash = self.stake_or_mine(old_block_count=old_block_count, use_staking=use_staking)

        block_txs = self.node.getblock(block_hash)['tx']

        if use_staking:
            block_txs.pop(1) # Ignore the coinstake tx so we can reuse the tests for both pow and pos

        assert_equal(len(block_txs), 5)
        assert_equal(block_txs[1], tx1)
        assert_equal(block_txs[2], tx2)
        assert_equal(block_txs[3], tx3)
        assert_equal(block_txs[4], tx4)

    # Creates two different contract tx chains.
    def verify_contract_ancestor_txs_test(self, with_restart=False, use_staking=False):
        contract_address = list(self.node.listcontracts().keys())[0]
        for unspent in self.node.listunspent():
            if unspent['amount'] > 10000:
                break
        address = self.node.getnewaddress()
        expected_tx_order = []

        for (expected_tx_index, gas_price) in [(1, 100900), (2, 800), (7, 100), (8, 9900)]:
            tx = CTransaction()
            tx.vin = [CTxIn(COutPoint(int(unspent['txid'], 16), unspent['vout']), nSequence=0)]
            tx.vout = [
                CTxOut(0, scriptPubKey=CScript([b"\x04", CScriptNum(30000), CScriptNum(gas_price), b"\x00", hex_str_to_bytes(contract_address), OP_CALL])),
                CTxOut(int((unspent['amount'] - 100)*COIN), scriptPubKey=CScript([OP_DUP, OP_HASH160, hex_str_to_bytes(p2pkh_to_hex_hash(address)), OP_EQUALVERIFY, OP_CHECKSIG]))
                ]
            tx_raw = self.node.signrawtransaction(bytes_to_hex_str(tx.serialize()))['hex']

            # Make the next vin refer to this tx.
            unspent['amount'] -= 101
            unspent['txid'] = self.node.sendrawtransaction(tx_raw)
            unspent['vout'] = 1
            expected_tx_order.append((expected_tx_index, unspent['txid']))

        for unspent in self.node.listunspent():
            if unspent['amount'] == 20000 and unspent['address'] != address:
                break

        # The list of tuples specifies (expected position in block txs, gas_price)
        for (expected_tx_index, gas_price) in [(3, 600), (4, 300), (5, 200), (6, 9800)]:
            tx = CTransaction()
            tx.vin = [CTxIn(COutPoint(int(unspent['txid'], 16), unspent['vout']), nSequence=0)]
            tx.vout = [
                CTxOut(0, scriptPubKey=CScript([b"\x04", CScriptNum(30000), CScriptNum(gas_price), b"\x00", hex_str_to_bytes(contract_address), OP_CALL])),
                CTxOut(int((unspent['amount'] - 100)*COIN), scriptPubKey=CScript([OP_DUP, OP_HASH160, hex_str_to_bytes(p2pkh_to_hex_hash(address)), OP_EQUALVERIFY, OP_CHECKSIG]))
                ]
            tx_raw = self.node.signrawtransaction(bytes_to_hex_str(tx.serialize()))['hex']

            # Make the next vin refer to this tx.
            unspent['amount'] -= 101
            unspent['txid'] = self.node.sendrawtransaction(tx_raw)
            unspent['vout'] = 1
            expected_tx_order.append((expected_tx_index, unspent['txid']))

        old_block_count = self.node.getblockcount()
        if with_restart:
            self.restart_node()
        block_hash = self.stake_or_mine(old_block_count=old_block_count, use_staking=use_staking)
        block_txs = self.node.getblock(block_hash)['tx']
        
        if use_staking:
            block_txs.pop(1) # Ignore the coinstake tx so we can reuse the tests for both pow and pos

        # Even though the gas prices differ, since they the ancestor txs must be included before the child txs we expect the order by which they were sent,
        # always chosing the tx with the highest gas price whose ancestors have already been included. 
        for (expected_tx_index, txid) in expected_tx_order:
            assert_equal(block_txs[expected_tx_index], txid)

    def run_test(self):
        self.node.generate(500+COINBASE_MATURITY)
        print("running pow tests")
        self.verify_contract_txs_are_added_last_test()
        self.verify_ancestor_chain_with_contract_txs_test()
        self.verify_contract_txs_internal_order_test()
        self.verify_contract_ancestor_txs_test()

        # Verify that the mempool is empty before running more tests
        assert_equal(self.node.getrawmempool(), [])

        # Redo the testing and check that the mempool is correctly ordered after a restart
        print("running pow tests with restart")
        self.verify_contract_txs_are_added_last_test(with_restart=True)
        self.verify_ancestor_chain_with_contract_txs_test(with_restart=True)
        self.verify_contract_txs_internal_order_test(with_restart=True)
        self.verify_contract_ancestor_txs_test(with_restart=True)

        # Verify that the mempool is empty before running more tests
        assert_equal(self.node.getrawmempool(), [])

        print("running pos tests")
        self.verify_contract_txs_are_added_last_test(use_staking=True)
        self.verify_ancestor_chain_with_contract_txs_test(use_staking=True)
        self.verify_contract_txs_internal_order_test(use_staking=True)
        self.verify_contract_ancestor_txs_test(use_staking=True)

        # Verify that the mempool is empty before running more tests
        assert_equal(self.node.getrawmempool(), [])

        print("running pos tests with restart")
        self.verify_contract_txs_are_added_last_test(with_restart=True, use_staking=True)
        self.verify_ancestor_chain_with_contract_txs_test(with_restart=True, use_staking=True)
        self.verify_contract_txs_internal_order_test(with_restart=True, use_staking=True)
        self.verify_contract_ancestor_txs_test(with_restart=True, use_staking=True)

if __name__ == '__main__':
    QtumTransactionPrioritizationTest().main()
