#!/usr/bin/env python3
import copy
import io
import time
from test_framework.blocktools import create_block, create_coinbase, create_tx_with_script
from test_framework.messages import COIN, CBlock, msg_block
from test_framework.mininode import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.qtum import *

class QtumHeaderOnlySignatureTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [[], []]

    def remove_used_staking_prevout(self, staking_prevouts, block):
        for j in range(len(staking_prevouts)):
            prevout = staking_prevouts[j]
            if prevout[0].serialize() == block.prevoutStake.serialize():
                staking_prevouts.pop(j)
                return prevout


    def run_test(self):
        synced_node = self.nodes[0]
        unsynced_node = self.nodes[1]
        privkey = byte_to_base58(hash256(struct.pack('<I', 0)), 239)
        t = int(time.time() - 10000)
        for n in self.nodes:
            n.importprivkey(privkey)
            n.setmocktime(t)
        synced_node.generatetoaddress(600+COINBASE_MATURITY, "qSrM9K6FMhZ29Vkp8Rdk8Jp66bbfpjFETq")
        staking_prevouts = collect_prevouts(synced_node)
        used_staking_prevouts = []
        self.sync_all()
        for n in self.nodes:
            n.setmocktime(0)
        disconnect_nodes(synced_node, 1)
        for _ in range(501):
            tip = synced_node.getblock(synced_node.getbestblockhash())
            block, block_sig_key = create_unsigned_pos_block(synced_node, staking_prevouts, nTime=(tip['time']&0xfffffff0)+0x10)
            used_prevout = self.remove_used_staking_prevout(staking_prevouts, block)
            used_staking_prevouts.append(used_prevout)
            block.sign_block(block_sig_key)
            assert_equal(synced_node.submitblock(bytes_to_hex_str(block.serialize())), None)

        # A block created more than 500 blocks in the past from the chain tip should be rejected
        block, block_sig_key = create_unsigned_pos_block(unsynced_node, used_staking_prevouts, nTime=(tip['time']&0xfffffff0))
        block.sign_block(block_sig_key)
        assert_equal(synced_node.submitblock(bytes_to_hex_str(block.serialize())), "invalid")

        # sync one block from synced_node to unsynced_node and try again
        assert_equal(unsynced_node.submitblock(synced_node.getblock(synced_node.getblockhash(600+COINBASE_MATURITY+1), False)), None)
        block, block_sig_key = create_unsigned_pos_block(unsynced_node, used_staking_prevouts[1:], nTime=(tip['time']&0xfffffff0))
        block.sign_block(block_sig_key)
        assert_equal(synced_node.submitblock(bytes_to_hex_str(block.serialize())), "inconclusive")

if __name__ == '__main__':
    QtumHeaderOnlySignatureTest().main()
