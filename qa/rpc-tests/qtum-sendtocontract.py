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
import time

class SendToContractTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [['-txindex=1']])
        self.node = self.nodes[0]
        self.is_network_split = False
 
    def setup_contract(self):
        """
        pragma solidity ^0.4.0;

        contract Test {
            address owner;
            function Test(){
                owner=msg.sender;
            }
            function setOwner(address test) payable{
                owner=test;
            }
            function setSenderAsOwner() payable{
                owner=msg.sender;
            }
            function getOwner() constant returns (address cOwner){
                return owner;
            }
            function getSender() constant returns (address cSender){
                return msg.sender;
            }
        }
        """
        contract_data = self.node.createcontract("6060604052341561000c57fe5b5b33600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b5b6102218061005f6000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806313af40351461005c5780632bcf51b41461008a5780635e01eb5a14610094578063893d20e8146100e6575bfe5b610088600480803573ffffffffffffffffffffffffffffffffffffffff16906020019091905050610138565b005b61009261017d565b005b341561009c57fe5b6100a46101c1565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b34156100ee57fe5b6100f66101ca565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b80600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b50565b33600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b565b60003390505b90565b6000600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690505b905600a165627a7a72305820791895230daae0ed58cc374ad5b639044408f1942d7b85689a616caee50dc42e0029", 1000000, QTUM_MIN_GAS_PRICE/COIN)
        self.contract_address = contract_data['address']
        self.node.generate(1)


    # Verifies that the contract storage and abi work correctly
    def sendtocontract_verify_storage_test(self):
        # call set owner with 8888888888888888888888888888888888888888 as owner
        ret = self.node.sendtocontract(self.contract_address, "13af40350000000000000000000000008888888888888888888888888888888888888888")
        assert('txid' in ret)
        assert('sender' in ret)
        assert('hash160' in ret)
        self.node.generate(1)
        
        ret = self.node.getaccountinfo(self.contract_address)
        assert_equal(ret['address'], self.contract_address)
        assert_equal(ret['balance'], 0)
        assert_equal(ret['storage']['290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e563']['0000000000000000000000000000000000000000000000000000000000000000'], '0000000000000000000000008888888888888888888888888888888888888888')
        assert_equal(ret['code'], '60606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806313af40351461005c5780632bcf51b41461008a5780635e01eb5a14610094578063893d20e8146100e6575bfe5b610088600480803573ffffffffffffffffffffffffffffffffffffffff16906020019091905050610138565b005b61009261017d565b005b341561009c57fe5b6100a46101c1565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b34156100ee57fe5b6100f66101ca565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b80600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b50565b33600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b565b60003390505b90565b6000600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690505b905600a165627a7a72305820791895230daae0ed58cc374ad5b639044408f1942d7b85689a616caee50dc42e0029')


    # Verifies that the contract storage, abi, balance work correctly
    def sendtocontract_verify_storage_and_balance_test(self):
        # call set owner with 9999999999999999999999999999999999999999 as owner with 100 qtum
        ret = self.node.sendtocontract(self.contract_address, "13af40350000000000000000000000009999999999999999999999999999999999999999", 100)
        assert('txid' in ret)
        assert('sender' in ret)
        assert('hash160' in ret)
        self.node.generate(1)
        
        ret = self.node.getaccountinfo(self.contract_address)
        #print(ret)
        assert_equal(ret['address'], self.contract_address)
        assert_equal(ret['balance'], 10000000000)
        assert_equal(ret['storage']['290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e563']['0000000000000000000000000000000000000000000000000000000000000000'], '0000000000000000000000009999999999999999999999999999999999999999')
        assert_equal(ret['code'], '60606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806313af40351461005c5780632bcf51b41461008a5780635e01eb5a14610094578063893d20e8146100e6575bfe5b610088600480803573ffffffffffffffffffffffffffffffffffffffff16906020019091905050610138565b005b61009261017d565b005b341561009c57fe5b6100a46101c1565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b34156100ee57fe5b6100f66101ca565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b80600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b50565b33600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b565b60003390505b90565b6000600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690505b905600a165627a7a72305820791895230daae0ed58cc374ad5b639044408f1942d7b85689a616caee50dc42e0029')


    def sendtocontract_specify_sender_test(self):
        self.node.importprivkey("cQWxca9y9XBf4c6ohTwRQ9Kf4GZyRybhGBfzaFgkvRpw8HjbRC58")
        self.node.sendtoaddress("qabmqZk3re5b9UpUcznxDkCnCsnKdmPktT", 0.1)
        self.node.generate(1)
        # call setSenderAsOwner with 100 qtum
        ret = self.node.sendtocontract(self.contract_address, "2bcf51b4", 100, 1000000, QTUM_MIN_GAS_PRICE/COIN, "qabmqZk3re5b9UpUcznxDkCnCsnKdmPktT")
        assert('txid' in ret)
        assert('sender' in ret)
        assert('hash160' in ret)
        time.sleep(0.1)
        self.node.generate(1)
        ret = self.node.getaccountinfo(self.contract_address)
        assert(ret['address'] == self.contract_address)
        assert(ret['balance'] == 20000000000)
        assert('290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e563' in ret['storage'])
        assert('0000000000000000000000000000000000000000000000000000000000000000' in ret['storage']['290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e563'])
        assert(ret['storage']['290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e563']['0000000000000000000000000000000000000000000000000000000000000000'] == "000000000000000000000000baede766c1a4844efea1326fffc5cd8c1c3e42ae")
        assert(ret['code'] == '60606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806313af40351461005c5780632bcf51b41461008a5780635e01eb5a14610094578063893d20e8146100e6575bfe5b610088600480803573ffffffffffffffffffffffffffffffffffffffff16906020019091905050610138565b005b61009261017d565b005b341561009c57fe5b6100a46101c1565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b34156100ee57fe5b6100f66101ca565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b80600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b50565b33600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b565b60003390505b90565b6000600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690505b905600a165627a7a72305820791895230daae0ed58cc374ad5b639044408f1942d7b85689a616caee50dc42e0029')

    def sendtocontract_no_broadcast(self):
        self.node.sendtoaddress("qabmqZk3re5b9UpUcznxDkCnCsnKdmPktT", 0.1)
        self.node.generate(1)
        # call setSenderAsOwner with 100 qtum
        ret = self.node.sendtocontract(self.contract_address, "2bcf51b4", 100, 1000000, QTUM_MIN_GAS_PRICE/COIN, "qabmqZk3re5b9UpUcznxDkCnCsnKdmPktT", False)
        assert('raw transaction' in ret)
        assert(len(ret.keys()) == 1)
        decoded_tx = self.node.decoderawtransaction(ret['raw transaction'])
        # verify that at least one output has a scriptPubKey of type call
        for out in decoded_tx['vout']:
            if out['scriptPubKey']['type'] == 'call':
                return
        assert(False)

    def run_test(self):
        self.nodes[0].generate(100+COINBASE_MATURITY)
        self.setup_contract()
        self.sendtocontract_verify_storage_test()
        self.sendtocontract_verify_storage_and_balance_test()
        self.sendtocontract_specify_sender_test()
        self.sendtocontract_no_broadcast()

if __name__ == '__main__':
    SendToContractTest().main()