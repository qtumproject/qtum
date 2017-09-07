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


class CondensingTxsTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [['-txindex=1', '-rpcmaxgasprice=10000000']])
        self.node = self.nodes[0]
        self.is_network_split = False

    # verify that the state hash is not 0 on genesis
    def setup_contracts(self):
        """
        pragma solidity ^0.4.0;
        contract Sender1 { 
            // Sender2 sender2;
            // Sender3 sender3;
            address public sender2;
            address public sender3;
            
            function Sender1() {
            }
            function setSenders(address senderx, address sendery) public{
                // sender2=Sender2(senderx);
                // sender3=Sender3(sendery);
                sender2 = senderx;
                sender3 = sendery;
            }
            function share() public payable{
                if(msg.sender != address(sender3)){
                    // sender2.share.value(msg.value/2);
                    sender2.call.value(msg.value/2)(bytes4(sha3("share()")));
                }
            }
            function sendAll() public payable{
                // sender2.keep.value(msg.value + this.balance);
                // sender2.call.value(msg.value + this.balance)(bytes4(sha3("keep()")));
                sender2.call.value(this.balance)(bytes4(sha3("keep()")));
            }
            function keep() public payable{
            }
            function() payable { } //always payable
        }
        contract Sender2{ 
            // Sender1 sender1;
            // Sender3 sender3;
            address public sender1;
            address public sender3;
            
            function Sender2() {
            }
            function setSenders(address senderx, address sendery) public{
                // sender1=Sender1(senderx);
                // sender3=Sender3(sendery);
                sender1 = senderx;
                sender3 = sendery;
            }
            function share() public payable{
                // sender3.share.value(msg.value/2);
                sender3.call.value(msg.value/2)(bytes4(sha3("share()")));
            }
            function keep() public payable{
            }
            function withdrawAll() public{
                // sender3.withdraw();
                sender3.call(bytes4(sha3("withdraw()")));
                msg.sender.send(this.balance);
            }
            function() payable { } //always payable
        }

        contract Sender3 { 
            // Sender1 sender1;
            // Sender2 sender2;
            address public sender1;
            address public sender2;
            
            function Sender3() {
            }
            function setSenders(address senderx, address sendery) public{
                // sender1=Sender1(senderx);
                // sender2=Sender2(sendery);
                sender1 = senderx;
                sender2 = sendery;
            }
            function share() public payable{
                // sender1.share.value(msg.value/2);
                // sender2.keep.value(msg.value/4);
                sender1.call.value(msg.value/2)(bytes4(sha3("share()")));
                sender2.call.value(msg.value/4)(bytes4(sha3("keep()")));
            }
            function withdraw() public{
                msg.sender.send(this.balance);
            }
            function() payable { } //always payable
        }
        """
        sender1_bytecode = "6060604052341561000c57fe5b5b5b5b6104cb8061001e6000396000f30060606040523615610076576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680635579818d1461007f578063622836a3146100d45780639b0079d414610126578063a8d5fd6514610178578063e14f680f14610182578063e4d06d821461018c575b61007d5b5b565b005b341561008757fe5b6100d2600480803573ffffffffffffffffffffffffffffffffffffffff1690602001909190803573ffffffffffffffffffffffffffffffffffffffff16906020019091905050610196565b005b34156100dc57fe5b6100e461021d565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b341561012e57fe5b610136610243565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b610180610269565b005b61018a6103a9565b005b61019461049c565b005b81600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555080600160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b5050565b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b600160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b600160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff163373ffffffffffffffffffffffffffffffffffffffff161415156103a657600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1660023481151561030557fe5b0460405180807f7368617265282900000000000000000000000000000000000000000000000000815250600701905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886187965a03f19350505050505b5b565b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff163073ffffffffffffffffffffffffffffffffffffffff163160405180807f6b65657028290000000000000000000000000000000000000000000000000000815250600601905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886187965a03f19350505050505b565b5b5600a165627a7a72305820b491c90fc7b4f09ab3f6262b83707908d390a97f9730429d1ff5fa8e44a63b190029"
        self.sender1 = self.node.createcontract(sender1_bytecode, 1000000, QTUM_MIN_GAS_PRICE/COIN)['address']
        sender2_bytecode = "6060604052341561000c57fe5b5b5b5b6104b28061001e6000396000f30060606040523615610076576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680635579818d1461007f578063853828b6146100d45780639b0079d4146100e6578063a8d5fd6514610138578063e4d06d8214610142578063f34e0e7b1461014c575b61007d5b5b565b005b341561008757fe5b6100d2600480803573ffffffffffffffffffffffffffffffffffffffff1690602001909190803573ffffffffffffffffffffffffffffffffffffffff1690602001909190505061019e565b005b34156100dc57fe5b6100e4610225565b005b34156100ee57fe5b6100f661034f565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b610140610375565b005b61014a61045d565b005b341561015457fe5b61015c610460565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b81600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555080600160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b5050565b600160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1660405180807f7769746864726177282900000000000000000000000000000000000000000000815250600a01905060405180910390207c010000000000000000000000000000000000000000000000000000000090046040518163ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038160008761646e5a03f192505050503373ffffffffffffffffffffffffffffffffffffffff166108fc3073ffffffffffffffffffffffffffffffffffffffff16319081150290604051809050600060405180830381858888f19350505050505b565b600160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b600160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff166002348115156103ba57fe5b0460405180807f7368617265282900000000000000000000000000000000000000000000000000815250600701905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886187965a03f19350505050505b565b5b565b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16815600a165627a7a723058201842d5027fea2d624a38de6731e71832836efe8c51e5815b8ad85b7f3639e72a0029"
        self.sender2 = self.node.createcontract(sender2_bytecode, 1000000, QTUM_MIN_GAS_PRICE/COIN)['address']
        sender3_bytecode = "6060604052341561000c57fe5b5b5b5b6104a88061001e6000396000f3006060604052361561006b576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680633ccfd60b146100745780635579818d14610086578063622836a3146100db578063a8d5fd651461012d578063f34e0e7b14610137575b6100725b5b565b005b341561007c57fe5b610084610189565b005b341561008e57fe5b6100d9600480803573ffffffffffffffffffffffffffffffffffffffff1690602001909190803573ffffffffffffffffffffffffffffffffffffffff169060200190919050506101dc565b005b34156100e357fe5b6100eb610263565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b610135610289565b005b341561013f57fe5b610147610456565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b3373ffffffffffffffffffffffffffffffffffffffff166108fc3073ffffffffffffffffffffffffffffffffffffffff16319081150290604051809050600060405180830381858888f19350505050505b565b81600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555080600160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b5050565b600160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff166002348115156102ce57fe5b0460405180807f7368617265282900000000000000000000000000000000000000000000000000815250600701905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886187965a03f1935050505050600160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff166004348115156103b357fe5b0460405180807f6b65657028290000000000000000000000000000000000000000000000000000815250600601905060405180910390207c01000000000000000000000000000000000000000000000000000000009004906040518263ffffffff167c010000000000000000000000000000000000000000000000000000000002815260040180905060006040518083038185886187965a03f19350505050505b565b600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16815600a165627a7a72305820cb1b06b481990e1e218f7d0b51a3ffdf5b7439cfdd9bb2dccc1476cb84dfc95b0029"
        self.sender3 = self.node.createcontract(sender3_bytecode, 1000000, QTUM_MIN_GAS_PRICE/COIN)['address']

        self.node.generate(1)
        assert(len(self.node.listcontracts()) == 3+NUM_DEFAULT_DGP_CONTRACTS)

        self.keep_abi = "e4d06d82"
        self.sendAll_abi = "e14f680f"
        self.setSenders_abi = "5579818d"
        self.share_abi = "a8d5fd65"
        self.withdrawAll_abi = "853828b6"
        self.withdraw_abi = "3ccfd60b"
        self.sender1_abi = "f34e0e7b"
        self.sender2_abi = "622836a3"
        self.sender3_abi = "9b0079d4"

        padded_sender1 = self.sender1.zfill(64)
        padded_sender2 = self.sender2.zfill(64)
        padded_sender3 = self.sender3.zfill(64)
        self.node.sendtocontract(self.sender1, self.setSenders_abi + padded_sender2 + padded_sender3)
        self.node.sendtocontract(self.sender2, self.setSenders_abi + padded_sender1 + padded_sender3)
        self.node.sendtocontract(self.sender3, self.setSenders_abi + padded_sender1 + padded_sender2)
        self.node.generate(1)

        # Verify that the senders have been set correctly
        assert_equal(self.node.callcontract(self.sender1, self.sender2_abi)['executionResult']['output'][24:], self.sender2)
        assert_equal(self.node.callcontract(self.sender1, self.sender3_abi)['executionResult']['output'][24:], self.sender3)

        assert_equal(self.node.callcontract(self.sender2, self.sender1_abi)['executionResult']['output'][24:], self.sender1)
        assert_equal(self.node.callcontract(self.sender2, self.sender3_abi)['executionResult']['output'][24:], self.sender3)

        assert_equal(self.node.callcontract(self.sender3, self.sender1_abi)['executionResult']['output'][24:], self.sender1)
        assert_equal(self.node.callcontract(self.sender3, self.sender2_abi)['executionResult']['output'][24:], self.sender2)

    def run_test(self):
        self.node.generate(COINBASE_MATURITY+50)
        print("Setting up contracts and calling setSenders")
        self.setup_contracts()
        A1 = self.node.getnewaddress()
        self.node.sendtoaddress(A1, 1)
        self.node.generate(1)
        assert("vin" not in self.node.getaccountinfo(self.sender1))
        assert("vin" not in self.node.getaccountinfo(self.sender2))
        assert("vin" not in self.node.getaccountinfo(self.sender3))

        T1_id = self.node.sendtocontract(self.sender1, self.share_abi, 8)['txid']
        B2_id = self.node.generate(1)[0]
        B2 = self.node.getblock(B2_id)

        # Since this is a ṔoW block we only require 3 txs atm (coinbase, T1 and COND tx)
        assert_equal(B2['tx'][1], T1_id)
        assert_equal(len(B2['tx']), 3)
        C1_id = B2['tx'][2]

        C1 = self.node.getrawtransaction(C1_id, True)
        assert_vin(C1, [('OP_SPEND', )])
        assert_vout(C1, [(5, 'call'), (2.5, 'call'), (0.5, 'call')])
        assert("vin" in self.node.getaccountinfo(self.sender1))
        assert("vin" in self.node.getaccountinfo(self.sender2))
        assert("vin" in self.node.getaccountinfo(self.sender3))

        # We set the tx fee of T2 to a higher value such that it will be prioritized (be at index 1 in the block)
        T2_id = self.node.sendtocontract(self.sender1, self.keep_abi, 2, 50000, 0.0001)['txid']
        T3_id = self.node.sendtocontract(self.sender1, self.sendAll_abi, 2)['txid']
        B3_id = self.node.generate(1)[0]
        B3 = self.node.getblock(B3_id)

        # coinbase, T2, C2, T3, C3
        assert_equal(len(B3['tx']), 5)
        assert_equal(B3['tx'][1], T2_id)
        C2_id = B3['tx'][2]
        C3_id = B3['tx'][4]
        C2 = self.node.getrawtransaction(C2_id, True)
        C3 = self.node.getrawtransaction(C3_id, True)
        assert_vin(C2, [('OP_SPEND', ), ('OP_SPEND', )])
        assert_vout(C2, [(7, 'call')])
        assert_vin(C3, [('OP_SPEND', ), ('OP_SPEND', ), ('OP_SPEND', )])
        assert_vout(C3, [(11.5, 'call')])
        assert("vin" not in self.node.getaccountinfo(self.sender1))
        assert("vin" in self.node.getaccountinfo(self.sender2))
        assert("vin" in self.node.getaccountinfo(self.sender3))

        # We need the txfee to be higher than T5 so that T4 tx is prioritized over T5.
        # We set the gas such that the the tx will run but not immediately throw a out of gas exception
        T4_raw = make_transaction(self.node, [make_vin(self.node, 3*COIN)], [make_op_call_output(2*COIN, b"\x04", 22000, CScriptNum(QTUM_MIN_GAS_PRICE), hex_str_to_bytes(self.share_abi), hex_str_to_bytes(self.sender2))])
        T4_id = self.node.sendrawtransaction(T4_raw)
        T5_id = self.node.sendtocontract(self.sender2, self.withdrawAll_abi, 0, 1000000, QTUM_MIN_GAS_PRICE/COIN, A1)['txid']
        B4_id = self.node.generate(1)[0]
        B4 = self.node.getblock(B4_id)

        # Coinbase, T4, R1, T5, C4
        assert_equal(len(B4['tx']), 5)
        assert_equal(B4['tx'][1], T4_id)
        assert_equal(B4['tx'][3], T5_id)
        R1_id = B4['tx'][2]
        R1 = self.node.getrawtransaction(R1_id, True)

        C4_id = B4['tx'][4]
        C4 = self.node.getrawtransaction(C4_id, True)
        assert_vout(R1, [(2, 'pubkeyhash')])
        assert_vin(C4, [('OP_SPEND', ), ('OP_SPEND', )])
        assert_vout(C4, [(12, 'pubkeyhash')])
        assert_equal(sum(self.node.listcontracts().values()), 0)
        assert("vin" not in self.node.getaccountinfo(self.sender1))
        assert("vin" not in self.node.getaccountinfo(self.sender2))
        assert("vin" not in self.node.getaccountinfo(self.sender3))

if __name__ == '__main__':
    CondensingTxsTest().main()
