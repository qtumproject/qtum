#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.address import *
import threading

def waitforlogs(node, contract_address):
    logs = node.cli.waitforlogs(node.cli.getblockcount()-1, COINBASE_MATURITY+500, '{"addresses": ["'+contract_address+'"]}')
    node.result = logs


class QtumTransactionReceiptOriginContractAddressTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-logevents', '-txindex']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.node = self.nodes[0]
        self.nodes[0].generate(10 + COINBASE_MATURITY)
        """
        pragma solidity ^0.5.2;

        contract Test {
            event TestEvent();
            address private child;
            function setChildContract(address childContractAddress) external {
                child = childContractAddress;
            }
            function doEvent() external {
                if(child == address(0x0)) {
                    emit TestEvent();
                } else {
                    Test(child).doEvent();
                }
            }
            function getChildAddress() public view returns(address) {
                return child;
            }
        }
        """
        """
        Function signatures: 
        afd67ce7: doEvent()
        bcb1c3a9: getChildAddress()
        f8d86e18: setChildContract(address)
        """

        # Set up a chain of 10 contracts that reference their child contract. I.e. the tenth contract is the leaf
        contracts = []
        contract_bytecode = "608060405234801561001057600080fd5b506102b8806100206000396000f3fe608060405234801561001057600080fd5b506004361061005e576000357c010000000000000000000000000000000000000000000000000000000090048063afd67ce714610063578063bcb1c3a91461006d578063f8d86e18146100b7575b600080fd5b61006b6100fb565b005b610075610220565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b6100f9600480360360208110156100cd57600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff169060200190929190505050610249565b005b600073ffffffffffffffffffffffffffffffffffffffff166000809054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff161415610182577f24ec1d3ff24c2f6ff210738839dbc339cd45a5294d85c79361016243157aae7b60405160405180910390a161021e565b6000809054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1663afd67ce76040518163ffffffff167c0100000000000000000000000000000000000000000000000000000000028152600401600060405180830381600087803b15801561020757600080fd5b5060325a03f115801561021957600080fd5b505050505b565b60008060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16905090565b806000806101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505056fea165627a7a723058203cf61a18e40f6e2bd01b2f7bd607c6e6aff032f12bd5e3eca68212d2e2c80dbf0029"
        for i in range(10):
            contracts.append(self.nodes[0].createcontract(contract_bytecode)['address'])
            self.node.generate(1)
            if len(contracts) > 1:
                self.node.sendtocontract(contracts[-2], "f8d86e18" + (contracts[-1].zfill(64)), 0, 1000000)
                self.node.generate(1)
        
        # Run the doEvent function recursively starting at the root contract and make sure that no event entries is in the returndata for waitforlogs for the first 9 contracts
        for contract_address in contracts[:-1]:
            thread = threading.Thread(target=waitforlogs, args=(self.node, contract_address))
            thread.start()
            txid = self.node.sendtocontract(contracts[0], "afd67ce7", 0, 1000000)['txid']
            self.node.generate(7)
            thread.join()
            receipt = self.node.gettransactionreceipt(txid)
            assert_equal(receipt[0]['log'][0]['address'], contracts[-1])
            assert_equal(len(self.node.result['entries']), 0)


        # Do the same thing again but make sure that the event triggers for the "leaf" (10th) contract
        thread = threading.Thread(target=waitforlogs, args=(self.node, contracts[-1]))
        thread.start()
        txid = self.node.sendtocontract(contracts[0], "afd67ce7", 0, 1000000)['txid']
        self.node.generate(7)
        thread.join()
        receipt = self.node.gettransactionreceipt(txid)
        assert_equal(receipt[0]['log'][0]['address'], contracts[-1])
        assert_equal(len(self.node.result['entries']), 1)


if __name__ == '__main__':
    QtumTransactionReceiptOriginContractAddressTest().main()
