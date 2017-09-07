#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.address import *
from test_framework.qtum import *
from test_framework.blocktools import *
import sys
import random
import time
import io

def rpc_sign_transaction(node, tx):
    tx_signed_raw_hex = node.signrawtransaction(bytes_to_hex_str(tx.serialize()))['hex']
    f = io.BytesIO(hex_str_to_bytes(tx_signed_raw_hex))
    tx_signed = CTransaction()
    tx_signed.deserialize(f)
    return tx_signed

class QtumNoExecCallDisabledTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        self.is_network_split = False
        self.node = self.nodes[0]

    def run_test(self):
        self.node.generate(10 + COINBASE_MATURITY)
        """
        pragma solidity ^0.4.12;
        contract Test {
            function () payable  {}
        }
        """
        bytecode = "60606040523415600e57600080fd5b5b603580601c6000396000f30060606040525b5b5b0000a165627a7a723058208938cb174ee70dbb41b522af1feac2c7e0e252b7bc9ecb92c8d87a50c445a26c0029"
        contract_address = self.node.createcontract(bytecode)['address']
        self.node.generate(1)
        self.node.sendtocontract(contract_address, "00", 1)
        self.node.generate(1)
        tx = CTransaction()
        tx.vin = [make_vin(self.node, int(COIN+1000000))]

        tx.vout = [CTxOut(int(COIN), scriptPubKey=CScript([b"\x00", CScriptNum(0), CScriptNum(0), b"\x00", hex_str_to_bytes(contract_address), OP_CALL]))]
        tx = rpc_sign_transaction(self.node, tx)
        assert_raises(JSONRPCException, self.node.sendrawtransaction, bytes_to_hex_str(tx.serialize()))

        tx.vout = [CTxOut(int(COIN), scriptPubKey=CScript([b"\x00", CScriptNum(100000), CScriptNum(QTUM_MIN_GAS_PRICE), b"\x00", hex_str_to_bytes(contract_address), OP_CALL]))]
        tx = rpc_sign_transaction(self.node, tx)
        assert_raises(JSONRPCException, self.node.sendrawtransaction, bytes_to_hex_str(tx.serialize()))


        tip = self.node.getblock(self.node.getbestblockhash())
        block = create_block(int(tip['hash'], 16), create_coinbase(tip['height']+1), tip['time']+1)
        block.vtx.append(tx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        blockcount = self.node.getblockcount()
        self.node.submitblock(bytes_to_hex_str(block.serialize()))
        assert_equal(self.node.getblockcount(), blockcount)

        block.vtx[1].vout = [CTxOut(int(COIN), scriptPubKey=CScript([b"\x00", CScriptNum(0), CScriptNum(0), b"\x00", hex_str_to_bytes(contract_address), OP_CALL]))]
        block.vtx[1] = rpc_sign_transaction(self.node, block.vtx[1])
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        blockcount = self.node.getblockcount()
        self.node.submitblock(bytes_to_hex_str(block.serialize()))
        assert_equal(self.node.getblockcount(), blockcount)


if __name__ == '__main__':
    QtumNoExecCallDisabledTest().main()
