#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.qtum import *
import sys


class OpCallTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [['-txindex=1']]*2)
        self.node = self.nodes[0]
        self.is_network_split = False
        connect_nodes(self.nodes[0], 1)

    def send_one_op_call_tx_with_counter_check(self, outputs, counter_should_increase_by=0, input_value=500000000, should_throw=False):
        # 61bc221a counter()
        old_out = int(self.node.callcontract(self.contract_address, "61bc221a")['executionResult']['output'], 16)

        tx = make_transaction(self.node, [make_vin(self.node, input_value)], outputs)

        if should_throw:
            try:
                self.node.sendrawtransaction(tx)
                assert(False)
            except JSONRPCException as e:
                print(e)
                pass
        else:
            self.node.sendrawtransaction(tx)

        self.node.generate(1)
        sync_blocks(self.nodes)
        for i in range(2):
            # 61bc221a counter()
            out = int(self.nodes[i].callcontract(self.contract_address, "61bc221a")['executionResult']['output'], 16)
            assert(out-old_out == counter_should_increase_by)

    def send_multiple_op_call_txs_with_counter_check(self, num_txs, outputs, counter_should_increase_by):
        # 61bc221a counter()
        old_out = int(self.node.callcontract(self.contract_address, "61bc221a")['executionResult']['output'], 16)
        i = 0
        unspents = self.node.listunspent()
        while i < num_txs and len(unspents) > 0:
            # Select as input a tx which has at least 5 qtum spendable
            for tx_i in range(len(unspents)):
                if unspents[tx_i]['amount'] > 5.00000000 and unspents[tx_i]['spendable']:
                    break
            
            inpt = CTxIn(COutPoint(int(unspents[tx_i]['txid'], 16), unspents[tx_i]['vout']), nSequence=0)
            tx = make_transaction(self.node, [inpt], outputs)
            self.node.sendrawtransaction(tx)
            unspents = self.node.listunspent()
            i += 1
        
        self.node.generate(1)
        sync_blocks(self.nodes)
        for i in range(2):
            # 61bc221a counter()
            out = int(self.nodes[i].callcontract(self.contract_address, "61bc221a")['executionResult']['output'], 16)
            assert(out-old_out == counter_should_increase_by)

    # Deploy the testing contract
    def create_contract_test(self):
        """
        pragma solidity ^0.4.10;

        contract Example {
            uint public counter;
            
            function inc() public {
                counter += 1;
            }

            function getBalance() public {
                return this.balance;
            }
        }
        """
        contract_data = self.node.createcontract("6060604052341561000c57fe5b5b61011e8061001c6000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806312065fe0146058578063371303c014607b57806361bc221a14608a578063d0e30db01460ad575bfe5b3415605f57fe5b606560b5565b6040518082815260200191505060405180910390f35b3415608257fe5b608860d5565b005b3415609157fe5b609760e9565b6040518082815260200191505060405180910390f35b60b360ef565b005b60003073ffffffffffffffffffffffffffffffffffffffff163190505b90565b60016000600082825401925050819055505b565b60005481565b5b5600a165627a7a72305820fe93d8cc66557a2a6c8347f481f6d334402a7f90f8b2288668a874c34416a4dc0029", 1000000, QTUM_MIN_GAS_PRICE / COIN)
        self.contract_address = contract_data['address']
        block_height = self.node.getblockcount()
        self.node.generate(1)
        sync_blocks(self.nodes)
        for i in range(2):
            assert(self.nodes[i].getblockcount() == block_height+1)
            assert(len(self.nodes[i].listcontracts()) == 1+NUM_DEFAULT_DGP_CONTRACTS)

    # Sends a tx containing 2 op_call outputs calling inc()
    def many_calls_in_same_tx_test(self):
        outputs = []
        outputs.append(make_op_call_output(0, b"\x04", CScriptNum(1000000), CScriptNum(QTUM_MIN_GAS_PRICE), bytes.fromhex("371303c0"), bytes.fromhex(self.contract_address)))
        outputs.append(make_op_call_output(0, b"\x04", CScriptNum(1000000), CScriptNum(QTUM_MIN_GAS_PRICE), bytes.fromhex("371303c0"), bytes.fromhex(self.contract_address)))
        self.send_one_op_call_tx_with_counter_check(outputs, counter_should_increase_by=2)

    # Sends a normal raw op_call tx with a single output.
    def normal_op_call_output_test(self):
        outputs = []
        outputs.append(make_op_call_output(0, b"\x04", b"\xff\x7f", CScriptNum(QTUM_MIN_GAS_PRICE), bytes.fromhex("371303c0"), bytes.fromhex(self.contract_address)))
        self.send_one_op_call_tx_with_counter_check(outputs, counter_should_increase_by=1)

    # Sends a tx containing 1 op_call output where txfee == gas_price*gas_limit.
    def gas_equal_to_tx_fee_test(self):
        outputs = []
        outputs.append(make_op_call_output(0, b"\x04", CScriptNum(1000000), CScriptNum(QTUM_MIN_GAS_PRICE), bytes.fromhex("371303c0"), bytes.fromhex(self.contract_address)))
        self.send_one_op_call_tx_with_counter_check(outputs, counter_should_increase_by=1, input_value=1000000*QTUM_MIN_GAS_PRICE)

    # Sends a tx containing 1 op_call output where txfee < gas_price*gas_limit.
    def gas_exceeding_tx_fee_100001_1_test(self):
        outputs = []
        outputs.append(make_op_call_output(0, b"\x04", CScriptNum(10000001), CScriptNum(QTUM_MIN_GAS_PRICE), bytes.fromhex("371303c0"), bytes.fromhex(self.contract_address)))
        self.send_one_op_call_tx_with_counter_check(outputs, input_value=1000001*QTUM_MIN_GAS_PRICE-1, should_throw=True)

    # Sends a tx containing 1 op_call output where txfee < gas_price*gas_limit.
    def gas_exceeding_tx_fee_100001_2_test(self):
        outputs = []
        outputs.append(make_op_call_output(0, b"\x04", CScriptNum(1000001), CScriptNum(QTUM_MIN_GAS_PRICE), bytes.fromhex("371303c0"), bytes.fromhex(self.contract_address)))
        self.send_one_op_call_tx_with_counter_check(outputs, input_value=1000000*QTUM_MIN_GAS_PRICE, should_throw=True)

    # Sends a tx containing 2 op_call outputs that has a combined gas_price*gas_limit exceeding the tx fee.
    # This tx should be rejected since executing such a tx would be unable to pay for its potential execution costs in the same way as a tx with one output where txfee < gas_price*gas_limit.
    def two_calls_in_same_tx_exceeding_tx_fee_test(self):
        outputs = []
        outputs.append(make_op_call_output(0, b"\x04", CScriptNum(1000000), CScriptNum(QTUM_MIN_GAS_PRICE), bytes.fromhex("371303c0"), bytes.fromhex(self.contract_address)))
        outputs.append(make_op_call_output(0, b"\x04", CScriptNum(1000000), CScriptNum(QTUM_MIN_GAS_PRICE), bytes.fromhex("371303c0"), bytes.fromhex(self.contract_address)))
        self.send_one_op_call_tx_with_counter_check(outputs, input_value=2000000*QTUM_MIN_GAS_PRICE-1, should_throw=True)

    # sends a tx containing 1 op_call output with a (if interpreted with a signed integer) negative gas limit calling inc()
    def gas_limit_signedness_test(self):
        outputs = []
        gas_limit = b"\xff"
        while len(gas_limit) < 20:
            outputs.append(make_op_call_output(0, b"\x04", gas_limit, CScriptNum(QTUM_MIN_GAS_PRICE), bytes.fromhex("371303c0"), bytes.fromhex(self.contract_address)))
            self.send_one_op_call_tx_with_counter_check(outputs, should_throw=True)
            gas_limit += b"\xff"

    # sends a tx containing 1 op_call output with a (if interpreted with a signed integer) negative gas limit calling inc()
    def gas_limit_signedness_one_valid_test(self):
        outputs = []
        gas_limit = b"\xff"
        outputs.append(make_op_call_output(0, b"\x04", b"\xff\xff\x00", CScriptNum(QTUM_MIN_GAS_PRICE), bytes.fromhex("371303c0"), bytes.fromhex(self.contract_address)))
        outputs.append(make_op_call_output(0, b"\x04", b"\xff\xff", CScriptNum(QTUM_MIN_GAS_PRICE), bytes.fromhex("371303c0"), bytes.fromhex(self.contract_address)))
        self.send_one_op_call_tx_with_counter_check(outputs, should_throw=True)

    # sends a tx containing 1 op_call output with a (if interpreted with a signed integer) negative gas price calling inc()
    def gas_price_signedness_test(self):
        outputs = []
        outputs.append(make_op_call_output(0, b"\x04", b"\x01\x00", b"\xff\xff", bytes.fromhex("371303c0"), bytes.fromhex(self.contract_address)))
        self.send_one_op_call_tx_with_counter_check(outputs, should_throw=True)

    # sends a tx containing 1 op_call output with a possible negative gas limit and price calling inc()
    def gas_limit_and_price_signedness_test(self):
        outputs = []
        outputs.append(make_op_call_output(0, b"\x04", b"\xff\xff", b"\xff", bytes.fromhex("371303c0"), bytes.fromhex(self.contract_address)))
        self.send_one_op_call_tx_with_counter_check(outputs, should_throw=True)

    # Sends 100 valid op_call txs
    def send_100_txs_test(self):
        outputs = []
        outputs.append(make_op_call_output(0, b"\x04", CScriptNum(1000000), CScriptNum(QTUM_MIN_GAS_PRICE), bytes.fromhex("371303c0"), bytes.fromhex(self.contract_address)))
        self.send_multiple_op_call_txs_with_counter_check(100, outputs, 100)

    def send_tx_with_value_test(self):
        outputs = []
        # d0e30db0 deposit()
        outputs.append(make_op_call_output(100000000, b"\x04", CScriptNum(1000000), CScriptNum(QTUM_MIN_GAS_PRICE), bytes.fromhex("d0e30db0"), bytes.fromhex(self.contract_address)))
        self.send_one_op_call_tx_with_counter_check(outputs, counter_should_increase_by=0)
        
        # 12065fe0 getBalance()
        balance = int(self.node.callcontract(self.contract_address, "12065fe0")['executionResult']['output'], 16)
        assert(balance == 100000000)




    def run_test(self):
        self.nodes[0].generate(100+COINBASE_MATURITY)
        print("Creating contract")
        self.create_contract_test()
        print("Calling inc() in two outputs")
        self.many_calls_in_same_tx_test()
        print("Calling inc() in one output")
        self.normal_op_call_output_test()
        print("Calling inc() in one output with txfee equal to gas_limit*gas_price")
        self.gas_equal_to_tx_fee_test()
        print("Calling inc() in one output with txfee < gas_limit*gas_price")
        self.gas_exceeding_tx_fee_100001_1_test()
        print("Second test of inc() in one outputs with txfee < gas_limit*gas_price")
        self.gas_exceeding_tx_fee_100001_2_test()
        print("Second test of inc() in one output with txfee < gas_limit*gas_price")
        self.two_calls_in_same_tx_exceeding_tx_fee_test()
        print("Mining a block with 100 txs each with an output calling inc()")
        self.send_100_txs_test()
        print("Checking that the value of txs are correctly updated")
        self.send_tx_with_value_test()
        print("Checking gas limit signedness where one tx is valid")
        self.gas_limit_signedness_one_valid_test()
        print("Checking gas limit signedness")
        self.gas_limit_signedness_test()
        print("Checking gas price signedness")
        self.gas_price_signedness_test()
        print("Checking gas limit and gas price signedness")
        self.gas_limit_and_price_signedness_test()


if __name__ == '__main__':
    OpCallTest().main()