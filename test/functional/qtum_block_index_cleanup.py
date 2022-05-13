#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.qtum import *
from test_framework.qtumconfig import *
import sys

class QtumBlockIndexCleanupTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [['-txindex=1', '-cleanblockindextimeout=1']]*2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def calculate_stake_modifier(self, parent_block_modifier, current_block):
        data = b""
        data += ser_uint256(current_block.sha256 if current_block.prevoutStake.serialize() == COutPoint(0, 0xffffffff).serialize() else current_block.prevoutStake.hash)
        data += ser_uint256(parent_block_modifier)
        return uint256_from_str(hash256(data))


    def create_pos_block(self, staking_prevouts, parent_block, parent_block_stake_modifier, block_height, block_reward=2000400000000, start_time_addition=0x0):
        coinbase = create_coinbase(block_height)
        coinbase.vout[0].nValue = 0
        coinbase.vout[0].scriptPubKey = b""
        coinbase.rehash()
        block = create_block(parent_block.sha256, coinbase, (parent_block.nTime + 0x10+start_time_addition) & 0xfffffff0)
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
        block.rehash()
        return block

    def _remove_from_staking_prevouts(self, block, staking_prevouts):
        for j in range(len(staking_prevouts)):
            prevout = staking_prevouts[j]
            if prevout[0].serialize() == block.prevoutStake.serialize():
                staking_prevouts.pop(j)
                break

    def create_fork_chain(self, prevblock, length, is_pow=True, start_time_addition=0x0):
        tip = self.node.getblock(prevblock)
        prevhash = int(tip['hash'], 16)
        blocks = []

        if not is_pow:
            prevouts = collect_prevouts(self.node, min_confirmations=2*COINBASE_MATURITY+100)
            modifier = int(tip['modifier'], 16)
            block = CBlock()
            f = io.BytesIO(hex_str_to_bytes(self.node.getblock(tip['hash'], False)))
            block.deserialize(f)
            block.rehash()

        for i in range(length):
            if is_pow:
                coinbase = create_coinbase(tip['height']+1+i)
                block = create_block(prevhash, coinbase, tip['time']+1+i)
                block.solve()
            else:
                block = self.create_pos_block(prevouts, block, modifier, tip['height']+1+i, start_time_addition=start_time_addition)
                modifier = self.calculate_stake_modifier(modifier, block)
                self._remove_from_staking_prevouts(block, prevouts)

            blocks.append(block)
            prevhash = block.sha256
        return blocks

    def run_test(self):
        self.node = self.nodes[0]
        privkey = byte_to_base58(hash256(struct.pack('<I', 0)), 239)
        self.node.importprivkey(privkey)
        mocktime = int(time.time())-100000
        for n in self.nodes:
            n.setmocktime(mocktime)
        
        generatesynchronized(self.node, COINBASE_MATURITY, "qSrM9K6FMhZ29Vkp8Rdk8Jp66bbfpjFETq", self.nodes, mocktime)
        self.disconnect_nodes(0, 1)
        self.connect_nodes(0, 1)

        generatesynchronized(self.node, COINBASE_MATURITY, "qSrM9K6FMhZ29Vkp8Rdk8Jp66bbfpjFETq", self.nodes, mocktime)
        self.disconnect_nodes(0, 1)
        self.connect_nodes(0, 1)

        generatesynchronized(self.node, COINBASE_MATURITY, "qSrM9K6FMhZ29Vkp8Rdk8Jp66bbfpjFETq", self.nodes, mocktime)
        self.disconnect_nodes(0, 1)
        self.connect_nodes(0, 1)

        generatesynchronized(self.node, COINBASE_MATURITY, "qSrM9K6FMhZ29Vkp8Rdk8Jp66bbfpjFETq", self.nodes, mocktime)
        self.disconnect_nodes(0, 1)
        self.connect_nodes(0, 1)

        for n in self.nodes:
            n.setmocktime(0)
        self.sync_all()

        bhash = self.node.getbestblockhash()
        for n in self.nodes:
            n.invalidateblock(bhash)
        print("Make sure that it cleans up a the old main chain if a fork overtakes it. Make sure that all nodes do this")
        blocks = self.create_fork_chain(self.node.getblockhash(self.node.getblockcount()-COINBASE_MATURITY), COINBASE_MATURITY+2)
        for n in self.nodes:
            print(n.submitblock(bytes_to_hex_str(blocks[0].serialize())))
        print('before reconsider', self.node.getbestblockhash())
        for n in self.nodes:
            n.reconsiderblock(bhash)
        print('after reconsider', self.node.getbestblockhash())
        for i, block in enumerate(blocks[1:]):
            print(self.node.submitblock(bytes_to_hex_str(block.serialize())))
        print(self.node.getchaintips())
        print('syncing')
        print(bhash)
        time.sleep(2)
        for i, n in enumerate(self.nodes):
            print("checking node#" + str(i))
            print(n.getchaintips())
        self.sync_all()
        print(bhash)
        print(self.node.getchaintips())
        time.sleep(2)
        for n in self.nodes:
            assert_equal(self.node.getchaintips(), n.getchaintips())

if __name__ == '__main__':
    QtumBlockIndexCleanupTest().main()
