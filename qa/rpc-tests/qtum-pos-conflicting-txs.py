#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.blocktools import *
from test_framework.qtum import *

"""
 This test specifically tests that inputs to transactions in the mempool are not used in staking.
"""
class QtumPOSConflictingStakingMempoolTxTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.num_nodes = 2
        self.setup_clean_chain = True

    def setup_network(self, split=False):
        # Use aggressive staking to lower the amount of time we have to wait.
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [['-staking=1', '-txindex=1', '-aggressive-staking'], ['-staking=1', '-txindex=1']])
        self.is_network_split = True

    def sync_disconnected_nodes(self, receive_from, sync_to):
        for block_number in range(sync_to.getblockcount(), receive_from.getblockcount()+1):
            block_hash = receive_from.getblockhash(block_number)
            block_raw_hex = receive_from.getblock(block_hash, False)
            sync_to.submitblock(block_raw_hex)

    def run_test(self):
        # First generate some blocks so we have 20 valid staking txs for the node we run the test on (node#0)
        # We also mature three coinbases for the node that will orphan node#0s blocks
        # We contiously sync the blocks between the disconnected nodes, using getblock and submitblock.
        staking_nodes_prevouts = []
        self.nodes[1].generate(3)
        self.sync_disconnected_nodes(self.nodes[1], self.nodes[0])
        last_block_hashes = self.nodes[0].generate(20)
        self.sync_disconnected_nodes(self.nodes[0], self.nodes[1])
        self.nodes[1].generate(500)
        self.sync_disconnected_nodes(self.nodes[1], self.nodes[0])

        # Wait until all the (disconnected) nodes have started staking.
        while not self.nodes[0].getstakinginfo()['staking']:
            time.sleep(0.01)
        print('staking...')

        # Spend the only available staking tx for node#0, give the staker some time to start before sending the tx that spends the only available staking tx
        txs = []
        for last_block_hash in last_block_hashes:
            last_coinbase = self.nodes[0].getblock(last_block_hash)['tx'][0]
            staking_prevout = COutPoint(int(last_coinbase, 16), 0)
            tx = CTransaction()
            tx.vin = [CTxIn(staking_prevout)]
            tx.vout = [CTxOut(int(20000*COIN), CScript([OP_DUP, OP_HASH160, hex_str_to_bytes(p2pkh_to_hex_hash(self.nodes[0].getnewaddress())), OP_EQUALVERIFY, OP_CHECKSIG]))]
            txs.append(rpc_sign_transaction(self.nodes[0], tx))

        # We set the time so that the staker will find a valid staking block 3 timeslots away
        time_until_next_valid_block = int(self.nodes[0].getblock(self.nodes[0].getbestblockhash())['time'] - 16)
        self.nodes[0].setmocktime(time_until_next_valid_block)

        # Allow the qtum staker some time to run (since we set a mocktime the time will not advance)
        time.sleep(80)

        # Now the staker will have found a valid staking block and is waiting to publish it to the network
        # Therefore we send the tx spending the block's staking tx
        for tx in txs:
            self.nodes[0].sendrawtransaction(bytes_to_hex_str(tx.serialize()))
        
        # Advance the time so that the block can be published
        # Wait for 10 seconds so that the staker has time to attempt to publish the block
        self.nodes[0].setmocktime(time_until_next_valid_block+48)
        time.sleep(10)

        print('Checking node %d; blockcount=%d' % (0, self.nodes[0].getblockcount()))
        
        # Allow node#1 to stake two blocks, which will orphan any (potentially) staked block in node#0
        while self.nodes[1].getblockcount() < 525:
            time.sleep(0.1)
        self.nodes[0].setmocktime(self.nodes[1].getblock(self.nodes[1].getbestblockhash())['time'])

        # Connect the nodes
        connect_nodes_bi(self.nodes, 0, 1)

        # Sync the nodes
        timeout = time.time() + 10
        while timeout < time.time():
            if self.nodes[0].getbestblockhash() == self.nodes[1].getbestblockhash():
                break
            time.sleep(0.1)
            self.nodes[0].setmocktime(int(time.time()))

        assert_equal(self.nodes[0].getbestblockhash(), self.nodes[1].getbestblockhash())

        best_chain_height = self.nodes[1].getblockcount()
        assert_equal(self.nodes[0].getblockcount(), best_chain_height)

        # Allow one more block to be staked, which will include the txs in the mempool
        while self.nodes[1].getblockcount() < best_chain_height+1:
            self.nodes[0].setmocktime(int(time.time()))
            time.sleep(0.1)

        print('node#0 %d; blockcount=%d' % (0, self.nodes[0].getblockcount()))
        print('node#1 %d; blockcount=%d' % (0, self.nodes[1].getblockcount()))
        # Now we should have a balance equal to 
        assert_equal(int(self.nodes[0].getbalance()*COIN), int(20*20000*COIN))

if __name__ == '__main__':
    QtumPOSConflictingStakingMempoolTxTest().main()