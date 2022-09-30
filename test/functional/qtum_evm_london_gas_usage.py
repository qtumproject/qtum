#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.address import *
from test_framework.qtum import *
import pprint
import subprocess

pp = pprint.PrettyPrinter()

class QtumEVMLondonTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-txindex', '-logevents=1', '-londonheight=10000']]
    
    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        
    def run_test(self):
        self.nodes[0].generate(COINBASE_MATURITY+50)
        self.node = self.nodes[0]
        
        """
        pragma solidity >=0.7.0;
        
        contract LondonContract {
            address private owner;
            uint256 num_store;
            constructor() {
                owner = msg.sender;
                num_store = 1;
            }
            function getBaseFee() public view returns (uint256 fee) {
                assembly {
                    fee := basefee()
                }
            }
            function close() public {
                address payable addr = payable(owner);
                selfdestruct(addr);
            }
            function getStore() public {
                num_store = 2;
                num_store = 3;
                num_store = 4;
            }
            function getLoad() public view returns (uint256 num_load) {
                num_load = num_store + 1;
                num_load = num_store + 2;
                num_load = num_store + 3;
            }
        
            function getExtcodesize(address addr) public view returns(uint32 size){
              assembly {
                size := extcodesize(addr)
                size := extcodesize(addr)
              }
            }
        }
        
        Function signatures:
        43d726d6: close()
        15e812ad: getBaseFee()
        458f6cf8: getExtcodesize(address)
        dfa2062e: getLoad()
        c2722ecc: getStore()
        """
        
        bytecode = "608060405234801561001057600080fd5b50336000806101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055506001808190555061033c806100676000396000f3fe608060405234801561001057600080fd5b50600436106100575760003560e01c806315e812ad1461005c57806343d726d61461007a578063458f6cf814610084578063c2722ecc146100b4578063dfa2062e146100be575b600080fd5b6100646100dc565b604051610071919061019c565b60405180910390f35b6100826100e4565b005b61009e6004803603810190610099919061021a565b610123565b6040516100ab9190610266565b60405180910390f35b6100bc610132565b005b6100c661014c565b6040516100d3919061019c565b60405180910390f35b600048905090565b60008060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690508073ffffffffffffffffffffffffffffffffffffffff16ff5b6000813b9050813b9050919050565b600260018190555060036001819055506004600181905550565b60006001805461015c91906102b0565b9050600260015461016d91906102b0565b9050600360015461017e91906102b0565b905090565b6000819050919050565b61019681610183565b82525050565b60006020820190506101b1600083018461018d565b92915050565b600080fd5b600073ffffffffffffffffffffffffffffffffffffffff82169050919050565b60006101e7826101bc565b9050919050565b6101f7816101dc565b811461020257600080fd5b50565b600081359050610214816101ee565b92915050565b6000602082840312156102305761022f6101b7565b5b600061023e84828501610205565b91505092915050565b600063ffffffff82169050919050565b61026081610247565b82525050565b600060208201905061027b6000830184610257565b92915050565b7f4e487b7100000000000000000000000000000000000000000000000000000000600052601160045260246000fd5b60006102bb82610183565b91506102c683610183565b9250827fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff038211156102fb576102fa610281565b5b82820190509291505056fea264697066735822122016446d11ce4f38e565be88aa14d62b19af5978fc992c575437620bc38fa1cec964736f6c634300080d0033"
        
        # Pre London fork tests
        switch_height = self.node.getblockcount()
        self.log.info('Runnig tests before london (height: %s)', switch_height)
        
        self.log.info('Create test contract')
        create_contract = self.node.createcontract(bytecode)
        self.contract_address = create_contract['address']
        self.sender = create_contract['sender']
        self.node.generate(1)
        
        self.log.info('Call getBaseFee()')
        info = self.node.callcontract(self.contract_address, "15e812ad")
        self.node.generate(1)
        assert_equal(info['executionResult']['excepted'], 'BadInstruction')
        
        self.log.info('Call getExtcodesize(address)')
        info = self.node.callcontract(self.contract_address, "458f6cf8"+self.contract_address.zfill(64))
        self.node.generate(1)
        assert_approx(info['executionResult']['gasUsed'], 23519, 12)
        
        self.log.info('Call getStore()')
        info = self.node.sendtocontract(self.contract_address, "c2722ecc", 0, 4000000, QTUM_MIN_GAS_PRICE_STR, self.sender)
        self.node.generate(1)
        receipt=self.node.gettransactionreceipt(info['txid'])[0];
        assert_equal(receipt['gasUsed'], 27894)
        
        self.log.info('Call getLoad()')
        info = self.node.callcontract(self.contract_address, "dfa2062e")
        self.node.generate(1)
        assert_equal(info['executionResult']['gasUsed'], 24465)
        
        self.log.info('Call close()')
        info = self.node.sendtocontract(self.contract_address, "43d726d6", 0, 4000000, QTUM_MIN_GAS_PRICE_STR, self.sender)
        self.node.generate(1)
        receipt=self.node.gettransactionreceipt(info['txid'])[0];
        assert_equal(receipt['gasUsed'], 13528)
        assert(self.contract_address not in self.node.listcontracts())
        
        # Post London fork tests
        switch_height = self.node.getblockcount()
        self.log.info('Runnig tests after london (height: %s)', switch_height)
        self.restart_node(0, ['-txindex', '-logevents', '-londonheight={}'.format(switch_height)])
        
        self.log.info('Create test contract')
        self.contract_address = self.node.createcontract(bytecode)['address']
        self.node.generate(1)
        
        self.log.info('Call getBaseFee()')
        info = self.node.callcontract(self.contract_address, "15e812ad")
        assert_equal(info['executionResult']['excepted'], 'None')
        
        self.log.info('Call getExtcodesize(address)')
        info = self.node.callcontract(self.contract_address, "458f6cf8"+self.contract_address.zfill(64))
        assert_approx(info['executionResult']['gasUsed'], 24819, 12)
        
        self.log.info('Call getStore()')
        info = self.node.sendtocontract(self.contract_address, "c2722ecc", 0, 4000000, QTUM_MIN_GAS_PRICE_STR, self.sender)
        self.node.generate(1)
        receipt=self.node.gettransactionreceipt(info['txid'])[0];
        assert_equal(receipt['gasUsed'], 26494)
        
        self.log.info('Call getLoad()')
        info = self.node.callcontract(self.contract_address, "dfa2062e")
        assert_equal(info['executionResult']['gasUsed'], 24365)
        
        self.log.info('Call close()')
        info = self.node.sendtocontract(self.contract_address, "43d726d6", 0, 4000000, QTUM_MIN_GAS_PRICE_STR, self.sender)
        hash=self.node.generate(1)
        receipt=self.node.gettransactionreceipt(info['txid'])[0];
        assert_equal(receipt['gasUsed'], 30955)
        assert(self.contract_address not in self.node.listcontracts())
        
if __name__ == '__main__':
    QtumEVMLondonTest().main()