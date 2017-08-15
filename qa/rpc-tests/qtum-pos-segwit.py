#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.qtum import *
from test_framework.blocktools import *
from test_framework.key import *
import io
import time

class QtumPOSSegwitTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [[]])
        self.node = self.nodes[0]


    def create_unsigned_pos_block(self, staking_prevouts, nTime):

        best_block_hash = self.node.getbestblockhash()
        block_height = self.node.getblockcount()

        parent_block_stake_modifier = int(self.node.getblock(best_block_hash)['modifier'], 16)
        parent_block_raw_hex = self.node.getblock(best_block_hash, False)
        f = io.BytesIO(hex_str_to_bytes(parent_block_raw_hex))
        parent_block = CBlock()
        parent_block.deserialize(f)
        coinbase = create_coinbase(block_height+1)
        coinbase.vout[0].nValue = 0
        coinbase.vout[0].scriptPubKey = b""
        coinbase.rehash()
        block = create_block(int(best_block_hash, 16), coinbase, nTime)
        block.hashPrevBlock = int(best_block_hash, 16)
        if not block.solve_stake(parent_block_stake_modifier, staking_prevouts):
            return None

        # create a new private key used for block signing.
        block_sig_key = CECKey()
        block_sig_key.set_secretbytes(hash256(struct.pack('<I', 0xffff)))
        pubkey = block_sig_key.get_pubkey()
        scriptPubKey = CScript([pubkey, OP_CHECKSIG])
        stake_tx_unsigned = CTransaction()
        coinstake_prevout = block.prevoutStake

        stake_tx_unsigned.vin.append(CTxIn(coinstake_prevout))
        stake_tx_unsigned.vout.append(CTxOut())
        stake_tx_unsigned.vout.append(CTxOut(int(10002*COIN), scriptPubKey))
        stake_tx_unsigned.vout.append(CTxOut(int(10002*COIN), scriptPubKey))

        stake_tx_signed_raw_hex = self.node.signrawtransaction(bytes_to_hex_str(stake_tx_unsigned.serialize()))['hex']
        f = io.BytesIO(hex_str_to_bytes(stake_tx_signed_raw_hex))
        stake_tx_signed = CTransaction()
        stake_tx_signed.deserialize(f)
        block.vtx.append(stake_tx_signed)

        return (block, block_sig_key)

    def collect_staking_prevouts(self):
        blocks = []
        for block_number in range(self.node.getblockcount()):
            block_hash = self.node.getblockhash(block_number)
            blocks.append(self.node.getblock(block_hash))

        staking_prevouts = []

        for unspent in self.node.listunspent():
            for block in blocks:
                if unspent['txid'] in block['tx']:
                    tx_block_time = block['time']
                    break
            else:
                assert(False)

            if unspent['confirmations'] > COINBASE_MATURITY:
                staking_prevouts.append((COutPoint(int(unspent['txid'], 16), unspent['vout']), int(unspent['amount'])*COIN, tx_block_time))

        return staking_prevouts

    def run_test(self):
        self.node.setmocktime(int(time.time()) - 2*COINBASE_MATURITY)
        self.node.generate(50+COINBASE_MATURITY)

        staking_prevouts = self.collect_staking_prevouts()

        # Create a tx that will be spent by the segwit parent tx
        tx = CTransaction()
        tx.vin = [make_vin(self.node, 2*COIN)]
        tx.vout = [CTxOut(2*COIN - 100000, CScript([OP_TRUE]))]
        tx.rehash()
        tx_hex = self.node.signrawtransaction(bytes_to_hex_str(tx.serialize()))['hex']
        txid = self.node.sendrawtransaction(tx_hex)
        self.node.generate(1)


        self.node.setmocktime(int(time.time()))


        NUM_DROPS = 200
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


        t = int(time.time()) & 0xfffffff0
        (block, block_sig_key) = self.create_unsigned_pos_block(staking_prevouts, nTime=t)
        block.vtx.extend([parent_tx, child_tx])

        # Add the witness commitment to the coinbase,
        # since it is a PoS block, the witness hash for the coinstake is 0
        add_witness_commitment(block, 0, is_pos=True)

        block.sign_block(block_sig_key)
        block.rehash()

        block_count = self.node.getblockcount()
        self.node.submitblock(bytes_to_hex_str(block.serialize(with_witness=True)))
        assert_equal(self.node.getblockcount(), block_count+1)

if __name__ == '__main__':
    QtumPOSSegwitTest().main()
