#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.address import *
from test_framework.qtum import *
import sys
import random
import time

class QtumIgnoreMPOSParticipantRewardTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-lastmposheight=999999']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def remove_from_staking_prevouts(self, remove_prevout):
        for j in range(len(self.staking_prevouts)):
            prevout = self.staking_prevouts[j]
            if prevout[0].serialize() == remove_prevout.serialize():
                self.staking_prevouts.pop(j)
                break

    def run_test(self):
        privkey = byte_to_base58(hash256(struct.pack('<I', 0)), 239)
        for n in self.nodes:
            n.importprivkey(privkey)

        self.node = self.nodes[0]
        self.node.setmocktime(int(time.time()) - 1000000)
        self.node.generatetoaddress(10 + COINBASE_MATURITY, "qSrM9K6FMhZ29Vkp8Rdk8Jp66bbfpjFETq")

        """
        pragma solidity ^0.4.12;
        contract Test {
            function() payable {
            }
        }
        """
        bytecode = "60606040523415600e57600080fd5b5b603580601c6000396000f30060606040525b5b5b0000a165627a7a723058205093ec5d227f741a4c8511f495e12b897d670259ab2f2b5b241af6af08753f5e0029"
        contract_address = self.node.createcontract(bytecode)['address']

        activate_mpos(self.node)

        self.staking_prevouts = collect_prevouts(self.node)
        # Only have staking outputs with nValue == 20000.0
        # Since the rest of the code relies on this
        i = 0
        while i < len(self.staking_prevouts):
            if self.staking_prevouts[i][1] != 20000*COIN:
                self.staking_prevouts.pop(i)
            i += 1

        nTime = int(time.time()) & 0xfffffff0
        self.node.setmocktime(nTime)


        # Find the block.number - 505 coinstake's 2nd output
        # This will be an mpos participant
        mpos_participant_block = self.node.getblock(self.node.getblockhash(self.node.getblockcount() - 505))
        mpos_participant_txid = mpos_participant_block['tx'][1]
        mpos_participant_tx = self.node.decoderawtransaction(self.node.gettransaction(mpos_participant_txid)['hex'])
        mpos_participant_pubkey = hex_str_to_bytes(mpos_participant_tx['vout'][1]['scriptPubKey']['asm'].split(' ')[0])
        mpos_participant_hpubkey = hash160(mpos_participant_pubkey)
        mpos_participant_addr = hex_hash_to_p2pkh(bytes_to_hex_str(mpos_participant_hpubkey))

        tx = CTransaction()
        tx.vin = [make_vin_from_unspent(self.node, address=mpos_participant_addr)]
        tx.vout = [CTxOut(0, scriptPubKey=CScript([b"\x04", CScriptNum(4000000), CScriptNum(100000), b"\x00", hex_str_to_bytes(contract_address), OP_CALL]))]
        tx = rpc_sign_transaction(self.node, tx)
        self.remove_from_staking_prevouts(tx.vin[0].prevout)

        block, block_sig_key = create_unsigned_mpos_block(self.node, self.staking_prevouts, block_fees=int(10000*COIN)-397897500000, nTime=nTime)
        block.vtx.append(tx)
        block.vtx[1].vout.append(CTxOut(397897500000, scriptPubKey=CScript([OP_DUP, OP_HASH160, mpos_participant_hpubkey, OP_EQUALVERIFY, OP_CHECKSIG])))

        # Delete the mpos participant reward and assign it to the staker
        block.vtx[1].vout.pop(-5)
        block.vtx[1].vout[1].nValue += 260210250000

        # Resign the coinstake tx
        block.vtx[1] = rpc_sign_transaction(self.node, block.vtx[1])
        block.hashMerkleRoot = block.calc_merkle_root()
        block.sign_block(block_sig_key)

        # Make sure that the block was rejected
        blockcount = self.node.getblockcount()
        print(self.node.submitblock(bytes_to_hex_str(block.serialize())))
        assert_equal(self.node.getblockcount(), blockcount)
        
if __name__ == '__main__':
    QtumIgnoreMPOSParticipantRewardTest().main()
