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
import sys
import time
import subprocess
import os


def get_dir_size(path):
    size = sum(os.path.getsize(path+"/"+f) for f in os.listdir(path) if os.path.isfile(path+"/"+f))
    print(path + " directory size: " + str(size) + " bytes")
    return size


class QtumHeaderSpamTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [['-headerspamfilter=1']]*2

    def add_options(self, parser):
        parser.add_argument("--dos-same-height", dest="dos_same_height", default=False, action="store_true",
                            help="Run the DoS same height test")
        parser.add_argument("--dos-variable-height", dest="dos_variable_height", default=False, action="store_true",
                            help="Run the DoS variable height test")
        parser.add_argument("--run-standard-tests", dest="run_standard_tests", default=False, action="store_true",
                            help="Run all other tests")


    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def _remove_from_staking_prevouts(self, remove_prevout):
        for j in range(len(self.staking_prevouts)):
            prevout = self.staking_prevouts[j]
            if prevout[0].serialize() == remove_prevout.serialize():
                self.staking_prevouts.pop(j)
                break

    def _create_pos_block(self, node, staking_prevouts, prevBlockHash=None, nTime=None, tip=None, nNonce=0):
        if not tip:
            tip = node.getblock(prevBlockHash if prevBlockHash else node.getbestblockhash())

        if not nTime:
            current_time = tip['time'] + 16
            nTime = current_time & 0xfffffff0

        parent_block_stake_modifier = int(tip['modifier'], 16)
        coinbase = create_coinbase(tip['height']+1)
        coinbase.vout[0].nValue = 0
        coinbase.vout[0].scriptPubKey = b""
        coinbase.rehash()
        block = create_block(int(tip['hash'], 16), coinbase, nTime)
        block.nNonce = nNonce
        block.hashStateRoot = int(tip['hashStateRoot'], 16)
        block.hashUTXORoot = int(tip['hashUTXORoot'], 16)
        if not block.solve_stake(parent_block_stake_modifier, staking_prevouts):
            return None

        txout = node.gettxout(hex(block.prevoutStake.hash)[2:].zfill(64), block.prevoutStake.n)
        # input value + block reward
        out_value = int((float(str(txout['value'])) + INITIAL_BLOCK_REWARD) * COIN) // 2

        # create a new private key used for block signing.
        block_sig_key = ECKey()
        block_sig_key.set(hash256(struct.pack('<I', 0)), False)
        pubkey = block_sig_key.get_pubkey().get_bytes()
        scriptPubKey = CScript([pubkey, OP_CHECKSIG])
        stake_tx_unsigned = CTransaction()

        stake_tx_unsigned.vin.append(CTxIn(block.prevoutStake))
        stake_tx_unsigned.vout.append(CTxOut())

        # Split the output value into two separate txs
        stake_tx_unsigned.vout.append(CTxOut(int(out_value), scriptPubKey))
        stake_tx_unsigned.vout.append(CTxOut(int(out_value), scriptPubKey))

        stake_tx_signed_raw_hex = node.signrawtransactionwithwallet(bytes_to_hex_str(stake_tx_unsigned.serialize()))['hex']
        f = io.BytesIO(hex_str_to_bytes(stake_tx_signed_raw_hex))
        stake_tx_signed = CTransaction()
        stake_tx_signed.deserialize(f)
        block.vtx.append(stake_tx_signed)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.sign_block(block_sig_key)
        return block, block_sig_key

    def start_p2p_connection(self):
        self.restart_node(0)
        time.sleep(1)
        self.p2p_node = self.node.add_p2p_connection(P2PInterface())

    def close_p2p_connection(self):
        self.node.disconnect_p2ps()

    def _create_pos_header(self, node, staking_prevouts, prevBlockHash, nTime=None, nNonce=0):
        return CBlockHeader(self._create_pos_block(node, staking_prevouts, prevBlockHash, nTime, nNonce=nNonce)[0])

    def assert_chain_tip_rejected(self, header_hash):
        tips = self.node.getchaintips()
        for tip in tips:
            assert(tip['hash'] != header_hash)

    def assert_chain_tip_accepted(self, header_hash):
        tips = self.node.getchaintips()
        is_found = False
        for tip in tips:
            if tip['hash'] == header_hash:
                return
        assert(False)

    def cannot_submit_header_before_rolling_checkpoint_test(self):
        block_header = self._create_pos_header(self.node, self.staking_prevouts, self.node.getblockhash(self.node.getblockcount()-(COINBASE_MATURITY+1)), int(time.time())&0xfffffff0)
        block_header.rehash()
        msg = msg_headers()
        msg.headers.extend([block_header])
        self.p2p_node.send_message(msg)
        time.sleep(1)
        self.assert_chain_tip_rejected(block_header.hash)
        self._remove_from_staking_prevouts(block_header.prevoutStake)

    def can_submit_header_after_rolling_checkpoint_test(self):
        block_header = self._create_pos_header(self.node, self.staking_prevouts, self.node.getblockhash(self.node.getblockcount()-COINBASE_MATURITY))
        block_header.rehash()
        msg = msg_headers()
        msg.headers.extend([block_header])
        self.p2p_node.send_message(msg)
        time.sleep(1)
        self.assert_chain_tip_accepted(block_header.hash)
        self._remove_from_staking_prevouts(block_header.prevoutStake)

    def cannot_submit_header_oversized_signature_test(self):
        block_header = self._create_pos_header(self.node, self.staking_prevouts, self.node.getblockhash(self.node.getblockcount()-COINBASE_MATURITY))
        block_header.vchBlockSig = b'x'*7999812
        block_header.rehash()
        msg = msg_headers()
        msg.headers.extend([block_header])
        self.p2p_node.send_message(msg)
        time.sleep(1)
        self.assert_chain_tip_rejected(block_header.hash)
        self._remove_from_staking_prevouts(block_header.prevoutStake)

    def cannot_submit_invalid_prevout_test(self):
        block_header = self._create_pos_header(self.node, self.staking_prevouts, self.node.getblockhash(self.node.getblockcount()-COINBASE_MATURITY))
        block_header.prevoutStake.n = 0xffff
        block_header.rehash()
        msg = msg_headers()
        msg.headers.extend([block_header])
        self.p2p_node.send_message(msg)
        time.sleep(1)
        self.assert_chain_tip_rejected(block_header.hash)
        self._remove_from_staking_prevouts(block_header.prevoutStake)

    # Constant height header spam cause ban (in our case disconnect) after a max of COINBASE_MATURITY headers
    def dos_protection_triggered_via_spam_on_same_height_test(self):
        self.start_p2p_connection()

        prevblock = self.node.getblock(self.node.getblockhash(self.node.getblockcount()-COINBASE_MATURITY))
        t = prevblock['time'] & 0xfffffff0
        for i in range((COINBASE_MATURITY+1)):
            block_header = self._create_pos_header(self.node, self.staking_prevouts, prevblock['hash'], nTime=t+0x10*i, nNonce=i)
            block_header.rehash()
            msg = msg_headers()
            msg.headers.extend([block_header])
            self.p2p_node.send_message(msg)
        self.p2p_node.wait_for_disconnect(timeout=5)

    # Variable height header spam cause ban (in our case disconnect) after a max of 1504(?) headers
    def dos_protection_triggered_via_spam_on_variable_height_test(self):
        self.start_p2p_connection()
        self.node.setmocktime(int(time.time())+1000000)
        t = (int(time.time()) + 10000) & 0xfffffff0

        for i in range(int(COINBASE_MATURITY*4.1+5)):
            prevblock = self.node.getblock(self.node.getblockhash(self.node.getblockcount()-COINBASE_MATURITY+(i%COINBASE_MATURITY)))
            block_header = self._create_pos_header(self.node, self.staking_prevouts, prevblock['hash'], nTime=t+0x10*i, nNonce=i)
            block_header.rehash()
            msg = msg_headers()
            msg.headers.extend([block_header])
            self.p2p_node.send_message(msg)

        self.p2p_node.wait_for_disconnect(timeout=5)
        self.node.setmocktime(0)

    
    def can_sync_after_offline_period_test(self):
        self.connect_nodes(0, 1)
        self.sync_blocks()
        assert_equal(self.node.getblockchaininfo()['initialblockdownload'], False)
        assert_equal(self.nodes[1].getblockchaininfo()['initialblockdownload'], False)
        self.disconnect_nodes(0, 1)
        block_time = int(time.time() - 10*24*60*60) & 0xfffffff0
        for i in range(COINBASE_MATURITY+2):
            block, block_sig_key = create_unsigned_pos_block(self.node, self.staking_prevouts, block_time)
            block.sign_block(block_sig_key)
            self.node.submitblock(bytes_to_hex_str(block.serialize()))
            block_time += 0x10
            self._remove_from_staking_prevouts(block.prevoutStake)

            if i % 100 == 0:
                self.staking_prevouts = collect_prevouts(self.node)
        self.connect_nodes(0, 1)
        self.sync_blocks()

    def reorg_requires_more_chainwork_test(self):
        old_size = get_dir_size(self.node.datadir+'/regtest/blocks/')
        self.start_p2p_connection()

        tip = self.node.getblock(self.node.getblockhash(self.node.getblockcount() - 20))
        blocks = []
        recent_unspents = [unspent for unspent in self.node.listunspent() if unspent['confirmations'] < 2*COINBASE_MATURITY and unspent['confirmations'] > COINBASE_MATURITY+40]

        for i in range(20):
            block, block_sig_key = self._create_pos_block(self.node, self.staking_prevouts, tip=tip)
            for j in range(19):
                unspent = recent_unspents.pop(0)
                tx = CTransaction()
                tx.vin = [CTxIn(COutPoint(int(unspent['txid'], 16), unspent['vout']))]
                tx.vout = [CTxOut(0, CScript([OP_TRUE]*100*1000))]
                tx = rpc_sign_transaction(self.node, tx)
                block.vtx.append(tx)
            block.hashMerkleRoot = block.calc_merkle_root()
            block.sign_block(block_sig_key)
            block.rehash()
            blocks.append(block)
            self._remove_from_staking_prevouts(block.prevoutStake)
            tip['height'] += 1
            tip['hash'] = block.hash
            tip['time'] += 0x10
            tip['modifier'] = bytes_to_hex_str(hash256(ser_uint256(block.prevoutStake.hash) + ser_uint256(int(tip['modifier'], 16)))[::-1])

        msg = msg_headers()
        msg.headers.extend([CBlockHeader(b) for b in blocks])
        self.p2p_node.send_message(msg)
        self.node.gettxoutsetinfo()
        for block in blocks:
            self.p2p_node.send_message(msg_block(block))
            self.p2p_node.sync_with_ping()

        new_size = get_dir_size(self.node.datadir+'/regtest/blocks/')
        assert(new_size-old_size < 1000000)
        self.close_p2p_connection()


    def spam_same_stake_block_test(self):
        old_size = get_dir_size(self.node.datadir+'/regtest/blocks/')
        self.start_p2p_connection()

        tip = self.node.getblock(self.node.getblockhash(self.node.getblockcount()-1))
        block, block_sig_key = self._create_pos_block(self.node, self.staking_prevouts, tip=tip)
        recent_unspents = [unspent for unspent in self.node.listunspent() if unspent['confirmations'] < 2*COINBASE_MATURITY]
        for i in range(19):
            unspent = recent_unspents.pop(0)
            tx = CTransaction()
            tx.vin = [CTxIn(COutPoint(int(unspent['txid'], 16), unspent['vout']))]
            tx.vout = [CTxOut(0, CScript([OP_TRUE]*100*1000))]
            tx = rpc_sign_transaction(self.node, tx)
            block.vtx.append(tx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.sign_block(block_sig_key)

        for i in range(400):
            block.nNonce = i
            block.rehash()
            block.sign_block(block_sig_key)
            self.p2p_node.send_message(msg_block(block))
        time.sleep(10)

        new_size = get_dir_size(self.node.datadir+'/regtest/blocks/')
        assert(new_size-old_size < 1000000)
        self.close_p2p_connection()

    def run_test(self):
        self.node = self.nodes[0]
        privkey = byte_to_base58(hash256(struct.pack('<I', 0)), 239)        
        self.node.importprivkey(privkey)
        self.node.setmocktime(int(time.time() - 1000*24*60*60))
        generatesynchronized(self.node, 1000 + COINBASE_MATURITY, "qSrM9K6FMhZ29Vkp8Rdk8Jp66bbfpjFETq", self.nodes)
        self.staking_prevouts = collect_prevouts(self.node)
        generatesynchronized(self.node, COINBASE_MATURITY, None, self.nodes)
        self.sync_blocks()
        self.disconnect_nodes(0, 1)
        self.node.setmocktime(0)
        self.start_p2p_connection()
        
        if self.options.run_standard_tests:
            print("cannot_submit_header_before_rolling_checkpoint_test")
            self.cannot_submit_header_before_rolling_checkpoint_test()
            print("can_submit_header_after_rolling_checkpoint_test")
            self.can_submit_header_after_rolling_checkpoint_test()
            #self.cannot_submit_invalid_prevout_test()
            print("cannot_submit_header_oversized_signature_test")
            self.cannot_submit_header_oversized_signature_test()
            self.node.generate(1)
            print("can_sync_after_offline_period_test")
            self.can_sync_after_offline_period_test()
            self.staking_prevouts = collect_prevouts(self.node, min_confirmations=self.node.getblockcount()-1000)
            print("reorg_requires_more_chainwork_test")
            self.reorg_requires_more_chainwork_test()
        
        if self.options.dos_same_height:
            print("dos_protection_triggered_via_spam_on_same_height_test")
            self.dos_protection_triggered_via_spam_on_same_height_test()

        if self.options.dos_variable_height:
            print("dos_protection_triggered_via_spam_on_variable_height_test")
            self.dos_protection_triggered_via_spam_on_variable_height_test()

        self.spam_same_stake_block_test()
        self.connect_nodes(0, 1)
        self.sync_blocks()

        # None of the spam should have been relayed to our other node
        assert_equal(len(self.nodes[1].getchaintips()), 1)
        assert_equal(len(self.nodes[1].getpeerinfo()), 1)
        assert_equal(self.nodes[1].getchaintips()[0]['hash'], self.node.getbestblockhash())

if __name__ == '__main__':
    QtumHeaderSpamTest().main()
