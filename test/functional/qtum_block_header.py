#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.blocktools import *
from test_framework.p2p import *
from test_framework.address import *
from test_framework.qtum import *
import time
from test_framework.key import ECKey
from test_framework.script import *
import struct
import io


def find_unspent(node, amount):
    for unspent in node.listunspent():
        if unspent['amount'] == amount and unspent['spendable']:
            return CTxIn(COutPoint(int(unspent['txid'], 16), unspent['vout']), nSequence=0)
    assert(False)

class QtumBlockHeaderTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [[]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.nodes[0].add_p2p_connection(P2PDataStore())
        self.nodes[0].p2ps[0].wait_for_getheaders(timeout=5)

        node = self.nodes[0]
        #mocktime = 1490247077
        #node.setmocktime(mocktime)

        node.generate(10)
        self.block_time = int(time.time())+20
        for i in range(COINBASE_MATURITY):
            self.tip = create_block(int(node.getbestblockhash(), 16), create_coinbase(node.getblockcount()+1), self.block_time+i)
            self.tip.solve()
            self.sync_all_blocks([self.tip])

        #node.generate(COINBASE_MATURITY+50)
        mocktime = COINBASE_MATURITY+50
        spendable_addresses = []
        # store some addresses to use later
        for unspent in node.listunspent():
            spendable_addresses.append(unspent['address'])

        # first make sure that what is a valid block is accepted
        coinbase = create_coinbase(node.getblockcount()+1)
        coinbase.rehash()
        self.tip = create_block(int(node.getbestblockhash(), 16), coinbase, int(time.time()+mocktime+100))
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.solve()
        self.sync_all_blocks([self.tip])

        coinbase = create_coinbase(node.getblockcount()+1)
        coinbase.rehash()
 
        # A block that has an OP_CREATE tx, butwith an incorrect state root
        """
            pragma solidity ^0.4.11;
            contract Test {
                function() payable {}
            }
        """
        tx_hex = node.createcontract("60606040523415600b57fe5b5b60398060196000396000f30060606040525b600b5b5b565b0000a165627a7a72305820693c4900c412f72a51f8c01a36d38d9038d822d953faf5a5b28e40ec6e1a25020029", 1000000, QTUM_MIN_GAS_PRICE_STR, spendable_addresses.pop(-1), False)['raw transaction']
        f = io.BytesIO(hex_str_to_bytes(tx_hex))
        tx = CTransaction()
        tx.deserialize(f)

        coinbase = create_coinbase(node.getblockcount()+1)
        coinbase.rehash()
        self.tip = create_block(int(node.getbestblockhash(), 16), coinbase, int(mocktime+200))
        self.tip.vtx.append(tx)
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.solve()
        self.sync_all_blocks([self.tip], success=False, reconnect=True)


        # Create a contract for use later.
        """
            pragma solidity ^0.4.11;
            contract Test {
                function() payable {}
            }
        """
        contract_address = node.createcontract("60606040523415600b57fe5b5b60398060196000396000f30060606040525b600b5b5b565b0000a165627a7a72305820693c4900c412f72a51f8c01a36d38d9038d822d953faf5a5b28e40ec6e1a25020029")['address']
        node.generate(1)

        realHashUTXORoot = int(node.getblock(node.getbestblockhash())['hashUTXORoot'], 16)
        realHashStateRoot = int(node.getblock(node.getbestblockhash())['hashStateRoot'], 16)

        # A block with both an invalid hashStateRoot and hashUTXORoot
        coinbase = create_coinbase(node.getblockcount()+1)
        coinbase.rehash()
        self.tip = create_block(int(node.getbestblockhash(), 16), coinbase, int(mocktime+300))
        self.tip.hashUTXORoot = 0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
        self.tip.hashStateRoot = 0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.solve()
        self.sync_all_blocks([self.tip], success=False, reconnect=True)


        # A block with a tx, but without updated state hashes
        tx_hex = node.sendtocontract(contract_address, "00", 1, 100000, QTUM_MIN_GAS_PRICE_STR, spendable_addresses.pop(-1), False)['raw transaction']
        f = io.BytesIO(hex_str_to_bytes(tx_hex))
        tx = CTransaction()
        tx.deserialize(f)

        coinbase = create_coinbase(node.getblockcount()+1)
        coinbase.rehash()
        self.tip = create_block(int(node.getbestblockhash(), 16), coinbase, int(mocktime+400))
        self.tip.hashUTXORoot = realHashUTXORoot
        self.tip.hashStateRoot = realHashStateRoot
        self.tip.vtx.append(tx)
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.solve()
        self.sync_all_blocks([self.tip], success=False, reconnect=True)

        # A block with an invalid hashUTXORoot
        coinbase = create_coinbase(node.getblockcount()+1)
        coinbase.rehash()
        self.tip = create_block(int(node.getbestblockhash(), 16), coinbase, int(mocktime+500))
        self.tip.hashStateRoot = realHashStateRoot
        self.tip.hashUTXORoot = 0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.solve()
        self.sync_all_blocks([self.tip], success=False, reconnect=True)

        # A block with an invalid hashStateRoot
        coinbase = create_coinbase(node.getblockcount()+1)
        coinbase.rehash()
        self.tip = create_block(int(node.getbestblockhash(), 16), coinbase, int(mocktime+600))
        self.tip.hashUTXORoot = realHashUTXORoot
        self.tip.hashStateRoot = 0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.solve()
        self.sync_all_blocks([self.tip], success=False, reconnect=True)

        # Verify that blocks with a correct hashStateRoot and hashUTXORoot are accepted.
        coinbase = create_coinbase(node.getblockcount()+1)
        coinbase.rehash()
        self.tip = create_block(int(node.getbestblockhash(), 16), coinbase, int(mocktime+700))
        self.tip.hashUTXORoot = realHashUTXORoot
        self.tip.hashStateRoot = realHashStateRoot
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.solve()
        self.sync_all_blocks([self.tip])


    def reconnect_p2p(self):
        """Tear down and bootstrap the P2P connection to the node.

        The node gets disconnected several times in this test. This helper
        method reconnects the p2p and restarts the network thread."""
        self.nodes[0].disconnect_p2ps()
        self.nodes[0].add_p2p_connection(P2PDataStore())
        self.nodes[0].p2ps[0].wait_for_getheaders(timeout=5)

    def sync_all_blocks(self, blocks, success=True, reject_code=None, reject_reason=None, force_send=False, reconnect=False, timeout=5):
        """Sends blocks to test node. Syncs and verifies that tip has advanced to most recent block.

        Call with success = False if the tip shouldn't advance to the most recent block."""
        self.nodes[0].p2ps[0].send_blocks_and_test(blocks, self.nodes[0], success=success, reject_reason=reject_reason, force_send=force_send, timeout=timeout, expect_disconnect=reconnect)

        if reconnect:
            self.reconnect_p2p()

if __name__ == '__main__':
    QtumBlockHeaderTest().main()
