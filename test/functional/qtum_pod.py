#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.blocktools import *
from test_framework.qtum import *
from test_framework.script import *
from test_framework.address import *
import time

class QtumPODTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [
            ['-txindex=1', '-logevents=1'],
            ['-txindex=1', '-logevents=1', '-superstaking=1', '-staking=1']
        ]
    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        mocktime = int(time.time()) - 3600
        mocktime &= 0xffffffff - TIMESTAMP_MASK
        for n in self.nodes: n.setmocktime(mocktime)
        self.delegator = self.nodes[0]
        delegator_address = self.delegator.getnewaddress()
        delegator_address_hex = p2pkh_to_hex_hash(delegator_address)
        delegator_key = wif_to_ECKey(self.delegator.dumpprivkey(delegator_address))

        self.staker = self.nodes[1]
        staker_address = self.staker.getnewaddress()
        staker_key = wif_to_ECKey(self.staker.dumpprivkey(staker_address))

        block_count_staker = 100
        self.staker.generatetoaddress(block_count_staker, staker_address)
        self.sync_all()
        generatesynchronized(self.delegator, COINBASE_MATURITY + 100, delegator_address, self.nodes)
        self.sync_all()

        pod = create_POD(self.delegator, delegator_address, staker_address)

        fee = 20

        delegate_to_staker(self.delegator, delegator_address, staker_address, fee, pod)
        self.delegator.generatetoaddress(1, delegator_address)
        self.sync_all()

        # delegation not available for now
        self.restart_node(0, ['-txindex=1', '-logevents=1', '-offlinestakingheight=1000000'])
        self.restart_node(1, ['-txindex=1', '-logevents=1', '-offlinestakingheight=1000000', '-superstaking=1'])

        mocktime += 512
        for n in self.nodes: n.setmocktime(mocktime)

        delegator_prevouts = collect_prevouts(self.delegator)
        staker_prevouts = collect_prevouts(self.staker)
        staker_prevout_for_nas = staker_prevouts.pop(0)[0]

        best_block_hash = self.delegator.getbestblockhash()
        block = create_delegated_pos_block(self.staker, staker_key, staker_prevout_for_nas, delegator_address_hex, pod, fee, delegator_prevouts, nFees=0, nTime=mocktime)
        assert_equal(self.staker.submitblock(bytes_to_hex_str(block.serialize())), 'bad-cb-header')
        assert_equal(self.staker.getbestblockhash(), best_block_hash)

        # re-enable delegation from now
        self.restart_node(0, ['-txindex=1', '-logevents=1'])
        self.restart_node(1, ['-txindex=1', '-logevents=1', '-superstaking=1'])

        delegator_prevouts = collect_prevouts(self.delegator)
        staker_prevouts = collect_prevouts(self.staker)
        staker_prevout_for_nas = staker_prevouts.pop(0)[0]

        block = create_delegated_pos_block(self.staker, staker_key, staker_prevout_for_nas, delegator_address_hex, pod, fee, delegator_prevouts, nFees=0, nTime=mocktime)
        assert_equal(self.staker.submitblock(bytes_to_hex_str(block.serialize())), None)
        assert_equal(self.staker.getbestblockhash(), block.hash)

        delegator_prevouts = collect_prevouts(self.delegator)
        staker_prevouts = collect_prevouts(self.staker)

        # try to pos with delegated utxos
        block, k = create_unsigned_pos_block(self.delegator, delegator_prevouts, nTime=mocktime)
        block.vtx[1].vout[1].scriptPubKey = CScript([delegator_key.get_pubkey().get_bytes(), OP_CHECKSIG])
        block.vtx[1].vout[2].scriptPubKey = CScript([delegator_key.get_pubkey().get_bytes(), OP_CHECKSIG])
        block.vtx[1] = rpc_sign_transaction(self.delegator, block.vtx[1])
        block.vtx[1].rehash()
        block.hashMerkleRoot = block.calc_merkle_root()
        block.sign_block(wif_to_ECKey(self.delegator.dumpprivkey(delegator_address)))
        block.rehash()
        assert_equal(self.delegator.submitblock(bytes_to_hex_str(block.serialize())), 'stake-delegation-not-used')

        mocktime += 128
        for n in self.nodes: n.setmocktime(mocktime)
        # try to stake with staker's own utxos
        block, k = create_unsigned_pos_block(self.staker, staker_prevouts, nTime=mocktime)
        block.vtx[1].vout[1].scriptPubKey = CScript([staker_key.get_pubkey().get_bytes(), OP_CHECKSIG])
        block.vtx[1].vout[2].scriptPubKey = CScript([staker_key.get_pubkey().get_bytes(), OP_CHECKSIG])
        block.vtx[1] = rpc_sign_transaction(self.staker, block.vtx[1])
        block.vtx[1].rehash()
        block.hashMerkleRoot = block.calc_merkle_root()
        block.sign_block(staker_key)
        block.rehash()
        assert_equal(self.staker.submitblock(bytes_to_hex_str(block.serialize())), None)
        assert_equal(self.staker.getbestblockhash(), block.hash)

if __name__ == '__main__':
    QtumPODTest().main()
