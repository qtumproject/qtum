#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.qtum import *
from test_framework.address import *
from test_framework.blocktools import *
import sys
import time
import io
import random

"""
Note, these tests do not test the functionality of the DGP template contract itself, for tests for the DGP template, see qtum-dgp.py
"""
class QtumDGPActivation(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [[]])
        self.node = self.nodes[0]


    def create_block_of_approx_max_size(self, size_in_bytes):
        tip = self.node.getblock(self.node.getbestblockhash())
        block = create_block(int(self.node.getbestblockhash(), 16), create_coinbase(self.node.getblockcount()+1), tip['time'])
        block.hashUTXORoot = int(tip['hashUTXORoot'], 16)
        block.hashStateRoot = int(tip['hashStateRoot'], 16)

        unspents = self.node.listunspent()
        while len(block.serialize()) < size_in_bytes:
            unspent = unspents.pop(0)
            tx = CTransaction()
            tx.vin = [CTxIn(COutPoint(int(unspent['txid'], 16), unspent['vout']), nSequence=0)]
            for i in range(50):
                tx.vout.append(CTxOut(int(unspent['amount']*COIN/100 - 11000), scriptPubKey=CScript([OP_TRUE]*10000)))
            tx_hex = self.node.signrawtransaction(bytes_to_hex_str(tx.serialize()))['hex']
            f = io.BytesIO(hex_str_to_bytes(tx_hex))
            block.vtx.append(CTransaction())
            block.vtx[-1].deserialize(f)

        while len(block.serialize()) > size_in_bytes:
            block.vtx[-1].vout.pop(-1)
            if not block.vtx[-1].vout:
                block.vtx.pop(-1)
        tx_hex = self.node.signrawtransaction(bytes_to_hex_str(block.vtx[-1].serialize()))['hex']
        f = io.BytesIO(hex_str_to_bytes(tx_hex))
        block.vtx[-1] = CTransaction()
        block.vtx[-1].deserialize(f)

        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        print("block size", len(block.serialize()))
        return block

    def create_proposal_contract(self, block_size=2000000):
        """
        pragma solidity ^0.4.11;

        contract blockSize {

            uint32[1] _blockSize=[
                8000000 //block size in bytes
            ];

            function getBlockSize() constant returns(uint32[1] _size){
                return _blockSize;
            }
        }
        """
        # The contracts below only differ in the _blockSize variable
        if block_size == 8000000:
            contract_data = self.node.createcontract("6060604052602060405190810160405280627a120062ffffff16815250600090600161002c92919061003d565b50341561003857600080fd5b610112565b8260016007016008900481019282156100ce5791602002820160005b8382111561009c57835183826101000a81548163ffffffff021916908362ffffff1602179055509260200192600401602081600301049283019260010302610059565b80156100cc5782816101000a81549063ffffffff021916905560040160208160030104928301926001030261009c565b505b5090506100db91906100df565b5090565b61010f91905b8082111561010b57600081816101000a81549063ffffffff0219169055506001016100e5565b5090565b90565b610162806101216000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806392ac3c621461003e575b600080fd5b341561004957600080fd5b610051610090565b6040518082600160200280838360005b8381101561007d5780820151818401525b602081019050610061565b5050505090500191505060405180910390f35b610098610108565b60006001806020026040519081016040528092919082600180156100fd576020028201916000905b82829054906101000a900463ffffffff1663ffffffff16815260200190600401906020826003010492830192600103820291508084116100c05790505b505050505090505b90565b6020604051908101604052806001905b600063ffffffff1681526020019060019003908161011857905050905600a165627a7a723058209bab110523b5fdedfb12512d3aedc1ba1add53dff85edb77aeec48ebdc01c35c0029", 10000000, QTUM_MIN_GAS_PRICE/COIN)
        elif block_size == 1000000:
            contract_data = self.node.createcontract("6060604052602060405190810160405280620f424062ffffff16815250600090600161002c92919061003d565b50341561003857600080fd5b610112565b8260016007016008900481019282156100ce5791602002820160005b8382111561009c57835183826101000a81548163ffffffff021916908362ffffff1602179055509260200192600401602081600301049283019260010302610059565b80156100cc5782816101000a81549063ffffffff021916905560040160208160030104928301926001030261009c565b505b5090506100db91906100df565b5090565b61010f91905b8082111561010b57600081816101000a81549063ffffffff0219169055506001016100e5565b5090565b90565b610162806101216000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806392ac3c621461003e575b600080fd5b341561004957600080fd5b610051610090565b6040518082600160200280838360005b8381101561007d5780820151818401525b602081019050610061565b5050505090500191505060405180910390f35b610098610108565b60006001806020026040519081016040528092919082600180156100fd576020028201916000905b82829054906101000a900463ffffffff1663ffffffff16815260200190600401906020826003010492830192600103820291508084116100c05790505b505050505090505b90565b6020604051908101604052806001905b600063ffffffff1681526020019060019003908161011857905050905600a165627a7a7230582034c00d84f338629f594676d9bc32d5b9d7b92f3b438e9cc82a3efd92805f14730029", 10000000, QTUM_MIN_GAS_PRICE/COIN)

        self.proposal_address = contract_data['address']

    def run_test(self):
        self.node.generate(1000 + COINBASE_MATURITY)
        self.BLOCK_SIZE_DGP = DGPState(self.node, "0000000000000000000000000000000000000081")
        
        # Start off by setting ourself as admin
        admin_address = self.node.getnewaddress()

        # Set ourself up as admin
        self.BLOCK_SIZE_DGP.send_set_initial_admin(admin_address)
        self.node.generate(1)

        # Activate a proposal for 8MB blocks
        max_block_size = 8000000
        self.create_proposal_contract(max_block_size)
        self.BLOCK_SIZE_DGP.send_add_address_proposal(self.proposal_address, 2, admin_address)
        self.node.generate(2)

        # Submit a block close to 8MB and make sure that it was accepted
        block = self.create_block_of_approx_max_size(max_block_size)
        current_block_count = self.node.getblockcount()
        assert_equal(self.node.submitblock(bytes_to_hex_str(block.serialize())), None)
        assert_equal(self.node.getblockcount(), current_block_count+1)

        # Activate a proposal for 1MB blocks
        max_block_size = 1000000
        self.create_proposal_contract(max_block_size)
        self.BLOCK_SIZE_DGP.send_add_address_proposal(self.proposal_address, 2, admin_address)
        self.node.generate(2)


        # We now have had the following chain of events:
        # 1. blocksizelimit=8MB
        # 2. 8MB block submitted and accepted
        # 3. blocksizelimit=1MB
        # Now we should only allow new 1MB blocks,
        # however the old 8MB block not cause any errors when restarting since it was accepted at a time when 8MB was the block size limit

        # Restart the qtumd to verify that no crashes occurs on startup
        stop_nodes(self.nodes)
        self.setup_network()


if __name__ == '__main__':
    QtumDGPActivation().main()
