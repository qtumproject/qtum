#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.address import *
from test_framework.qtum import *
import sys
import random
import time
import io

class QtumAssignMPoSFeesToGasRefundTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-lastmposheight=999999']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.node = self.nodes[0]
        privkey = byte_to_base58(hash256(struct.pack('<I', 0)), 239)
        self.node.importprivkey(privkey)

        self.node.setmocktime(int(time.time()) - 1000000)
        self.node.generatetoaddress(200 + COINBASE_MATURITY, "qSrM9K6FMhZ29Vkp8Rdk8Jp66bbfpjFETq")

        activate_mpos(self.node)
        # First setup a dummy contract
        bytecode = "60606040523415600e57600080fd5b603280601b6000396000f30060606040520000a165627a7a7230582008e466283dd617547ad26d9f100792d4f3e1ec49377f8f53ce3ba3d135beb5b30029"
        address = self.node.createcontract(bytecode)['address']
        self.node.generate(1)

        staking_prevouts = collect_prevouts(self.node, amount=20000)
        tip = self.node.getblock(self.node.getbestblockhash())
        t = (tip['time'] + 0x30) & 0xfffffff0
        self.node.setmocktime(t)
        block, block_sig_key = create_unsigned_mpos_block(self.node, staking_prevouts, nTime=t, block_fees=2102200000)
        block.hashUTXORoot = int(tip['hashUTXORoot'], 16)
        block.hashStateRoot = int(tip['hashStateRoot'], 16)

        unspents = [unspent for unspent in self.node.listunspent()[::-1] if unspent['amount'] == 20000]
        unspent = unspents.pop(0)

        tx_all_fees = CTransaction()
        tx_all_fees.vin = [CTxIn(COutPoint(int(unspent['txid'], 16), unspent['vout']))]
        tx_all_fees.vout = [CTxOut(0, scriptPubKey=CScript([OP_DUP, OP_HASH160, bytes.fromhex(p2pkh_to_hex_hash(unspent['address'])), OP_EQUALVERIFY, OP_CHECKSIG]))]
        tx_all_fees = rpc_sign_transaction(self.node, tx_all_fees)
        tx_all_fees.rehash()
        block.vtx.append(tx_all_fees)

        # Generate a dummy contract call that will steal the mpos participants' fee rewards.
        # Since the fees in the last tx was 20000 00000000 we create a tx with 40000000 gas limit and 100000 gas price
        unspent = unspents.pop(0)
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(unspent['txid'], 16), unspent['vout']))]
        tx.vout = [CTxOut(0, scriptPubKey=CScript([b"\x04", CScriptNum(40000000), CScriptNum(100000), b"\x00", bytes.fromhex(address), OP_CALL]))]
        tx = rpc_sign_transaction(self.node, tx)
        tx.rehash()
        block.vtx.append(tx)

        # We must also add the refund output to the coinstake
        block.vtx[1].vout.append(CTxOut(3997897800000, scriptPubKey=CScript([OP_DUP, OP_HASH160, bytes.fromhex(p2pkh_to_hex_hash(unspent['address'])), OP_EQUALVERIFY, OP_CHECKSIG])))
        block.vtx[1].rehash()

        block.vtx[1] = rpc_sign_transaction(self.node, block.vtx[1])
        block.hashMerkleRoot = block.calc_merkle_root()
        block.sign_block(block_sig_key)
        blockcount = self.node.getblockcount()
        print(self.node.submitblock(bytes_to_hex_str(block.serialize())))
        assert_equal(self.node.getblockcount(), blockcount)

if __name__ == '__main__':
    QtumAssignMPoSFeesToGasRefundTest().main()
