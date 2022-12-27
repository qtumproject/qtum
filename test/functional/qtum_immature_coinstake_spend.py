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

class QtumPrematureCoinstakeSpendTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-acceptnonstdtxn', '-lastmposheight=999999']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def remove_from_staking_prevouts(self, remove_prevout):
        for j in range(len(self.staking_prevouts)):
            prevout = self.staking_prevouts[j]
            if prevout[0].serialize() == remove_prevout.serialize():
                self.staking_prevouts.pop(j)
                break

    def assert_spend_of_coinstake_at_height(self, height, should_accept):
        spend_block = self.node.getblock(self.node.getblockhash(height))
        spend_coinstake_txid = spend_block['tx'][1]
        spend_coinstake_txout = self.node.gettxout(spend_coinstake_txid, 1)

        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(spend_coinstake_txid, 16), 1))]
        tx.vout = [CTxOut(int(float(str(spend_coinstake_txout['value']))*COIN - 1000000), scriptPubKey=CScript([OP_TRUE]))]
        tx = rpc_sign_transaction(self.node, tx)
        
        if should_accept:
            self.node.sendrawtransaction(bytes_to_hex_str(tx.serialize()))
        else:
            assert_raises_rpc_error(-26, "bad-txns-premature-spend-of-coinbase", self.node.sendrawtransaction, bytes_to_hex_str(tx.serialize()))

        tip = self.node.getblock(self.node.getbestblockhash())

        next_block_time = (tip['time'] + 0x30) & 0xfffffff0
        self.node.setmocktime(next_block_time)
        block, sig_key = create_unsigned_mpos_block(self.node, self.staking_prevouts, next_block_time, 1000000)
        block.vtx.append(tx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.sign_block(sig_key)
        blockcount = self.node.getblockcount()
        self.node.submitblock(bytes_to_hex_str(block.serialize()))
        assert_equal(self.node.getblockcount(), blockcount + (1 if should_accept else 0))
        self.remove_from_staking_prevouts(block.prevoutStake)


    def run_test(self):
        privkey = byte_to_base58(hash256(struct.pack('<I', 0)), 239)
        for n in self.nodes:
            n.importprivkey(privkey)

        self.node = self.nodes[0]
        self.node.setmocktime(int(time.time()) - 1000000)
        generatesynchronized(self.node, 10 + COINBASE_MATURITY, "qSrM9K6FMhZ29Vkp8Rdk8Jp66bbfpjFETq", self.nodes)
        # These are the privkeys that corresponds to the pubkeys in the pos outputs
        # These are used by default by create_pos_block
        for i in range(0xff+1):
            privkey = byte_to_base58(hash256(struct.pack('<I', i)), 239)
            self.node.importprivkey(privkey)

        activate_mpos(self.node)

        self.staking_prevouts = collect_prevouts(self.node)
        self.assert_spend_of_coinstake_at_height(height=5002-COINBASE_MATURITY, should_accept=False)
        self.assert_spend_of_coinstake_at_height(height=5001-COINBASE_MATURITY, should_accept=True)
        # Invalidate the last block and make sure that the previous rejection of the premature coinstake spends fails
        self.node.invalidateblock(self.node.getbestblockhash())
        self.assert_spend_of_coinstake_at_height(height=5002-COINBASE_MATURITY, should_accept=False)

if __name__ == '__main__':
    QtumPrematureCoinstakeSpendTest().main()
