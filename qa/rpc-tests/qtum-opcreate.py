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

class OpCreateTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [['-txindex=1']]*2)
        self.is_network_split = False
        connect_nodes(self.nodes[0], 1)

    # Creates a simple contract via a raw tx
    def basic_contract_is_created_raw_tx_test(self):
        for i in range(2):
            assert(len(self.nodes[i].listcontracts()) == 0+NUM_DEFAULT_DGP_CONTRACTS)
        node = self.nodes[0]
        amount = 10*COIN

        """
        pragma solidity ^0.4.11;
        contract Example {
            function () payable {}
        }
        """
        tx = make_transaction(node,
            [self.vins.pop(-1)],
            [make_op_create_output(node, 0, 4, CScriptNum(500000), CScriptNum(QTUM_MIN_GAS_PRICE), bytes.fromhex("60606040523415600b57fe5b5b60398060196000396000f30060606040525b600b5b5b565b0000a165627a7a72305820e3bed070fd3a81dd00e02efd22d18a3b47b70860155d6063e47e1e2674fc5acb0029"))]
        )
        #node.createcontract("60606040523415600b57fe5b5b60398060196000396000f30060606040525b600b5b5b565b0000a165627a7a72305820e3bed070fd3a81dd00e02efd22d18a3b47b70860155d6063e47e1e2674fc5acb0029");
        node.sendrawtransaction(tx)
        node.generate(1)
        sync_blocks(self.nodes)
        # for i in range(2):
        #     assert(len(self.nodes[i].listcontracts()) == 1)

    # Verifies that large contracts can be deployed
    def large_contract_creation_test(self):
        node = self.nodes[0]
        """
        contract Factory {
            bytes32[] Names;
            address[] newContracts;
            function createContract (bytes32 name) {
                address newContract = new Contract(name);
                newContracts.push(newContract);
            } 
            function getName (uint i) {
                Contract con = Contract(newContracts[i]);
                Names[i] = con.Name();
            }
        }
        contract Contract {
            bytes32 public Name;
            function Contract (bytes32 name) {
                Name = name;
            }
            function () payable {}
        }
        """
        node.createcontract("606060405234610000575b61034a806100196000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680633f811b80146100495780636b8ff5741461006a575b610000565b3461000057610068600480803560001916906020019091905050610087565b005b3461000057610085600480803590602001909190505061015b565b005b60008160405160e18061023e833901808260001916600019168152602001915050604051809103906000f08015610000579050600180548060010182818154818355818115116101035781836000526020600020918201910161010291905b808211156100fe5760008160009055506001016100e6565b5090565b5b505050916000526020600020900160005b83909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550505b5050565b6000600182815481101561000057906000526020600020900160005b9054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690508073ffffffffffffffffffffffffffffffffffffffff16638052474d6000604051602001526040518163ffffffff167c0100000000000000000000000000000000000000000000000000000000028152600401809050602060405180830381600087803b156100005760325a03f1156100005750505060405180519050600083815481101561000057906000526020600020900160005b5081600019169055505b50505600606060405234610000576040516020806100e1833981016040528080519060200190919050505b80600081600019169055505b505b609f806100426000396000f30060606040523615603d576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638052474d146045575b60435b5b565b005b34600057604f606d565b60405180826000191660001916815260200191505060405180910390f35b600054815600a165627a7a723058209bd85f2ac8e941766991a59f8a0183902c8168f671bfdb430ad9eab85fd697b70029a165627a7a72305820ed128e4929006fce038bc859dd8837890e7ee8d296cd3ed30b66603a8423397e0029", 1000000, 0.000001)
        block_height = node.getblockcount()
        node.generate(1)
        sync_blocks(self.nodes)
        for i in range(2):
            assert(self.nodes[i].getblockcount() == block_height+1)
            assert(len(self.nodes[i].listcontracts()) == 2+NUM_DEFAULT_DGP_CONTRACTS)


    # Tests mining many contracts in one block
    def many_contracts_in_one_block_test(self):
        node = self.nodes[0]
        num_new_contracts = 25
        for _ in range(num_new_contracts):
            """
            pragma solidity ^0.4.10;
            contract Example {
                function () payable {}
            }
            """
            node.createcontract("60606040523415600b57fe5b5b60398060196000396000f30060606040525b600b5b5b565b0000a165627a7a7230582092926a9814888ff08700cbd86cf4ff8c50052f5fd894e794570d9551733591d60029")

        block_height = node.getblockcount()
        node.generate(1)
        sync_blocks(self.nodes)
        for i in range(2):
            assert(self.nodes[i].getblockcount() == block_height+1)
            assert(len(self.nodes[i].listcontracts(1, 10000)) == 2+num_new_contracts+NUM_DEFAULT_DGP_CONTRACTS)

    # Checks that contracts are removed if the block it was mined in was invalidated
    def contract_reorg_test(self):
        node = self.nodes[0]
        num_old_contracts = len(node.listcontracts(1, 1000))
        old_block_height = node.getblockcount()
        """
        pragma solidity ^0.4.10;
        contract Example {
            function () payable {}
        }
        """
        node.createcontract("60606040523415600b57fe5b5b60398060196000396000f30060606040525b600b5b5b565b0000a165627a7a7230582092926a9814888ff08700cbd86cf4ff8c50052f5fd894e794570d9551733591d60029")
        new_block_hash = node.generate(1)[0]
        assert_equal(node.getblockcount(), old_block_height+1)
        assert_equal(len(node.listcontracts(1, 1000)), num_old_contracts+1)
        node.invalidateblock(new_block_hash)
        assert_equal(node.getblockcount(), old_block_height)
        assert_equal(len(node.listcontracts(1, 1000)), num_old_contracts)

    def gas_limit_signedness_test(self):
        node = self.nodes[0]
        num_old_contracts = len(node.listcontracts(1, 1000))

        """
        pragma solidity ^0.4.10;
        contract Example {
            function () payable {}
        }
        """
        tx = make_transaction(node,
            [self.vins.pop(-1)],
            # changing the gas limit \xff\xff -> \xff\xff\x00 results in success.
            [make_op_create_output(node, 0, b"\x04", b"\xff\xff", 1000, bytes.fromhex("60606040523415600b57fe5b5b60398060196000396000f30060606040525b600b5b5b565b0000a165627a7a7230582092926a9814888ff08700cbd86cf4ff8c50052f5fd894e794570d9551733591d60029"))]
        )
        try:
            node.sendrawtransaction(tx)
            assert(False)
        except:
            pass


    def gas_limit_signedness_test(self):
        node = self.nodes[0]
        num_old_contracts = len(node.listcontracts(1, 1000))

        """
        pragma solidity ^0.4.10;
        contract Example {
            function () payable {}
        }
        """
        tx = make_transaction(node,
            [self.vins.pop(-1)],
            # changing the gas limit \xff\xff -> \xff\xff\x00 results in success.
            [make_op_create_output(node, 0, b"\x04", b"\xff\x4f", 1000, bytes.fromhex("60606040523415600b57fe5b5b60398060196000396000f30060606040525b600b5b5b565b0000a165627a7a7230582092926a9814888ff08700cbd86cf4ff8c50052f5fd894e794570d9551733591d60029")),
            make_op_create_output(node, 0, b"\x04", b"\xff\xff", 1000, bytes.fromhex("60606040523415600b57fe5b5b60398060196000396000f30060606040525b600b5b5b565b0000a165627a7a7230582092926a9814888ff08700cbd86cf4ff8c50052f5fd894e794570d9551733591d60029"))]
        )
        try:
            node.sendrawtransaction(tx)
            assert(False)
        except:
            pass


    def run_test(self):
        self.nodes[0].generate(COINBASE_MATURITY+40)
        self.vins = [make_vin(self.nodes[0], 10*COIN) for _ in range(10)]
        self.basic_contract_is_created_raw_tx_test()
        self.large_contract_creation_test()
        self.many_contracts_in_one_block_test()
        self.contract_reorg_test()
        self.gas_limit_signedness_test()

if __name__ == '__main__':
    OpCreateTest().main()
