#!/usr/bin/env python3

from decimal import Decimal
import http.client
import subprocess
import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.blocktools import *
from test_framework.messages import *
from test_framework.p2p import *
from test_framework.script import *
from test_framework.qtum import *



class QtumBitcoreTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [['-addrindex=1'], ['-addrindex=0']]
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        #self.skip_if_no_bitcore()
        self.skip_if_no_wallet()

    def unknown_segwit_address_test(self):
        unspent = self.nodes[0].listunspent()[0]
        script_pubkey = CScript([CScriptOp(OP_16), sha256(b"\x00")])
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(unspent['txid'], 16), unspent['vout']), b"")]
        tx.vout = [CTxOut(int(unspent['amount']*100000000 - 100000), script_pubkey)]
        tx = rpc_sign_transaction(self.nodes[0], tx)
        self.nodes[0].sendrawtransaction(bytes_to_hex_str(tx.serialize()))

        tx.rehash()
        tx2 = CTransaction()
        tx2.vin = [CTxIn(COutPoint(tx.sha256, 0), b"")]
        tx2.vout = [CTxOut(int(unspent['amount']*100000000 - 200000), script_pubkey)]
        tx.rehash()
        tip = self.nodes[0].getblock(self.nodes[0].getbestblockhash())
        block = create_block(int(self.nodes[0].getbestblockhash(), 16), create_coinbase(self.nodes[0].getblockcount()+1), tip['time'])
        block.vtx.append(tx)
        block.vtx.append(tx2)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        assert_equal(self.nodes[0].submitblock(bytes_to_hex_str(block.serialize())), None)


    def run_test(self):
        node = self.nodes[0]
        t = int(time.time())-10000
        node.setmocktime(t)
        node.generate(3990)
        for i in range(10):
            node.setmocktime(t+9900+i)
            node.generate(1)
        node.setmocktime(0)

        node.setmocktime(0)
        self.unknown_segwit_address_test()

        confirmed_address = node.getnewaddress()
        mempool_address = node.getnewaddress()
        expected_address_txids = []
        for i in range(10):
            expected_address_txids.append(node.sendtoaddress(confirmed_address, 10))
        time.sleep(0.1)
        node.generate(1)
        mempool_txid = node.sendtoaddress(mempool_address, 19999)

        # check dgp info
        ret = node.getdgpinfo()
        assert_equal(ret, {'blockgaslimit': 40000000, 'maxblocksize': 500000, 'mingasprice': 40})
        ret = node.getaddresstxids({'addresses': [confirmed_address]})
        assert_equal(set(ret), set(expected_address_txids))

        ret = node.getaddressdeltas({'addresses': [confirmed_address]})
        assert_equal(len(ret), 10)

        ret = node.getaddressbalance({'addresses': [confirmed_address]})
        assert_equal(ret['balance'], 10000000000)

        ret = node.getaddressutxos({'addresses': [confirmed_address]})

        ret = node.getaddressmempool({'addresses': [mempool_address]})
        assert_equal(ret[0]['txid'], mempool_txid)
        assert_equal(len(ret), 1)

        new_block = node.getblock(node.getblockhash(3994))
        old_block = node.getblock(node.getblockhash(3991))
        ret = node.getblockhashes(new_block['time'], old_block['time'])
        assert_equal(set(ret), set(node.getblockhash(3991+i) for i in range(3)))
        time.sleep(1)

        txinfo = node.decoderawtransaction(node.gettransaction(expected_address_txids[0])['hex'])
        spent_prevout = txinfo['vin'][0]
        ret = node.getspentinfo({"txid": spent_prevout['txid'], "index": spent_prevout['vout']})
        assert_equal(ret, {"txid": expected_address_txids[0], "index": 0, "height": 4002})
        self.sync_all()


if __name__ == '__main__':
    QtumBitcoreTest().main()