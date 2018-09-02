#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.address import *
from test_framework.qtum import *
from test_framework.qtumx86 import *
import sys
import random
import time
import io

"""
    Verifies that the x86 globals return the expected values
    Currently only tests execution upon deployment (OP_CALL state mutations do not seem to persist as of now)
"""
class QtumX86GlobalsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-staking=1']]

    def get_contract_formatted_asm(self):
        formatted_asm = yasm_compile("""
            ; syscall, value length, key, key length
            %macro store_ebx_syscall 4
                mov eax, %1
                mov ebx, {QX86_DATA_ADDRESS}
                int {QtumSystem}

                ; set up the correct address to persist the return value
                mov edx, {QX86_DATA_ADDRESS}
                mov esi, %2
                mov ebx, %3
                mov ecx, %4
                mov eax, {QSCWriteStorage}
                int {QtumSystem}                ; commit the pair to the KV-store
            %endmacro

            %macro store_eax_syscall 4
                mov eax, %1
                int {QtumSystem}
                mov edx, {QX86_DATA_ADDRESS}
                mov [edx], eax
                mov esi, %2
                mov ebx, %3
                mov ecx, %4
                mov eax, {QSCWriteStorage}
                int {QtumSystem}
            %endmacro

            ; is this an OP_CREATE call
            start:
                mov eax, {QSCIsCreate}
                int {QtumSystem}
                cmp eax, 0
                jnz  store_globals; error if not constructor

            ; if no valid route, exit with status = 1
            error:
                mov eax, 1
                int 0xf0

            store_globals:

            QSCPreviousBlockTime:
                store_eax_syscall {QSCPreviousBlockTime}, 0x4, keyblocktime, 9

            QSCBlockCreator:
                store_ebx_syscall {QSCBlockCreator}, 36, keyblockcreator, 12

            QSCBlockGasLimit:
                store_ebx_syscall {QSCBlockGasLimit}, 0x8, keyblockgaslimit, 13

            QSCBlockHeight:
                store_eax_syscall {QSCBlockHeight}, 0x4, keyblockheight, 11

            QSCSelfAddress:
                store_ebx_syscall {QSCSelfAddress}, 36, keyselfaddress, 11

            exit:
                ; clean exit
                mov eax, 0
                int 0xf0

            section .data align=1
            keyblocktime db 'blocktime'
            keyblockcreator db 'blockcreator'
            keyblockdifficulty db 'blockdifficulty'
            keyblockgaslimit db 'blockgaslimit'
            keyblockheight db 'blockheight'
            keyselfaddress db 'selfaddress'
            keygetbalance db 'getbalance'
        """.format(QSCIsCreate=QSCIsCreate, QSCWriteStorage=QSCWriteStorage, QSCPreviousBlockTime=QSCPreviousBlockTime,
            QSCBlockGasLimit=QSCBlockGasLimit, QSCBlockCreator=QSCBlockCreator, QSCBlockHeight=QSCBlockHeight, QSCSelfAddress=QSCSelfAddress,
            QtumSystem=QtumSystem, QX86_DATA_ADDRESS=QX86_DATA_ADDRESS, QX86_TX_CALL_DATA_ADDRESS=QX86_TX_CALL_DATA_ADDRESS))
        return formatted_asm

    def verify_x86_globals_test(self, use_staking=False):
        sender = self.node.getnewaddress()
        sender_pkh = p2pkh_to_hex_hash(sender)

        formatted_asm = self.get_contract_formatted_asm()
        contract_address = self.node.createcontract(formatted_asm)['address']

        if use_staking:
            for n in self.nodes:
                n.setmocktime((self.node.getblock(self.node.getbestblockhash())['time']+100) & 0xfffffff0)

            blockcount = self.node.getblockcount()
            while blockcount == self.node.getblockcount():
                time.sleep(0.1)
            blockhash = self.node.getblockhash(blockcount+1)
            authorTxIndexAndVoutIndex = 1
        else:
            blockhash = self.node.generate(1)[0]
            authorTxIndexAndVoutIndex = 0

        block = self.node.getblock(blockhash)
        blocktxids = block['tx']
        coinbase_tx = self.node.getrawtransaction(blocktxids[authorTxIndexAndVoutIndex], True)
        coinbase_address = coinbase_tx['vout'][authorTxIndexAndVoutIndex]['scriptPubKey']['addresses'][0]
        coinbase_pkh = p2pkh_to_hex_hash(coinbase_address)

        block_time = struct.unpack('<I', hex_str_to_bytes(self.node.x86readstorage(contract_address, "blocktime")))[0]
        assert_equal(block_time, block['time'])

        block_creator = self.node.x86readstorage(contract_address, "blockcreator")
        assert_equal(block_creator, "02000000" + coinbase_pkh + "000000000000000000000000")

        block_gas_limit = struct.unpack('<Q', hex_str_to_bytes(self.node.x86readstorage(contract_address, "blockgaslimit")))[0]
        assert_equal(block_gas_limit, 10000000)

        block_height = struct.unpack('<I', hex_str_to_bytes(self.node.x86readstorage(contract_address, "blockheight")))[0]
        assert_equal(block_height, block['height'])

        self_address = self.node.x86readstorage(contract_address, "selfaddress")
        assert_equal(self_address, "04000000" + contract_address + "000000000000000000000000")

    def run_test(self):
        self.node = self.nodes[0]
        self.node.generate(COINBASE_MATURITY+1)
        self.verify_x86_globals_test()
        self.verify_x86_globals_test(use_staking=True)

if __name__ == '__main__':
    QtumX86GlobalsTest().main()
