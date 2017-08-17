#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.qtum import *
from test_framework.address import *
from test_framework.blocktools import *
import time
import io

class QtumSpendOpCallTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [[]])
        self.is_network_split = False

    def run_test(self):
        self.nodes[0].generate(10+COINBASE_MATURITY)

        # Create a new contract that can receive funds
        """
            pragma solidity ^0.4.12;

            contract Test {
                function () payable {}
            }
        """
        contract_bytecode = "60606040523415600e57600080fd5b5b603580601c6000396000f30060606040525b5b5b0000a165627a7a723058202a205a0473a338a161903e98bd0920e9c01b9ab0a8f94f8f19028c49733fb60d0029"
        first_contract_address = self.nodes[0].createcontract(contract_bytecode)['address']
        self.nodes[0].generate(1)

        # Send 100000 qtum to the contract
        self.nodes[0].sendtocontract(first_contract_address, "00", 100000)['txid']
        blockhash = self.nodes[0].generate(1)[0]
        prev_block = self.nodes[0].getblock(blockhash)

        # Extract the transaction which will be the prevout to spend the contract's funds later on
        op_call_txid = prev_block['tx'][-1]

        block = create_block(int(prev_block['hash'], 16), create_coinbase(prev_block['height']+1), prev_block['time']+1)
        block.hashStateRoot = int(prev_block['hashStateRoot'], 16)
        block.hashUTXORoot = int(prev_block['hashUTXORoot'], 16)

        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(op_call_txid, 16), 0), scriptSig=CScript([]))]
        tx.vout = [CTxOut(int(100000*COIN), scriptPubKey=CScript([OP_TRUE]))]
        block.vtx.append(tx)

        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()

        block_count = self.nodes[0].getblockcount()
        ret = self.nodes[0].submitblock(bytes_to_hex_str(block.serialize()))
        assert_equal(self.nodes[0].getblockcount(), block_count)

if __name__ == '__main__':
    QtumSpendOpCallTest().main()
