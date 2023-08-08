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


class QtumEVMStaticCallTest(BitcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-logevents', '-minmempoolgaslimit=21000', '-constantinopleheight=%d' % (204 + COINBASE_MATURITY), '-muirglacierheight=100000', '-londonheight=100000']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def reset(self):
        self.node.sendtocontract(self.contract_address, 'd826f88f')
        self.node.generate(1)

    def get_value_at_index(self, i):
        info = self.node.callcontract(self.contract_address, "a05d8c0b" + hex(i)[2:].zfill(64))
        ret = info['executionResult']['output']
        return ret

    def generate_staticcall_abi(self, fun_name):
        abi = "00349441"
        abi += hex(0x20)[2:].zfill(64)
        param = ''.join(hex(ord(x))[2:] for x in fun_name)
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
            assert_equal((gas_refund_output['value'] * 100000000) // 10000000,
                         ((gas - gas_used) * gas_price * 100000000) // 10000000)
            assert_equal(sender_utxo['scriptPubKey']['asm'], gas_refund_output['scriptPubKey']['asm'])
        else:
            assert_equal(len(coinbase_tx['vout']), 2)

    def run_test(self):
        dummy_key = ECKey()
        dummy_key.generate()
        dummy_address = hex_hash_to_p2pkh("12"*20)

        self.node = self.nodes[0]
        self.node.generatetoaddress(200, self.node.getnewaddress())
        self.node.generatetoaddress(COINBASE_MATURITY, dummy_address)

        """
        pragma solidity ^0.5.10;
                
        contract Test {
            event TestTest();
            uint256[] private ret;
        
            function getRet(uint256 index) public view returns (uint256) {
                return ret[index];
            }
        
            function reset()  public {
                delete ret;
            } 
            
            function normal_fun() pure public {
                // do nothing
                uint256 i;
                for (i = 0; i < 10;++i) {}
            }
        
            function create_fun() public {
                bytes memory code = hex"0000";
                bytes memory mem_code = code;
                address addr;
                assembly {
                    addr := create(100, add(mem_code,0x20), mload(mem_code))
                }
            }
            
            function create2_fun() public {
                bytes memory code = hex"0000";
                uint256 r;
                uint256 size = code.length;
                assembly {
                    r := create2(0, add(code, 0x20), size, 0x0)
                }
            }
            
            function log0_fun() public {
                log0("0");
            }
            
            function log1_fun() public {
                log1("0", "1");
            }
        
            function log2_fun() public {
                log2("0", "1", "2");
            }
        
            function log3_fun() public {
                log3("0", "1", "2", "3");
            }
            
            function log4_fun() public {
                log4("0", "1", "2", "3", "4");
            }    
        
            function sstore_fun() public {
                assembly {
                    sstore(0, 2)
                }
            }
            
            function selfdestruct_fun() public {
                selfdestruct(msg.sender);
            }    
            
            function payable_call() public payable {
                // do nothing
            }
            
            function call_value_fun() public {
                bytes32 sig = bytes32(bytes4(keccak256("payable_call()")));
                assembly {
                    let x := mload(0x40)
                    mstore(x, sig)
                    let r := call(sub(gas, 100000), address, 1, x, 4, 0, 0)
                }
            }      
        
            function callcode_fun() public {
                bytes32 sig = bytes32(bytes4(keccak256("payable_call()")));
                assembly {
                    let x := mload(0x40)
                    mstore(x, sig)
                    let r := callcode(sub(gas, 100000), address, 0, x, 4, 0, 0)
                }
            }  
            
            function event_fun() public {
                emit TestTest();
            }
            
            function transfer_fun() public {
                msg.sender.transfer(1);
            }
        
            function staticcall_test(bytes memory fun_name) public payable {
                bytes32 sig = bytes32(bytes4(keccak256(fun_name)));
                uint256 retval;
                assembly {
                    let x := mload(0x40)
                    mstore(x, sig)
                    retval := staticcall(sub(gas, 100000), address, x, 4, 0, 0)
                }
                ret.push(retval);
            }
            
        }
        """

        bytecode = "608060405234801561001057600080fd5b506108ff806100206000396000f3fe6080604052600436106101085760003560e01c806385b3382811610095578063d402b4ea11610064578063d402b4ea14610307578063d826f88f1461031e578063da2bf6f914610335578063f820fa6e1461034c578063fac9ecf11461036357610108565b806385b33828146102735780638896fb041461028a578063a05d8c0b146102a1578063b0b05a5a146102f057610108565b806341a0e892116100dc57806341a0e89214610200578063571ffff11461021757806362d910841461022e5780637c1c850f146102455780637cf0b3cd1461025c57610108565b80623494411461010d5780630311f972146101c857806314a91a95146101df578063372c0357146101f6575b600080fd5b6101c66004803603602081101561012357600080fd5b810190808035906020019064010000000081111561014057600080fd5b82018360208201111561015257600080fd5b8035906020019184600183028401116401000000008311171561017457600080fd5b91908080601f016020809104026020016040519081016040528093929190818152602001838380828437600081840152601f19601f82011690508083019250505050505050919291929050505061037a565b005b3480156101d457600080fd5b506101dd6103ef565b005b3480156101eb57600080fd5b506101f46103f6565b005b6101fe610424565b005b34801561020c57600080fd5b50610215610426565b005b34801561022357600080fd5b5061022c6104a0565b005b34801561023a57600080fd5b506102436104d8565b005b34801561025157600080fd5b5061025a61054c565b005b34801561026857600080fd5b5061027161057e565b005b34801561027f57600080fd5b506102886105b1565b005b34801561029657600080fd5b5061029f61066d565b005b3480156102ad57600080fd5b506102da600480360360208110156102c457600080fd5b81019080803590602001909291905050506106b7565b6040518082815260200191505060405180910390f35b3480156102fc57600080fd5b506103056106d7565b005b34801561031357600080fd5b5061031c610730565b005b34801561032a57600080fd5b5061033361074d565b005b34801561034157600080fd5b5061034a61075c565b005b34801561035857600080fd5b506103616107f7565b005b34801561036f57600080fd5b50610378610810565b005b600081805190602001207bffffffffffffffffffffffffffffffffffffffffffffffffffffffff19169050600060405182815260008060048330620186a05a03fa9150506000819080600181540180825580915050906001820390600052602060002001600090919290919091505550505050565b6002600055565b7f902ab12fc657922f9e7e1085a23c967a546ad6f8a771c0b5c7db57f7aac0076e60405160405180910390a1565b565b7f32000000000000000000000000000000000000000000000000000000000000007f310000000000000000000000000000000000000000000000000000000000000060405180807f3000000000000000000000000000000000000000000000000000000000000000815250600101905060405180910390a2565b60405180807f3000000000000000000000000000000000000000000000000000000000000000815250600101905060405180910390a0565b600060405180807f70617961626c655f63616c6c2829000000000000000000000000000000000000815250600e01905060405180910390207bffffffffffffffffffffffffffffffffffffffffffffffffffffffff19169050604051818152600080600483600130620186a05a03f1505050565b606060405180604001604052806002815260200160008152509050606081905060008151602083016064f09050505050565b60606040518060400160405280600281526020016000815250905060008082519050600081602085016000f59150505050565b7f34000000000000000000000000000000000000000000000000000000000000007f33000000000000000000000000000000000000000000000000000000000000007f32000000000000000000000000000000000000000000000000000000000000007f310000000000000000000000000000000000000000000000000000000000000060405180807f3000000000000000000000000000000000000000000000000000000000000000815250600101905060405180910390a4565b3373ffffffffffffffffffffffffffffffffffffffff166108fc60019081150290604051600060405180830381858888f193505050501580156106b4573d6000803e3d6000fd5b50565b60008082815481106106c557fe5b90600052602060002001549050919050565b7f310000000000000000000000000000000000000000000000000000000000000060405180807f3000000000000000000000000000000000000000000000000000000000000000815250600101905060405180910390a1565b60008090505b600a81101561074a57806001019050610736565b50565b60008061075a9190610884565b565b7f33000000000000000000000000000000000000000000000000000000000000007f32000000000000000000000000000000000000000000000000000000000000007f310000000000000000000000000000000000000000000000000000000000000060405180807f3000000000000000000000000000000000000000000000000000000000000000815250600101905060405180910390a3565b3373ffffffffffffffffffffffffffffffffffffffff16ff5b600060405180807f70617961626c655f63616c6c2829000000000000000000000000000000000000815250600e01905060405180910390207bffffffffffffffffffffffffffffffffffffffffffffffffffffffff19169050604051818152600080600483600030620186a05a03f2505050565b50805460008255906000526020600020908101906108a291906108a5565b50565b6108c791905b808211156108c35760008160009055506001016108ab565b5090565b9056fea265627a7a72305820b81a82cf6ff756317c40a1d2fbbe83691f4a624cb87770795678a41ae6ce38f564736f6c634300050a0032"
        self.contract_address = self.node.createcontract(bytecode)['address']
        self.node.generatetoaddress(1, dummy_address)
        assert_equal(self.node.listcontracts()[self.contract_address], 0)

        # Run a normal valid staticcall tx
        # will cause a throw since staticcall is undefined before qip7/constantinople
        abi = self.generate_staticcall_abi("normal_fun()")
        self.node.sendtocontract(self.contract_address, abi, 10, 1000000, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        # check refund
        self.assert_state(self.node, gas=1000000, gas_price=0.000001, gas_used=1000000, value=10,
                                 excepted='BadInstruction')

        self.node.generatetoaddress(10, dummy_address)

        # Run a normal valid staticcall tx with low gas
        self.node.sendtocontract(self.contract_address, abi, 10, 64835, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=64835, gas_price=0.000001, gas_used=64835, value=10,
                                 excepted='OutOfGas')
        assert_equal(self.get_value_at_index(0), "")

        # Run a normal valid staticcall tx
        self.node.sendtocontract(self.contract_address, abi, 10, 1000000, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=1000000, gas_price=0.000001, gas_used=64836, value=0,
                                 excepted='None')
        assert_equal(self.get_value_at_index(0), "0" * 63 + "1")
        assert_equal(self.node.listcontracts()[self.contract_address], 10)

        # check invalid staticcall with `CREATE`
        self.reset()
        abi = self.generate_staticcall_abi("create_fun()")
        self.node.sendtocontract(self.contract_address, abi, 10, 1000000, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=1000000, gas_price=0.000001, gas_used=926056, value=0,
                                 excepted='None')
        assert_equal(self.get_value_at_index(0), "0" * 64)
        assert_equal(self.node.listcontracts()[self.contract_address], 20)

        # check invalid staticcall with `CREATE2`
        self.reset()
        abi = self.generate_staticcall_abi("create2_fun()")
        self.node.sendtocontract(self.contract_address, abi, 1, 1000000, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=1000000, gas_price=0.000001, gas_used=926056, value=0,
                                 excepted='None')
        assert_equal(self.get_value_at_index(0), "0" * 64)
        assert_equal(self.node.listcontracts()[self.contract_address], 21)

        # check invalid staticcall with `LOG0`
        self.reset()
        abi = self.generate_staticcall_abi("log0_fun()")
        self.node.sendtocontract(self.contract_address, abi, 1, 1000000, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=1000000, gas_price=0.000001, gas_used=926056, value=0,
                                 excepted='None')
        assert_equal(self.get_value_at_index(0), "0" * 64)
        assert_equal(self.node.listcontracts()[self.contract_address], 22)

        # check invalid staticcall with `LOG1`
        self.reset()
        abi = self.generate_staticcall_abi("log1_fun()")
        self.node.sendtocontract(self.contract_address, abi, 1, 1000000, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=1000000, gas_price=0.000001, gas_used=926056, value=0,
                                 excepted='None')
        assert_equal(self.get_value_at_index(0), "0" * 64)
        assert_equal(self.node.listcontracts()[self.contract_address], 23)

        # check invalid staticcall with `LOG2`
        self.reset()
        abi = self.generate_staticcall_abi("log2_fun()")
        self.node.sendtocontract(self.contract_address, abi, 1, 1000000, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=1000000, gas_price=0.000001, gas_used=926056, value=0,
                                 excepted='None')
        assert_equal(self.get_value_at_index(0), "0" * 64)
        assert_equal(self.node.listcontracts()[self.contract_address], 24)
        
        # check invalid staticcall with `LOG3`
        self.reset()
        abi = self.generate_staticcall_abi("log3_fun()")
        self.node.sendtocontract(self.contract_address, abi, 1, 1000000, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=1000000, gas_price=0.000001, gas_used=926056, value=0,
                                 excepted='None')
        assert_equal(self.get_value_at_index(0), "0" * 64)
        assert_equal(self.node.listcontracts()[self.contract_address], 25)

        # check invalid staticcall with `LOG4`
        self.reset()
        abi = self.generate_staticcall_abi("log4_fun()")
        self.node.sendtocontract(self.contract_address, abi, 1, 1000000, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=1000000, gas_price=0.000001, gas_used=926056, value=0,
                                 excepted='None')
        assert_equal(self.get_value_at_index(0), "0" * 64)
        assert_equal(self.node.listcontracts()[self.contract_address], 26)

        # check invalid staticcall with `SSTORE`
        self.reset()
        abi = self.generate_staticcall_abi("sstore_fun()")
        self.node.sendtocontract(self.contract_address, abi, 1, 1000000, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=1000000, gas_price=0.000001, gas_used=926056, value=0,
                                 excepted='None')
        assert_equal(self.get_value_at_index(0), "0" * 64)
        assert_equal(self.node.listcontracts()[self.contract_address], 27)

        # check invalid staticcall with `SELFDESTRUCT`
        self.reset()
        abi = self.generate_staticcall_abi("selfdestruct_fun()")
        self.node.sendtocontract(self.contract_address, abi, 1, 1000000, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=1000000, gas_price=0.000001, gas_used=926056, value=0,
                                 excepted='None')
        assert_equal(self.get_value_at_index(0), "0" * 64)
        assert_equal(self.node.listcontracts()[self.contract_address], 28)


        # check invalid staticcall with `CALL`
        self.reset()
        abi = self.generate_staticcall_abi("call_value_fun()")
        self.node.sendtocontract(self.contract_address, abi, 1, 1000000, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=1000000, gas_price=0.000001, gas_used=926056, value=0,
                                 excepted='None')
        assert_equal(self.get_value_at_index(0), "0" * 64)
        assert_equal(self.node.listcontracts()[self.contract_address], 29)

        # check valid staticcall with `CALLCODE`
        self.reset()
        abi = self.generate_staticcall_abi("callcode_fun()")
        self.node.sendtocontract(self.contract_address, abi, 1, 1000000, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=1000000, gas_price=0.000001, gas_used=65577, value=0,
                                 excepted='None')
        assert_equal(self.get_value_at_index(0), "0" * 63 + "1")
        assert_equal(self.node.listcontracts()[self.contract_address], 30)

        # check invalid staticcall with event
        self.reset()
        abi = self.generate_staticcall_abi("event_fun()")
        self.node.sendtocontract(self.contract_address, abi, 1, 1000000, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=1000000, gas_price=0.000001, gas_used=926056, value=0,
                                 excepted='None')
        assert_equal(self.get_value_at_index(0), "0" * 64)
        assert_equal(self.node.listcontracts()[self.contract_address], 31)

        # check invalid staticcall with transfer
        self.reset()
        abi = self.generate_staticcall_abi("transfer_fun()")
        self.node.sendtocontract(self.contract_address, abi, 1, 1000000, 0.000001)
        self.node.generatetoaddress(1, dummy_address)
        self.assert_state(self.node, gas=1000000, gas_price=0.000001, gas_used=926056, value=0,
                                 excepted='None')
        assert_equal(self.get_value_at_index(0), "0" * 64)
        assert_equal(self.node.listcontracts()[self.contract_address], 32)


if __name__ == '__main__':
    QtumEVMStaticCallTest().main()
