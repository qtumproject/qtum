#!/usr/bin/env python3
# Copyright (c) 2016-2019 The Qtum Core developers
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


class QtumEVMCreate2Test(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-logevents', '-minmempoolgaslimit=21000', '-constantinopleheight=%d' % (204 + COINBASE_MATURITY), '-muirglacierheight=100000', '-londonheight=1000000']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def get_value_at_index(self, i):
        info = self.node.callcontract(self.contract_address, "a05d8c0b" + hex(i)[2:].zfill(64))
        ret = info['executionResult']['output']
        return ret

    def reset(self):
        self.node.sendtocontract(self.contract_address, 'd826f88f')
        self.node.generate(1)

    def keccak256(self, data):
        abi = "b8612c0a"
        abi += hex(0x20)[2:].zfill(64)
        abi += hex(len(data) // 2)[2:].zfill(64)
        abi += data
        return self.node.callcontract(self.contract_address, abi)['executionResult']['output']

    def generate_create2_abi(self, param):
        abi = "9fcc1813"
        abi += hex(0x20)[2:].zfill(64)
        abi += hex(len(param) // 2)[2:].zfill(64)
        abi += param
        return abi

    def assert_state(self, node, gas, gas_price, gas_used, value, excepted):
        blockhash = node.getbestblockhash()
        block = node.getblock(blockhash)
        txids = block['tx']
        coinbase_tx = node.getrawtransaction(txids[0], True, blockhash)
        call_tx = node.getrawtransaction(txids[1], True, blockhash)

        input_tx = node.decoderawtransaction(node.gettransaction(call_tx['vin'][0]['txid'])['hex'])
        sender_utxo = input_tx['vout'][call_tx['vin'][0]['vout']]
        sender_address = sender_utxo['scriptPubKey']['address']

        for op_call_vout_index in range(len(call_tx['vout'])):
            if call_tx['vout'][op_call_vout_index]['scriptPubKey']['type'] == 'call_sender':
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
            assert_equal(len(txids), 3)

        # Check that the coinstake contains a gas refund (if one should exist)
        if gas > gas_used:
            gas_refund_output = coinbase_tx['vout'][-2]
            assert_equal((gas_refund_output['value']*100000000)//10000000, ((gas-gas_used)*gas_price*100000000)//10000000)
            assert_equal(sender_utxo['scriptPubKey']['asm'], gas_refund_output['scriptPubKey']['asm'])
        else:
            assert_equal(len(coinbase_tx['vout']), 2)

    def run_test(self):
        # Dummy address to generate blocks to
        dummy_key = ECKey()
        dummy_key.generate()
        dummy_address = hex_hash_to_p2pkh("12"*20)

        self.node = self.nodes[0]
        self.node.generatetoaddress(200, self.node.getnewaddress())
        self.node.generatetoaddress(COINBASE_MATURITY, dummy_address)

        """
        pragma solidity ^0.5.10;
        
        contract Test {
            
            uint256[] private ret;
            
            function reset()  public {
                delete ret;
            }
        
            function getRet(uint256 index) public view returns (uint256) {
                return ret[index];
            }
            
            // To avoid having to add a dependancy for the non-standard keccak256 used in the EVM
            // we simply call it here to verify results of extcodehash and create2
            function dokeccak256(bytes memory data) public view returns (bytes32) {
                return keccak256(data);
            }
            
            function create2_test(bytes memory code) public payable {
                uint256 r;
                uint256 size = code.length;
                assembly {
                    r := create2(0, add(code, 0x20), size, 0x0)
                }
                ret.push(r);
            }
        }
        
        d826f88f: reset()
        a05d8c0b: getRet(uint256)
        b8612c0a: dokeccak256(bytes)
        9fcc1813: create2_test(bytes)        
        """

        bytecode = "608060405234801561001057600080fd5b50610340806100206000396000f3fe60806040526004361061003f5760003560e01c80639fcc181314610044578063a05d8c0b146100ff578063b8612c0a1461014e578063d826f88f1461022a575b600080fd5b6100fd6004803603602081101561005a57600080fd5b810190808035906020019064010000000081111561007757600080fd5b82018360208201111561008957600080fd5b803590602001918460018302840111640100000000831117156100ab57600080fd5b91908080601f016020809104026020016040519081016040528093929190818152602001838380828437600081840152601f19601f820116905080830192505050505050509192919290505050610241565b005b34801561010b57600080fd5b506101386004803603602081101561012257600080fd5b8101908080359060200190929190505050610285565b6040518082815260200191505060405180910390f35b34801561015a57600080fd5b506102146004803603602081101561017157600080fd5b810190808035906020019064010000000081111561018e57600080fd5b8201836020820111156101a057600080fd5b803590602001918460018302840111640100000000831117156101c257600080fd5b91908080601f016020809104026020016040519081016040528093929190818152602001838380828437600081840152601f19601f8201169050808301925050505050505091929192905050506102a5565b6040518082815260200191505060405180910390f35b34801561023657600080fd5b5061023f6102b6565b005b60008082519050600081602085016000f591506000829080600181540180825580915050906001820390600052602060002001600090919290919091505550505050565b600080828154811061029357fe5b90600052602060002001549050919050565b600081805190602001209050919050565b6000806102c391906102c5565b565b50805460008255906000526020600020908101906102e391906102e6565b50565b61030891905b808211156103045760008160009055506001016102ec565b5090565b9056fea265627a7a7230582081aabd93f08842152eaf2ff607e68cee84abb692b2fbfa0556ee4e0a29ce332464736f6c634300050a0032"
        self.contract_address = self.node.createcontract(bytecode)['address']
        self.node.generatetoaddress(1, dummy_address)

        create2_bytecode = "608060405234801561001057600080fd5b50610a57806100206000396000f3fe6080604052600436106100e85760003560e01c80639fcc18131161008a578063c5b319b211610059578063c5b319b214610473578063c9846c59146104b8578063d826f88f146104cf578063e5037fb1146104e6576100e8565b80639fcc181314610221578063a05d8c0b146102dc578063a1d98fa11461032b578063b8612c0a14610397576100e8565b80634f2f0adc116100c65780634f2f0adc1461015d578063825d4ba3146101ae5780639c9d8fbe146101f35780639ca24dff1461020a576100e8565b806308a86e5f146100ea5780631bbfb6821461012f5780634772905514610146575b005b3480156100f657600080fd5b5061012d6004803603604081101561010d57600080fd5b8101908080359060200190929190803590602001909291905050506104fd565b005b34801561013b57600080fd5b50610144610532565b005b34801561015257600080fd5b5061015b61061f565b005b34801561016957600080fd5b506101ac6004803603602081101561018057600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff1690602001909291905050506106c1565b005b3480156101ba57600080fd5b506101f1600480360360408110156101d157600080fd5b8101908080359060200190929190803590602001909291905050506106f7565b005b3480156101ff57600080fd5b5061020861072c565b005b34801561021657600080fd5b5061021f6107ce565b005b6102da6004803603602081101561023757600080fd5b810190808035906020019064010000000081111561025457600080fd5b82018360208201111561026657600080fd5b8035906020019184600183028401116401000000008311171561028857600080fd5b91908080601f016020809104026020016040519081016040528093929190818152602001838380828437600081840152601f19601f82011690508083019250505050505050919291929050505061081f565b005b3480156102e857600080fd5b50610315600480360360208110156102ff57600080fd5b8101908080359060200190929190505050610863565b6040518082815260200191505060405180910390f35b34801561033757600080fd5b50610340610883565b6040518080602001828103825283818151815260200191508051906020019060200280838360005b83811015610383578082015181840152602081019050610368565b505050509050019250505060405180910390f35b3480156103a357600080fd5b5061045d600480360360208110156103ba57600080fd5b81019080803590602001906401000000008111156103d757600080fd5b8201836020820111156103e957600080fd5b8035906020019184600183028401116401000000008311171561040b57600080fd5b91908080601f016020809104026020016040519081016040528093929190818152602001838380828437600081840152601f19601f820116905080830192505050505050509192919290505050610925565b6040518082815260200191505060405180910390f35b34801561047f57600080fd5b506104b66004803603604081101561049657600080fd5b810190808035906020019092919080359060200190929190505050610936565b005b3480156104c457600080fd5b506104cd61096b565b005b3480156104db57600080fd5b506104e46109b9565b005b3480156104f257600080fd5b506104fb6109c8565b005b80821c915060008290806001815401808255809150509060018203906000526020600020016000909192909190915055505050565b60006060600060405180807f72657475726e7661726c656e6774682829000000000000000000000000000000815250601101905060405180910390207bffffffffffffffffffffffffffffffffffffffffffffffffffffffff1916905060008090506040518281526040519350600080600483600030620186a05a03f13d95506040519450602086036020863e5050600090505b82518110156106195760008382815181106105dd57fe5b602002602001015190806001815401808255809150509060018203906000526020600020016000909192909190915055508060010190506105c6565b50505050565b600060405180807f6d757461626c6563616c6c6261636b2829000000000000000000000000000000815250601101905060405180910390207bffffffffffffffffffffffffffffffffffffffffffffffffffffffff19169050600060405182815260008060048330620186a05a03fa91505060008190806001815401808255809150509060018203906000526020600020016000909192909190915055505050565b6000813f905060008190806001815401808255809150509060018203906000526020600020016000909192909190915055505050565b80821d915060008290806001815401808255809150509060018203906000526020600020016000909192909190915055505050565b600060405180807f73746174696363616c6c6261636b282900000000000000000000000000000000815250601001905060405180910390207bffffffffffffffffffffffffffffffffffffffffffffffffffffffff19169050600060405182815260008060048330620186a05a03fa91505060008190806001815401808255809150509060018203906000526020600020016000909192909190915055505050565b60007f0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef9080600181540180825580915050906001820390600052602060002001600090919290919091505550600080fd5b60008082519050600081602085016000f591506000829080600181540180825580915050906001820390600052602060002001600090919290919091505550505050565b600080828154811061087157fe5b90600052602060002001549050919050565b60606000600f600143034060001c1690506060816040519080825280602002602001820160405280156108c55781602001602082028038833980820191505090505b50905060008090505b8281101561091c57807ff0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f01782828151811061090557fe5b6020026020010181815250508060010190506108ce565b50809250505090565b600081805190602001209050919050565b80821b915060008290806001815401808255809150509060018203906000526020600020016000909192909190915055505050565b60007f12121212121212121212121212121212121212121212121212121212121212129080600181540180825580915050906001820390600052602060002001600090919290919091505550565b6000806109c691906109e5565b565b60008090505b600a8110156109e2578060010190506109ce565b50565b5080546000825590600052602060002090810190610a039190610a06565b50565b610a2891905b80821115610a24576000816000905550600101610a0c565b5090565b9056fea165627a7a72305820db27ae89f2aafaaa420f35ae90480db051520a88007ede039df1c2302d284ec20029"
        abi = self.generate_create2_abi(create2_bytecode)

        # Run a normal create2 tx, will cause a throw since create2 is undefined before qip7/constantinople
        # this will cause a refund tx
        self.node.sendtocontract(self.contract_address, abi, 10, 1000000, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=1000000, gas_price=0.000001, gas_used=1000000, value=10,
                                 excepted='BadInstruction')

        self.node.generatetoaddress(10, dummy_address)

        # run a create2 tx with too little gas
        # this will cause a refund tx
        self.node.sendtocontract(self.contract_address, abi, 10, 795405, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=795405, gas_price=0.000001, gas_used=795405, value=10, excepted='OutOfGas')

        # run a create2 tx with too little gas
        # this will cause a refund tx
        self.node.sendtocontract(self.contract_address, abi, 10, 795406, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=795406, gas_price=0.000001, gas_used=795406, value=10, excepted='OutOfGas')

        # run a create2 tx with just enough gas
        # this will not cause a refund tx
        self.node.sendtocontract(self.contract_address, abi, 10, 795407, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=795407, gas_price=0.000001, gas_used=795407, value=0, excepted='None')

        # check contract address
        codehash = self.keccak256(create2_bytecode)
        expected_create2_address = self.keccak256("ff" + self.contract_address + ("0" * 64) + codehash)[24:]
        create2_address = self.get_value_at_index(0)[24:]
        assert (create2_address in self.node.listcontracts())
        assert_equal(expected_create2_address, create2_address)

        # Send some value to the create2 address, should succeed
        self.node.sendtocontract(create2_address, "00", 1)
        self.node.generate(1)
        assert_equal(self.node.listcontracts()[create2_address], 1)

        # Make sure that creating another exactly the same create2 contract fails
        self.reset()
        self.node.sendtocontract(self.contract_address, abi, 10, 10000000, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        assert_equal(self.get_value_at_index(0), "0" * 64)
        # Make sure that the value associated with the create2 address remains the same
        assert_equal(self.node.listcontracts()[create2_address], 1)

        # run a create2 tx with more than enough gas
        self.reset()
        create2_bytecode = '000000'
        abi = self.generate_create2_abi(create2_bytecode)
        self.node.sendtocontract(self.contract_address, abi, 10, 10000000, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=10000000, gas_price=0.000001, gas_used=94562, value=0, excepted='None')

        # check contract address
        codehash = self.keccak256(create2_bytecode)
        expected_create2_address = self.keccak256("ff" + self.contract_address + ("0" * 64) + codehash)[24:]
        create2_address = self.get_value_at_index(0)[24:]
        assert (create2_address in self.node.listcontracts())
        assert_equal(expected_create2_address, create2_address)

        # balance of self.contract_address should be 30 so far
        assert_equal(self.node.listcontracts()[self.contract_address], 30)


if __name__ == '__main__':
    QtumEVMCreate2Test().main()
