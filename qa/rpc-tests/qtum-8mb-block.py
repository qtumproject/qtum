#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.blocktools import *
from test_framework.qtum import *

class Qtum8MBBlock(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [[], []])
        self.node = self.nodes[0]
        self.is_network_split = False
        connect_nodes_bi(self.nodes, 0, 1)

    def run_test(self):
        # Make sure that segwit is activated
        self.node.generate(10+COINBASE_MATURITY*4)
        self.sync_all()

        tx = CTransaction()
        tx.vin = [make_vin(self.node, 2*COIN)]
        tx.vout = [CTxOut(2*COIN - 100000, CScript([OP_TRUE]))]
        tx.rehash()
        tx_hex = self.node.signrawtransaction(bytes_to_hex_str(tx.serialize()))['hex']
        txid = self.node.sendrawtransaction(tx_hex)
        self.node.generate(1)
        self.sync_all()


        NUM_DROPS = 200

        # To tweak the size of the submitted block, change this value
        NUM_OUTPUTS = 101

        witness_program = CScript([OP_2DROP]*NUM_DROPS + [OP_TRUE])
        witness_hash = uint256_from_str(sha256(witness_program))
        scriptPubKey = CScript([OP_0, ser_uint256(witness_hash)])

        prevout = COutPoint(int(txid, 16), 0)
        value = 2*COIN - 100000

        parent_tx = CTransaction()
        parent_tx.vin.append(CTxIn(prevout, b""))
        child_value = int(value/NUM_OUTPUTS)
        for i in range(NUM_OUTPUTS):
            parent_tx.vout.append(CTxOut(child_value, scriptPubKey))
        parent_tx.vout[0].nValue -= 50000
        assert(parent_tx.vout[0].nValue > 0)
        parent_tx.rehash()

        child_tx = CTransaction()
        for i in range(NUM_OUTPUTS):
            child_tx.vin.append(CTxIn(COutPoint(parent_tx.sha256, i), b""))
        child_tx.vout = [CTxOut(value - 100000, CScript([OP_TRUE]))]
        for i in range(NUM_OUTPUTS):
            child_tx.wit.vtxinwit.append(CTxInWitness())
            child_tx.wit.vtxinwit[-1].scriptWitness.stack = [b'a'*195]*(2*NUM_DROPS) + [witness_program]
        child_tx.rehash()

        tip = self.nodes[0].getbestblockhash()
        height = self.nodes[0].getblockcount() + 1
        block_time = self.nodes[0].getblockheader(tip)["time"] + 1
        block = create_block(int(tip, 16), create_coinbase(height), block_time)
        block.nVersion = 4
        block.rehash()
        block.vtx.extend([parent_tx, child_tx])
        add_witness_commitment(block, 0)
        block.solve()

        block_count = self.node.getblockcount()
        print("Size of submitted block: ", len(block.serialize(with_witness=True)), "bytes")
        ret = self.node.submitblock(bytes_to_hex_str(block.serialize(with_witness=True)))
        assert_equal(ret, None)
        self.sync_all()

        assert_equal(block_count+1, self.nodes[0].getblockcount())
        assert_equal(block_count+1, self.nodes[1].getblockcount())

if __name__ == '__main__':
    Qtum8MBBlock().main()
