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

# Class to simplifify communication with the DGP contract
class DGPState:
    def __init__(self, node, contract_address):
        self.node = node
        self.contract_address = contract_address
        self.abiAddAddressProposal = "bf5f1e83" #addAddressProposal(address,uint256)
        self.abiSetInitialAdmin = "6fb81cbb" # setInitialAdmin()

    def send_set_initial_admin(self, sender):
        self.node.sendtoaddress(sender, 1)
        self.node.sendtocontract(self.contract_address, self.abiSetInitialAdmin, 0, 30000000, 0.00000001, sender)

    def send_add_address_proposal(self, proposal_address, type1, sender):
        self.node.sendtoaddress(sender, 1)
        self.node.sendtocontract(self.contract_address, self.abiAddAddressProposal + proposal_address.zfill(64) + hex(type1)[2:].zfill(64), 0, 30000000, 0.00000001, sender)


class QtumDGP4MBBlockSizeTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        self.node = self.nodes[0]
        self.BLOCK_SIZE_DGP = DGPState(self.node, "0000000000000000000000000000000000000081")
        self.is_network_split = False
        connect_nodes_bi(self.nodes, 0, 1)


    # Creates a block close to but less than size_in_bytes
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
            for i in range(100):
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

    def create_proposal_contract(self):
        """
        pragma solidity ^0.4.11;

        contract blockSize {

            uint32[1] _blockSize=[
                4000000 //block size in bytes
            ];

            function getBlockSize() constant returns(uint32[1] _size){
                return _blockSize;
            }
        }
        """
        contract_data = self.node.createcontract("6060604052602060405190810160405280623d090062ffffff16815250600090600161002c92919061003d565b50341561003857600080fd5b610112565b8260016007016008900481019282156100ce5791602002820160005b8382111561009c57835183826101000a81548163ffffffff021916908362ffffff1602179055509260200192600401602081600301049283019260010302610059565b80156100cc5782816101000a81549063ffffffff021916905560040160208160030104928301926001030261009c565b505b5090506100db91906100df565b5090565b61010f91905b8082111561010b57600081816101000a81549063ffffffff0219169055506001016100e5565b5090565b90565b610162806101216000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806392ac3c621461003e575b600080fd5b341561004957600080fd5b610051610090565b6040518082600160200280838360005b8381101561007d5780820151818401525b602081019050610061565b5050505090500191505060405180910390f35b610098610108565b60006001806020026040519081016040528092919082600180156100fd576020028201916000905b82829054906101000a900463ffffffff1663ffffffff16815260200190600401906020826003010492830192600103820291508084116100c05790505b505050505090505b90565b6020604051908101604052806001905b600063ffffffff1681526020019060019003908161011857905050905600a165627a7a72305820c5f02b85c3d9d7b93140775449355f53a7cb98dcafc56f07cdb09e9f2dc240550029", 10000000, 0.00000001)
        self.proposal_address = contract_data['address']

    def run_test(self):
        self.node.generate(1000 + COINBASE_MATURITY)
        # Start off by setting ourself as admin
        admin_address = self.node.getnewaddress()

        # Set ourself up as admin
        self.BLOCK_SIZE_DGP.send_set_initial_admin(admin_address)
        self.node.generate(1)

        # Create a proposal contract for a new max block size of 4MB
        self.create_proposal_contract()
        # Submit the proposal (it is accepted immediately)
        self.BLOCK_SIZE_DGP.send_add_address_proposal(self.proposal_address, 2, admin_address)
        # After these two blocks the proposal becomes active (i.e. the max block size is 4MB)
        self.node.generate(2)

        current_block_count = self.node.getblockcount()

        # Create a block of size  ~3.99MB
        block = self.create_block_of_approx_max_size(4000000)

        # Verify that submitting the block does not cause any errors
        assert_equal(self.node.submitblock(bytes_to_hex_str(block.serialize())), None)

        # The block height should have increased by one
        assert_equal(self.node.getblockcount(), current_block_count+1)

        # Verify that the block is properly synced between the two nodes
        self.sync_all()



if __name__ == '__main__':
    QtumDGP4MBBlockSizeTest().main()

