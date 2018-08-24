#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import ComparisonTestFramework
from test_framework.comptool import TestManager, TestInstance, RejectResult
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.blocktools import *
from test_framework.key import CECKey
import io
import struct

class QtumPOSTest(ComparisonTestFramework):
    def __init__(self):
        super().__init__()
        self.num_nodes = 1
        self.tip = None

    def add_options(self, parser):
        super().add_options(parser)
        parser.add_option("--runbarelyexpensive", dest="runbarelyexpensive", default=True)

    def run_test(self):
        self.test = TestManager(self, self.options.tmpdir)
        self.test.add_all_connections(self.nodes)
        NetworkThread().start() # Start up network handling in another thread
        self.test.run()

    def create_unsigned_pos_block(self, staking_prevouts, nTime=None, outNValue=10002, signStakeTx=True, bestBlockHash=None, coinStakePrevout=None):
        if not nTime:
            current_time = int(time.time()) + 15
            nTime = current_time & 0xfffffff0

        if not bestBlockHash:
            bestBlockHash = self.node.getbestblockhash()
            block_height = self.node.getblockcount()
        else:
            block_height = self.node.getblock(bestBlockHash)['height']

        parent_block_stake_modifier = int(self.node.getblock(bestBlockHash)['modifier'], 16)
        parent_block_raw_hex = self.node.getblock(bestBlockHash, False)
        f = io.BytesIO(hex_str_to_bytes(parent_block_raw_hex))
        parent_block = CBlock()
        parent_block.deserialize(f)
        coinbase = create_coinbase(block_height+1)
        coinbase.vout[0].nValue = 0
        coinbase.vout[0].scriptPubKey = b""
        coinbase.rehash()
        block = create_block(int(bestBlockHash, 16), coinbase, nTime)
        block.hashPrevBlock = int(bestBlockHash, 16)
        if not block.solve_stake(parent_block_stake_modifier, staking_prevouts):
            return None

        # create a new private key used for block signing.
        block_sig_key = CECKey()
        block_sig_key.set_secretbytes(hash256(struct.pack('<I', 0xffff)))
        pubkey = block_sig_key.get_pubkey()
        scriptPubKey = CScript([pubkey, OP_CHECKSIG])
        stake_tx_unsigned = CTransaction()

        if not coinStakePrevout:
            coinStakePrevout = block.prevoutStake

        stake_tx_unsigned.vin.append(CTxIn(coinStakePrevout))
        stake_tx_unsigned.vout.append(CTxOut())
        stake_tx_unsigned.vout.append(CTxOut(int(outNValue*COIN), scriptPubKey))
        stake_tx_unsigned.vout.append(CTxOut(int(outNValue*COIN), scriptPubKey))

        if signStakeTx:
            stake_tx_signed_raw_hex = self.node.signrawtransaction(bytes_to_hex_str(stake_tx_unsigned.serialize()))['hex']
            f = io.BytesIO(hex_str_to_bytes(stake_tx_signed_raw_hex))
            stake_tx_signed = CTransaction()
            stake_tx_signed.deserialize(f)
            block.vtx.append(stake_tx_signed)
        else:
            block.vtx.append(stake_tx_unsigned)
        block.hashMerkleRoot = block.calc_merkle_root()
        return (block, block_sig_key)


    def get_tests(self):
        self.node = self.nodes[0]
        # returns a test case that asserts that the current tip was accepted
        def accepted():
            return TestInstance([[self.tip, True]])

        # returns a test case that asserts that the current tip was rejected
        def rejected(reject = None):
            if reject is None:
                return TestInstance([[self.tip, False]])
            else:
                return TestInstance([[self.tip, reject]])

        # First generate some blocks so we have some spendable coins
        block_hashes = self.node.generate(25)

        for i in range(COINBASE_MATURITY):
            self.tip = create_block(int(self.node.getbestblockhash(), 16), create_coinbase(self.node.getblockcount()+1), int(time.time()))
            self.tip.solve()
            yield accepted()

        for _ in range(10):
            self.node.sendtoaddress(self.node.getnewaddress(), 1000)
        block_hashes += self.node.generate(1)

        blocks = []
        for block_hash in block_hashes:
            blocks.append(self.node.getblock(block_hash))


        # These are our staking txs
        self.staking_prevouts = []
        self.bad_vout_staking_prevouts = []
        self.bad_txid_staking_prevouts = []
        self.unconfirmed_staking_prevouts = []

        for unspent in self.node.listunspent():
            for block in blocks:
                if unspent['txid'] in block['tx']:
                    tx_block_time = block['time']
                    break
            else:
                assert(False)

            if unspent['confirmations'] > COINBASE_MATURITY:
                self.staking_prevouts.append((COutPoint(int(unspent['txid'], 16), unspent['vout']), int(unspent['amount'])*COIN, tx_block_time))
                self.bad_vout_staking_prevouts.append((COutPoint(int(unspent['txid'], 16), 0xff), int(unspent['amount'])*COIN, tx_block_time))
                self.bad_txid_staking_prevouts.append((COutPoint(int(unspent['txid'], 16)+1, unspent['vout']), int(unspent['amount'])*COIN, tx_block_time))


            if unspent['confirmations'] < COINBASE_MATURITY:
                self.unconfirmed_staking_prevouts.append((COutPoint(int(unspent['txid'], 16), unspent['vout']), int(unspent['amount'])*COIN, tx_block_time))




        # First let 25 seconds pass so that we do not submit blocks directly after the last one
        #time.sleep(100)
        block_count = self.node.getblockcount()


        # 1 A block that does not have the correct timestamp mask
        t = int(time.time()) | 1
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts, nTime=t)
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()


        # 2 A block that with a too high reward
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts, outNValue=30006)
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()


        # 3 A block with an incorrect block sig
        bad_key = CECKey()
        bad_key.set_secretbytes(hash256(b'horse staple battery'))
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts)
        self.tip.sign_block(bad_key)
        self.tip.rehash()
        yield rejected()


        # 4 A block that stakes with txs with too few confirmations
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.unconfirmed_staking_prevouts)
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()


        # 5 A block that with a coinbase reward
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts)
        self.tip.vtx[0].vout[0].nValue = 1
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()


        # 6 A block that with no vout in the coinbase
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts)
        self.tip.vtx[0].vout = []
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()


        # 7 A block way into the future
        t = (int(time.time())+100) & 0xfffffff0
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts, nTime=t)
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()


        # 8 No vout in the staking tx
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts)
        self.tip.vtx[1].vout = []
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()


        # 9 Unsigned coinstake.
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts, signStakeTx=False)
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()
        

        # 10 A block without a coinstake tx.
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts)
        self.tip.vtx.pop(-1)
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()


        # 11 A block without a coinbase.
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts)
        self.tip.vtx.pop(0)
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()


        # 12 A block where the coinbase has no outputs
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts)
        self.tip.vtx[0].vout = []
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()


        # 13 A block where the coinstake has no outputs
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts)
        self.tip.vtx[1].vout.pop(-1)
        self.tip.vtx[1].vout.pop(-1)
        stake_tx_signed_raw_hex = self.node.signrawtransaction(bytes_to_hex_str(self.tip.vtx[1].serialize()))['hex']
        f = io.BytesIO(hex_str_to_bytes(stake_tx_signed_raw_hex))
        self.tip.vtx[1] = CTransaction()
        self.tip.vtx[1].deserialize(f)
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()


        # 14 A block with an incorrect hashStateRoot
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts)
        self.tip.hashStateRoot = 0xe
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()


        # 15 A block with an incorrect hashUTXORoot
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts)
        self.tip.hashUTXORoot = 0xe
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()


        # 16 A block with an a signature on wrong header data
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts)
        self.tip.sign_block(block_sig_key)
        self.tip.nNonce = 0xfffe
        self.tip.rehash()
        yield rejected()

        # 17 A block with where the pubkey of the second output of the coinstake has been modified after block signing
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts)
        scriptPubKey = self.tip.vtx[1].vout[1].scriptPubKey
        # Modify a byte of the pubkey
        self.tip.vtx[1].vout[1].scriptPubKey = scriptPubKey[0:20] + bytes.fromhex(hex(ord(scriptPubKey[20:21])+1)[2:4]) + scriptPubKey[21:] 
        assert_equal(len(scriptPubKey), len(self.tip.vtx[1].vout[1].scriptPubKey))
        stake_tx_signed_raw_hex = self.node.signrawtransaction(bytes_to_hex_str(self.tip.vtx[1].serialize()))['hex']
        f = io.BytesIO(hex_str_to_bytes(stake_tx_signed_raw_hex))
        self.tip.vtx[1] = CTransaction()
        self.tip.vtx[1].deserialize(f)
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()

        # 18. A block in the past
        t = (int(time.time())-700) & 0xfffffff0
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts, nTime=t)
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()


        # 19. A block with too many coinbase vouts
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts)
        self.tip.vtx[0].vout.append(CTxOut(0, CScript([OP_TRUE])))
        self.tip.vtx[0].rehash()
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()


        # 20. A block where the coinstake's vin is not the prevout specified in the block
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts, coinStakePrevout=self.staking_prevouts[-1][0])
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()


        # 21. A block that stakes with valid txs but invalid vouts
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.bad_vout_staking_prevouts)
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()


        # 22. A block that stakes with txs that do not exist
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.bad_txid_staking_prevouts)
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()


        # Make sure for certain that no blocks were accepted. (This is also to make sure that no segfaults ocurred)
        assert_equal(self.node.getblockcount(), block_count)

        # And at last, make sure that a valid pos block is accepted
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts)
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield accepted()
        assert_equal(self.node.getblockcount(), block_count+1)

if __name__ == '__main__':
    QtumPOSTest().main()
