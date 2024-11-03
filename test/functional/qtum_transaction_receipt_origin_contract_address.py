#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.address import *
import time

class QtumTransactionReceiptOriginContractAddressTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-logevents', '-txindex']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def wait_for_logs(self, contract_address, start_block, timeout=30):
        """
        Poll for logs with timeout
        """
        end_time = time.time() + timeout
        while time.time() < end_time:
            try:
                current_block = self.node.getblockcount()
                logs = self.node.waitforlogs(
                    fromblock=start_block,
                    toblock=current_block,
                    filter={"addresses": [contract_address]}
                )
                return logs
            except Exception as e:
                if "waitforlogs timeout" not in str(e):
                    raise
                time.sleep(1)
        
        raise TimeoutError(f"No logs found for contract {contract_address} after {timeout} seconds")

    def run_test(self):
        self.node = self.nodes[0]
        self.nodes[0].generate(10 + COINBASE_MATURITY)

        # Contract bytecode (unchanged)
        contract_bytecode = "608060405234801561001057600080fd5b506102b8806100206000396000f3fe608060405234801561001057600080fd5b506004361061005e576000357c010000000000000000000000000000000000000000000000000000000090048063afd67ce714610063578063bcb1c3a91461006d578063f8d86e18146100b7575b600080fd5b61006b6100fb565b005b610075610220565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b6100f9600480360360208110156100cd57600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff169060200190929190505050610249565b005b600073ffffffffffffffffffffffffffffffffffffffff166000809054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff161415610182577f24ec1d3ff24c2f6ff210738839dbc339cd45a5294d85c79361016243157aae7b60405160405180910390a161021e565b6000809054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1663afd67ce76040518163ffffffff167c0100000000000000000000000000000000000000000000000000000000028152600401600060405180830381600087803b15801561020757600080fd5b5060325a03f115801561021957600080fd5b505050505b565b60008060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16905090565b806000806101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505056fea165627a7a723058203cf61a18e40f6e2bd01b2f7bd607c6e6aff032f12bd5e3eca68212d2e2c80dbf0029"

        # Set up contract chain
        contracts = []
        for i in range(10):
            contracts.append(self.nodes[0].createcontract(contract_bytecode)['address'])
            self.node.generate(1)
            if len(contracts) > 1:
                self.node.sendtocontract(contracts[-2], "f8d86e18" + (contracts[-1].zfill(64)), 0, 1000000)
                self.node.generate(1)

        # Test non-leaf contracts (first 9)
        for contract_address in contracts[:-1]:
            # Get current block before transaction
            start_block = self.node.getblockcount()
            
            # Send contract transaction
            txid = self.node.sendtocontract(contracts[0], "afd67ce7", 0, 1000000)['txid']
            self.node.generate(7)

            # Wait for logs and verify
            logs = self.wait_for_logs(contract_address, start_block)
            receipt = self.node.gettransactionreceipt(txid)
            assert_equal(receipt[0]['log'][0]['address'], contracts[-1])
            assert_equal(len(logs['entries']), 0)

        # Test leaf contract (10th)
        start_block = self.node.getblockcount()
        txid = self.node.sendtocontract(contracts[0], "afd67ce7", 0, 1000000)['txid']
        self.node.generate(7)

        # Wait for logs and verify
        logs = self.wait_for_logs(contracts[-1], start_block)
        receipt = self.node.gettransactionreceipt(txid)
        assert_equal(receipt[0]['log'][0]['address'], contracts[-1])
        assert_equal(len(logs['entries']), 1)

if __name__ == '__main__':
    QtumTransactionReceiptOriginContractAddressTest().main()