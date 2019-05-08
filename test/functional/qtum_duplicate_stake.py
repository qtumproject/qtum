#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.messages import *
from test_framework.qtum import *
import time


class QtumDuplicateStakeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def start_p2p_connection(self):
        self.p2p_node = self.node.add_p2p_connection(P2PInterface())
        self.p2p_alt_node = self.nodes[1].add_p2p_connection(P2PInterface())

    def verify_duplicate_stakes_are_accepted_test(self):
        tip = self.node.getblock(self.node.getbestblockhash())
        t = (tip['time']+0x10) & 0xfffffff0
        # Create one "normal" block
        block, block_sig_key = create_unsigned_pos_block(self.node, self.staking_prevouts, nTime=t)
        block.sign_block(block_sig_key)
        block.rehash()

        # Create a slightly different block using the same staking utxo (only difference is the nonce)
        alt_block = CBlock(block)
        alt_block.vtx = block.vtx[:]
        alt_block.nNone = 1
        alt_block.rehash()
        alt_block.sign_block(block_sig_key)
        alt_block.rehash()

        # Send <block> to node
        self.p2p_node.send_message(msg_block(block))
        # Send <alt_block> to alt_node
        self.p2p_alt_node.send_message(msg_block(alt_block))
        time.sleep(2)

        assert_equal(self.node.getbestblockhash(), block.hash)
        assert_equal(self.alt_node.getbestblockhash(), alt_block.hash)

        # Build a longer chain on alt_node
        self.alt_node.generate(1)
        self.sync_all()

    def verify_spent_stake_is_accepted_in_fork_test(self):
        tip = self.node.getblock(self.node.getbestblockhash())
        t = (tip['time']+0x10) & 0xfffffff0
        # Create one "normal" block
        block, block_sig_key = create_unsigned_pos_block(self.node, self.staking_prevouts, nTime=t)
        block.sign_block(block_sig_key)
        block.rehash()

        # Create a different block that spends the prevoutStake from <block>
        alt_block, alt_block_sig_key = create_unsigned_pos_block(self.alt_node, self.alt_staking_prevouts, nTime=t)
        tx = CTransaction()
        tx.vin = [CTxIn(block.prevoutStake)]
        tx.vout = [CTxOut(int(COIN), scriptPubKey=CScript([OP_TRUE]))]
        tx = rpc_sign_transaction(self.node, tx)
        alt_block.vtx.append(tx)
        alt_block.hashMerkleRoot = alt_block.calc_merkle_root()
        alt_block.rehash()
        alt_block.sign_block(alt_block_sig_key)
        alt_block.rehash()

        # Send <alt_block> to alt_node
        self.p2p_alt_node.send_message(msg_block(alt_block))
        # Send <block> to node
        self.p2p_node.send_message(msg_block(block))
        time.sleep(2)

        assert_equal(self.node.getbestblockhash(), block.hash)
        assert_equal(self.alt_node.getbestblockhash(), alt_block.hash)
        # Build a longer chain on alt_node
        self.alt_node.generate(1)
        self.sync_all()

    def verify_spent_stake_in_old_block_is_rejected_test(self):
        tip = self.node.getblock(self.node.getbestblockhash())
        t = (tip['time']+0x10) & 0xfffffff0
        # Create one "normal" block
        block, block_sig_key = create_unsigned_pos_block(self.node, self.staking_prevouts, nTime=t)
        block.sign_block(block_sig_key)
        block.rehash()

        # Create a different block that spends the prevoutStake from <block>
        alt_block, alt_block_sig_key = create_unsigned_pos_block(self.alt_node, self.alt_staking_prevouts, nTime=t)
        tx = CTransaction()
        tx.vin = [CTxIn(block.prevoutStake)]
        tx.vout = [CTxOut(int(COIN), scriptPubKey=CScript([OP_TRUE]))]
        tx = rpc_sign_transaction(self.node, tx)
        alt_block.vtx.append(tx)
        alt_block.hashMerkleRoot = alt_block.calc_merkle_root()
        alt_block.rehash()
        alt_block.sign_block(alt_block_sig_key)
        alt_block.rehash()

        # Send <alt_block> to alt_node
        self.p2p_alt_node.send_message(msg_block(alt_block))
        time.sleep(2)
        self.alt_node.generate(1)
        time.sleep(2)
        
        # Send <block> to node
        self.p2p_node.send_message(msg_block(block))
        time.sleep(2)
        assert_raises_rpc_error(-5, "Block not found", self.node.getblockheader, block.hash)

        time.sleep(2)
        self.sync_all()

    def run_test(self):
        self.node = self.nodes[0]
        self.alt_node = self.nodes[1]
        self.node.setmocktime(int(time.time() - 100*24*60*60))
        self.alt_node.setmocktime(int(time.time() - 100*24*60*60))
        self.alt_node.generate(50)
        self.sync_all()
        self.node.generate(550)
        self.sync_all()
        self.staking_prevouts = collect_prevouts(self.node)
        self.alt_staking_prevouts = collect_prevouts(self.alt_node)
        self.node.setmocktime(0)
        self.alt_node.setmocktime(0)
        self.start_p2p_connection()

        time.sleep(0x10)

        self.verify_duplicate_stakes_are_accepted_test()
        assert_equal(self.node.getblockcount(), self.alt_node.getblockcount())
        assert_equal(self.node.getbestblockhash(), self.alt_node.getbestblockhash())

        time.sleep(0x10)

        self.staking_prevouts = collect_prevouts(self.node)
        self.alt_staking_prevouts = collect_prevouts(self.alt_node)
        self.verify_spent_stake_is_accepted_in_fork_test()
        assert_equal(self.node.getblockcount(), self.alt_node.getblockcount())
        assert_equal(self.node.getbestblockhash(), self.alt_node.getbestblockhash())

        time.sleep(0x10)

        self.staking_prevouts = collect_prevouts(self.node)
        self.alt_staking_prevouts = collect_prevouts(self.alt_node)
        self.verify_spent_stake_in_old_block_is_rejected_test()


if __name__ == '__main__':
    QtumDuplicateStakeTest().main()
