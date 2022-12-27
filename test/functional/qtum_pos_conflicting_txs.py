#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.blocktools import *
from test_framework.qtum import *

"""
 This test specifically tests that inputs to transactions in the mempool are not used in staking.
"""
class QtumPOSConflictingStakingMempoolTxTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [['-txindex=1', '-aggressive-staking'], ['-staking=1', '-txindex=1']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def sync_disconnected_nodes(self, receive_from, sync_to):
        for block_number in range(sync_to.getblockcount(), receive_from.getblockcount()+1):
            block_hash = receive_from.getblockhash(block_number)
            block_raw_hex = receive_from.getblock(block_hash, False)
            sync_to.submitblock(block_raw_hex)

    def run_test(self):
        privkey = byte_to_base58(hash256(struct.pack('<I', 0)), 239)
        self.nodes[0].importprivkey(privkey)
        self.disconnect_nodes(0, 1)
        for n in self.nodes: n.setmocktime(int(time.time())-10000)
        # First generate some blocks so we have 20 valid staking txs for the node we run the test on (node#0)
        # We also mature three coinbases for the node that will orphan node#0s blocks
        # We contiously sync the blocks between the disconnected nodes, using getblock and submitblock.
        staking_nodes_prevouts = []
        self.nodes[1].generate(3)
        self.sync_disconnected_nodes(self.nodes[1], self.nodes[0])
        last_block_hashes = self.nodes[0].generatetoaddress(20, "qSrM9K6FMhZ29Vkp8Rdk8Jp66bbfpjFETq")
        self.sync_disconnected_nodes(self.nodes[0], self.nodes[1])
        self.nodes[1].generate(COINBASE_MATURITY)
        self.sync_disconnected_nodes(self.nodes[1], self.nodes[0])

        # Spend the only available staking tx for node#0, give the staker some time to start before sending the tx that spends the only available staking tx
        txs = []
        for last_block_hash in last_block_hashes:
            last_coinbase = self.nodes[0].getblock(last_block_hash)['tx'][0]
            staking_prevout = COutPoint(int(last_coinbase, 16), 0)
            tx = CTransaction()
            tx.vin = [CTxIn(staking_prevout)]
            tx.vout = [CTxOut(int((20000-0.01)*COIN), CScript([OP_DUP, OP_HASH160, hex_str_to_bytes(p2pkh_to_hex_hash(self.nodes[0].getnewaddress())), OP_EQUALVERIFY, OP_CHECKSIG]))]
            txs.append(rpc_sign_transaction(self.nodes[0], tx))

        print("blkcnt", self.nodes[0].getblockcount())
        for n in self.nodes: n.setmocktime(int(time.time()))

        staking_prevouts = collect_prevouts(self.nodes[0])
        print(len(staking_prevouts))
        nTime = int(time.time()) & (~TIMESTAMP_MASK)
        block, key = create_unsigned_pos_block(self.nodes[0], staking_prevouts, nTime=nTime)
        block.sign_block(key)
        block.rehash()

        for tx in txs:
            self.nodes[0].sendrawtransaction(bytes_to_hex_str(tx.serialize()))

        print("blkcnt", self.nodes[0].getblockcount())
        assert_equal(self.nodes[0].submitblock(bytes_to_hex_str(block.serialize())), None)
        assert_equal(self.nodes[0].getblockcount(), COINBASE_MATURITY+24)
        print("blkcnt", self.nodes[0].getblockcount())

        # Allow node#1 to stake two blocks, which will orphan any (potentially) staked block in node#0
        self.wait_until(lambda: self.nodes[1].getblockcount() >= COINBASE_MATURITY+25)
        self.nodes[0].setmocktime(self.nodes[1].getblock(self.nodes[1].getbestblockhash())['time'])

        # Connect the nodes
        print(self.nodes[0].getpeerinfo())
        self.connect_nodes(0, 1)

        # Sync the nodes
        timeout = time.time() + 10

        self.wait_until(lambda: self.nodes[0].getbestblockhash() == self.nodes[1].getbestblockhash())

        print('node#0 %d; blockcount=%d' % (0, self.nodes[0].getblockcount()))
        print('node#1 %d; blockcount=%d' % (0, self.nodes[1].getblockcount()))
        
        best_chain_height = self.nodes[1].getblockcount()
        assert_equal(self.nodes[0].getblockcount(), best_chain_height)

        # Allow one more block to be staked, which will include the txs in the mempool
        self.wait_until(lambda: self.nodes[1].getblockcount() < best_chain_height+1)

        print('node#0 %d; blockcount=%d' % (0, self.nodes[0].getblockcount()))
        print('node#1 %d; blockcount=%d' % (0, self.nodes[1].getblockcount()))
        # Now we should have a balance equal to 
        assert_equal(int(self.nodes[0].getbalance()*COIN), int((19*(INITIAL_BLOCK_REWARD-0.01)+INITIAL_BLOCK_REWARD)*COIN))
        assert_equal(self.nodes[0].getbestblockhash(), self.nodes[1].getbestblockhash())
if __name__ == '__main__':
    QtumPOSConflictingStakingMempoolTxTest().main()