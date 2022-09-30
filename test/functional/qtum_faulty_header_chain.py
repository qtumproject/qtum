#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.messages import *
from test_framework.qtum import *
import time
import io


class QtumHeaderSpamTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def start_p2p_connection(self):
        self.p2p_node = self.node.add_p2p_connection(P2PInterface())

    def _remove_from_staking_prevouts(self, block):
        for j in range(len(self.staking_prevouts)):
            prevout = self.staking_prevouts[j]
            if prevout[0].serialize() == block.prevoutStake.serialize():
                self.staking_prevouts.pop(j)
                break

    def sign_block_with_standard_private_key(self, block):
        block_sig_key = ECKey()
        block_sig_key.set(hash256(struct.pack('<I', 0)), False)
        block.sign_block(block_sig_key)

    def create_pos_block(self, staking_prevouts, parent_block, parent_block_stake_modifier, block_height, block_reward=2000400000000):
        coinbase = create_coinbase(block_height+1)
        coinbase.vout[0].nValue = 0
        coinbase.vout[0].scriptPubKey = b""
        coinbase.rehash()
        block = create_block(parent_block.sha256, coinbase, (parent_block.nTime + 0x10) & 0xfffffff0)
        if not block.solve_stake(parent_block_stake_modifier, staking_prevouts):
            return None

        # create a new private key used for block signing.
        block_sig_key = ECKey()
        block_sig_key.set(hash256(struct.pack('<I', 0)), False)
        pubkey = block_sig_key.get_pubkey().get_bytes()
        scriptPubKey = CScript([pubkey, OP_CHECKSIG])
        stake_tx_unsigned = CTransaction()

        stake_tx_unsigned.vin.append(CTxIn(block.prevoutStake))
        stake_tx_unsigned.vout.append(CTxOut())
        stake_tx_unsigned.vout.append(CTxOut(2000400000000, scriptPubKey))

        stake_tx_signed_raw_hex = self.node.signrawtransactionwithwallet(bytes_to_hex_str(stake_tx_unsigned.serialize()))['hex']
        f = io.BytesIO(hex_str_to_bytes(stake_tx_signed_raw_hex))
        stake_tx_signed = CTransaction()
        stake_tx_signed.deserialize(f)
        block.vtx.append(stake_tx_signed)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.sign_block(block_sig_key)
        return block

    def calculate_stake_modifier(self, parent_block_modifier, current_block):
        data = b""
        data += ser_uint256(current_block.sha256 if current_block.prevoutStake.serialize() == COutPoint(0, 0xffffffff).serialize() else current_block.prevoutStake.hash)
        data += ser_uint256(parent_block_modifier)
        return uint256_from_str(hash256(data))

    def run_test(self):
        self.node = self.nodes[0]
        self.alt_node = self.nodes[1]
        privkey = byte_to_base58(hash256(struct.pack('<I', 0)), 239)
        self.node.importprivkey(privkey)
        self.start_p2p_connection()
        self.node.setmocktime(int(time.time())-10000)
        generatesynchronized(self.node, 100+COINBASE_MATURITY, "qSrM9K6FMhZ29Vkp8Rdk8Jp66bbfpjFETq", self.nodes)
        self.sync_all()
        self.disconnect_nodes(0, 1)
    
        self.node.setmocktime(0)
        self.staking_prevouts = collect_prevouts(self.node)
        tip = self.node.getblock(self.node.getbestblockhash())
        stake_modifier = int(tip['modifier'], 16)
        block_height = tip['height']
        block_raw_hex = self.node.getblock(self.node.getbestblockhash(), False)
        f = io.BytesIO(hex_str_to_bytes(block_raw_hex))
        block = CBlock()
        block.deserialize(f)
        block.rehash()

        # Make sure that first sending a header and then announcing its block succeeds
        block = self.create_pos_block(self.staking_prevouts, block, stake_modifier, block_height)
        self._remove_from_staking_prevouts(block)
        block.rehash()
        self._remove_from_staking_prevouts(block)
        self.p2p_node.send_message(msg_headers([CBlockHeader(block)]))
        time.sleep(0.05)
        assert(self.node.getblockheader(block.hash))
        assert_raises_rpc_error(-1, "Block not found on disk", self.node.getblock, block.hash)
        self.p2p_node.send_message(msg_block(block))
        time.sleep(0.05)
        assert(self.node.getblockheader(block.hash))
        assert(self.node.getblock(block.hash))

        # Make sure that the identical block with only a modified nonce (i.e. the same prevoutStake but different block hash is not accepted)
        block.nNonce += 1
        self.sign_block_with_standard_private_key(block)
        self.p2p_node.send_message(msg_headers([CBlockHeader(block)]))
        time.sleep(0.05)
        block.rehash()
        assert_raises_rpc_error(-5, "Block not found", self.node.getblockheader, block.hash)
        assert_raises_rpc_error(-5, "Block not found", self.node.getblock, block.hash)

        # Make sure that the chain is still reorgable if presented with a longer chain
        blocks = [block]
        child_stake_modifier = self.calculate_stake_modifier(stake_modifier, block)
        child_block = self.create_pos_block(self.staking_prevouts, block, child_stake_modifier, block_height+1)
        self._remove_from_staking_prevouts(child_block)
        child_block.rehash()
        print(self.alt_node.submitblock(bytes_to_hex_str(block.serialize())))
        print(self.alt_node.submitblock(bytes_to_hex_str(child_block.serialize())))
        self.node.setmocktime(child_block.nTime-16)
        self.connect_nodes(0, 1)
        time.sleep(1)
        self.node.setmocktime(0)
        self.alt_node.generate(1)
        self.sync_all()
        

if __name__ == '__main__':
    QtumHeaderSpamTest().main()
