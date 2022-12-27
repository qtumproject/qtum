#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.address import *
from test_framework.qtum import *

class QtumDivergenceDosTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [[]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def submit_block_with_txs(self, txs):
        tip = self.node.getblock(self.node.getbestblockhash())
        block = create_block(int(tip['hash'], 16), create_coinbase(tip['height']+1), tip['time']+1)
        block.hashStateRoot = int(tip['hashStateRoot'], 16)
        block.hashUTXORoot = int(tip['hashUTXORoot'], 16)
        if txs:
            address = self.node.gettxout(hex(txs[0].vin[0].prevout.hash)[2:].zfill(64), txs[0].vin[0].prevout.n)['scriptPubKey']['address']
            haddress = hex_str_to_bytes(p2pkh_to_hex_hash(address))
            block.vtx[0].vout.append(CTxOut(100, CScript([OP_DUP, OP_HASH160, haddress, OP_EQUALVERIFY, OP_CHECKSIG])))
            if len(txs) > 1:
                txs[-1].vout[0].scriptPubKey = CScript([OP_DUP, OP_HASH160, haddress, OP_EQUALVERIFY, OP_CHECKSIG])

        block.vtx += txs
        for tx in block.vtx:
            tx.rehash()
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        assert_equal(self.node.submitblock(bytes_to_hex_str(block.serialize())), 'incorrect-transactions-or-hashes-block')


    def too_few_txs_test(self):
        # Run it many times so we can trigger out of bounds segfaults with a high probability
        tx = CTransaction()
        tx.vin = [make_vin(self.node, COIN)]
        tx.vout = [CTxOut(COIN-40000000, scriptPubKey=CScript([b"\x04", CScriptNum(100000), CScriptNum(QTUM_MIN_GAS_PRICE), hex_str_to_bytes("00"), hex_str_to_bytes(self.contract_address), OP_CALL]))]
        tx = rpc_sign_transaction(self.node, tx)
        tx.rehash()
        self.submit_block_with_txs([tx])

    def different_but_same_number_aal_txs_test(self):
        # Run it many times so we can trigger out of bounds segfaults with a high probability
        tx1 = CTransaction()
        tx1.vin = [make_vin(self.node, COIN // 10)]
        tx1.vout = [CTxOut(1, scriptPubKey=CScript([b"\x04", CScriptNum(100000), CScriptNum(QTUM_MIN_GAS_PRICE), hex_str_to_bytes("00"), hex_str_to_bytes(self.contract_address), OP_CALL]))]
        tx1 = rpc_sign_transaction(self.node, tx1)
        tx1.rehash()

        tx2 = CTransaction()
        tx2.nVersion = 2
        tx2.vin = [CTxIn(COutPoint(tx1.sha256, 0), scriptSig=CScript([OP_SPEND]), nSequence=0xffffffff)]
        tx2.vout = [CTxOut(0, scriptPubKey=CScript([OP_DUP, OP_HASH160, hex_str_to_bytes("00"*19)+b"\x01", OP_EQUALVERIFY, OP_CHECKSIG]))]
        tx2.rehash()
        self.submit_block_with_txs([tx1, tx2])


    def too_many_txs_test(self):
        # Run it many times so we can trigger out of bounds segfaults with a high probability
        tx1 = CTransaction()
        tx1.vin = [make_vin(self.node, COIN // 10)]
        tx1.vout = [CTxOut(1, scriptPubKey=CScript([b"\x04", CScriptNum(100000), CScriptNum(QTUM_MIN_GAS_PRICE), hex_str_to_bytes("00"), hex_str_to_bytes(self.contract_address), OP_CALL]))]
        tx1 = rpc_sign_transaction(self.node, tx1)
        tx1.rehash()

        tx2 = CTransaction()
        tx2.nVersion = 2
        tx2.vin = [CTxIn(COutPoint(tx1.sha256, 0), scriptSig=CScript([OP_SPEND]), nSequence=0xffffffff)]
        tx2.vout = [CTxOut(0, scriptPubKey=CScript([b"\x00", CScriptNum(0), CScriptNum(0), hex_str_to_bytes("00"), hex_str_to_bytes(self.contract_address), OP_CALL]))]
        tx2.rehash()

        tx3 = CTransaction()
        tx2.nVersion = 2
        tx3.vin = [CTxIn(COutPoint(tx2.sha256, 0), scriptSig=CScript([OP_SPEND]))]
        tx3.vout = [CTxOut(0, scriptPubKey=CScript([OP_DUP, OP_HASH160, hex_str_to_bytes("00"*19)+b"\x00", OP_EQUALVERIFY, OP_CHECKSIG]))]
        tx3.rehash()
        self.submit_block_with_txs([tx1, tx2, tx3])

    def run_test(self):
        self.node = self.nodes[0]
        self.node.generate(500+COINBASE_MATURITY)
        """
        pragma solidity ^0.4.24;
        contract Ballot {
            function() payable public {
                while(true){}
            }
        }
        """
        bytecode = "6080604052348015600f57600080fd5b50603d80601d6000396000f30060806040525b600115600f576005565b0000a165627a7a72305820046fe704d7206dd7bd828449504709b4786e72b5b8cb47633add96fec4d343410029"
        self.contract_address = self.node.createcontract(bytecode)['address']
        self.node.generate(1)
        self.too_few_txs_test()
        self.different_but_same_number_aal_txs_test()
        self.too_many_txs_test()
         
if __name__ == '__main__':
    QtumDivergenceDosTest().main()
