#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.qtum import *
from test_framework.address import *
from test_framework.blocktools import *


"""
Note, these tests do not test the functionality of the DGP template contract itself, for tests for the DGP template, see qtum-dgp.py
"""
class QtumDGPGasPriceLingeringMempoolTxTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [['-rpcmaxgasprice=10000000'], ['-rpcmaxgasprice=10000000']])
        self.node = self.nodes[0]
        self.GAS_PRICE_DGP = DGPState(self.node, "0000000000000000000000000000000000000082")
        connect_nodes_bi(self.nodes, 0, 1)
        self.is_network_split = False

    def create_proposal_contract(self, min_gas_price):
        """
        pragma solidity ^0.4.11;

        contract minGasPrice {
            uint32[1] _minGasPrice = [
                1 //min gas price in satoshis
            ];
            function getMinGasPrice() constant returns (uint32[1] _gasPrice) {
                return _minGasPrice;
            }
        }
        """
        if min_gas_price == 1:
            contract_data = self.node.createcontract("6060604052602060405190810160405280600160ff168152506000906001610028929190610039565b50341561003457600080fd5b61010c565b8260016007016008900481019282156100c85791602002820160005b8382111561009657835183826101000a81548163ffffffff021916908360ff1602179055509260200192600401602081600301049283019260010302610055565b80156100c65782816101000a81549063ffffffff0219169055600401602081600301049283019260010302610096565b505b5090506100d591906100d9565b5090565b61010991905b8082111561010557600081816101000a81549063ffffffff0219169055506001016100df565b5090565b90565b6101628061011b6000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680633fb588191461003e575b600080fd5b341561004957600080fd5b610051610090565b6040518082600160200280838360005b8381101561007d5780820151818401525b602081019050610061565b5050505090500191505060405180910390f35b610098610108565b60006001806020026040519081016040528092919082600180156100fd576020028201916000905b82829054906101000a900463ffffffff1663ffffffff16815260200190600401906020826003010492830192600103820291508084116100c05790505b505050505090505b90565b6020604051908101604052806001905b600063ffffffff1681526020019060019003908161011857905050905600a165627a7a72305820485c90d0bd56d7e4a7c59cc8bfbe0a02b9c7106f74eaf66f03dc8b8b38b732030029", 1000000, 0.001)

        if min_gas_price == 100:
            contract_data = self.node.createcontract("6060604052602060405190810160405280606460ff168152506000906001610028929190610039565b50341561003457600080fd5b61010c565b8260016007016008900481019282156100c85791602002820160005b8382111561009657835183826101000a81548163ffffffff021916908360ff1602179055509260200192600401602081600301049283019260010302610055565b80156100c65782816101000a81549063ffffffff0219169055600401602081600301049283019260010302610096565b505b5090506100d591906100d9565b5090565b61010991905b8082111561010557600081816101000a81549063ffffffff0219169055506001016100df565b5090565b90565b6101628061011b6000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680633fb588191461003e575b600080fd5b341561004957600080fd5b610051610090565b6040518082600160200280838360005b8381101561007d5780820151818401525b602081019050610061565b5050505090500191505060405180910390f35b610098610108565b60006001806020026040519081016040528092919082600180156100fd576020028201916000905b82829054906101000a900463ffffffff1663ffffffff16815260200190600401906020826003010492830192600103820291508084116100c05790505b505050505090505b90565b6020604051908101604052806001905b600063ffffffff1681526020019060019003908161011857905050905600a165627a7a72305820f1da304bd8da0e7c9401164a42f2b176afcfb4698bbf9aece7185d91fc5312720029", 1000000, 0.001)

        self.proposal_address = contract_data['address']

    def run_test(self):
        self.nodes[1].generate(1)
        self.sync_all()
        self.node.generate(1000 + COINBASE_MATURITY)
        self.sync_all()

        # Start off by setting ourself as admin
        admin_address = self.node.getnewaddress()

        # Set ourself up as admin
        self.GAS_PRICE_DGP.send_set_initial_admin(admin_address)
        self.node.generate(1)

        # restart node 1, this will disconnect the nodes so that node 1 (B) is unaware of the gas price change
        # until they connect later on
        stop_node(self.nodes[1], 1)
        self.nodes[1] = start_node(1, self.options.tmpdir)
        self.nodes[1].createcontract("00", 1000000, QTUM_MIN_GAS_PRICE/COIN)

        # Set the minimum gas price to 100
        self.create_proposal_contract(100)
        self.GAS_PRICE_DGP.send_add_address_proposal(self.proposal_address, 2, admin_address)
        self.node.generate(2) # Activate the proposal

        # Reconnect and sync the blocks of the nodes
        connect_nodes_bi(self.nodes, 0, 1)
        self.is_network_split = False
        sync_blocks(self.nodes)

        # Make sure that the nodes have the same tip
        assert_equal(self.nodes[0].getbestblockhash(), self.nodes[1].getbestblockhash())

        self.nodes[1].generate(1)


if __name__ == '__main__':
    QtumDGPGasPriceLingeringMempoolTxTest().main()
