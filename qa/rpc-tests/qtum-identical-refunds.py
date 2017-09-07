#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.qtum import *
from test_framework.blocktools import *
import time
import io

class QtumIdenticalRefunds(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [[]])
        self.node = self.nodes[0]

    def run_test(self):
        self.node.generate(100+COINBASE_MATURITY)
        """
        contract Test {
            function() payable {}
        }
        """
        code = "60606040523415600e57600080fd5b5b603980601c6000396000f30060606040525b600b5b5b565b0000a165627a7a72305820e28f464040162c6b98c6be21a9de622f2c8f102259a90476df697d0beb9ac9880029"
        contract_address = self.node.createcontract(code)['address']
        self.node.generate(1)

        # Send 2 evm txs that will call the fallback function of the Test contract with the same sender.
        # This will result in the same amount of gas being spent and thus the same amount being refunded.
        sender_address = self.node.getnewaddress()
        self.node.sendtoaddress(sender_address, 1)
        self.node.sendtocontract(contract_address, "00", 0, 1000000, QTUM_MIN_GAS_PRICE/COIN, sender_address)
        self.node.sendtoaddress(sender_address, 1)
        self.node.sendtocontract(contract_address, "00", 0, 1000000, QTUM_MIN_GAS_PRICE/COIN, sender_address)

        # Check that all txs were accepted into the mempool.
        assert_equal(len(self.node.getrawmempool()), 4)
        block_hash = self.node.generate(1)[0]

        # Verify that all txs were included in the block.
        assert_equal(len(self.node.getrawmempool()), 0)

        # Fetch the last generated block containing the txs and refund outputs and deserialize it.
        manipulation_block = CBlock()
        block_raw = self.node.getblock(block_hash, False)
        f = io.BytesIO(hex_str_to_bytes(block_raw))
        manipulation_block.deserialize(f)

        # Make sure that all expected outputs are present in the coinbase
        assert_equal(len(manipulation_block.vtx[0].vout), 4)
        assert_equal(manipulation_block.vtx[0].vout[-2].scriptPubKey == manipulation_block.vtx[0].vout[0].scriptPubKey, False)

        # Now we change the last refund output's scriptPubKey to the miner's scriptPubKey.
        manipulation_block.vtx[0].vout[-2].scriptPubKey = manipulation_block.vtx[0].vout[0].scriptPubKey
        manipulation_block.hashMerkleRoot = manipulation_block.calc_merkle_root()
        manipulation_block.solve()
        
        # Invalidate the last block so that we can submit the manipulated block.
        self.node.invalidateblock(self.node.getbestblockhash())

        block_count = self.node.getblockcount()

        # Resubmit the manipulated block.
        ret = self.node.submitblock(bytes_to_hex_str(manipulation_block.serialize()))

        # Check that the block was not accepted
        assert_equal(ret is None, False)

        # Make sure that the block was not accepted.
        assert_equal(self.node.getblockcount(), block_count)
        
if __name__ == '__main__':
    QtumIdenticalRefunds().main()
