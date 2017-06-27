#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.blocktools import *
from test_framework.qtum import *
import sys
import io

class QtumDupeContractTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [[]])
        self.node = self.nodes[0]

    def run_test(self):
        self.node.generate(10+COINBASE_MATURITY)
        tx = CTransaction()
        tx.vin = [make_vin(self.node, 6*COIN)]
        tx.vout = []
        
        # First we append two OP_CREATE outputs (vout = 0, 1 (mod 256))
        tx.vout.append(CTxOut(0, CScript([1, 1000000, 1, bytes.fromhex("60606040523415600b57fe5b5b60398060196000396000f30060606040525b600b5b5b565b0000a165627a7a72305820e3bed070fd3a81dd00e02efd22d18a3b47b70860155d6063e47e1e2674fc5acb0029"), OP_CREATE])))
        tx.vout.append(CTxOut(0, CScript([1, 1000000, 1, bytes.fromhex("60606040523415600b57fe5b5b60398060196000396000f30060606040525b600b5b5b565b0000a165627a7a72305820e3bed070fd3a81dd00e02efd22d18a3b47b70860155d6063e47e1e2674fc5acb0029"), OP_CREATE])))

        # Now we append 255 vouts that spend no gas
        for _ in range(2, 256):
            tx.vout.append(CTxOut(1000000, CScript([OP_TRUE])))

        # Now we append another two more vouts that should create contracts (vout = 0, 1 (mod 256))
        tx.vout.append(CTxOut(0, CScript([1, 1000000, 1, bytes.fromhex("60606040523415600b57fe5b5b60398060196000396000f30060606040525b600b5b5b565b0000a165627a7a72305820e3bed070fd3a81dd00e02efd22d18a3b47b70860155d6063e47e1e2674fc5acb0029"), OP_CREATE])))
        tx.vout.append(CTxOut(0, CScript([1, 1000000, 1, bytes.fromhex("60606040523415600b57fe5b5b60398060196000396000f30060606040525b600b5b5b565b0000a165627a7a72305820e3bed070fd3a81dd00e02efd22d18a3b47b70860155d6063e47e1e2674fc5acb0029"), OP_CREATE])))

        ret = self.node.signrawtransaction(bytes_to_hex_str(tx.serialize()))
        self.node.sendrawtransaction(ret['hex'])
        self.node.generate(1)

        # This should result in 4 contracts being created.
        assert_equal(len(self.node.listcontracts()), 4)





if __name__ == '__main__':
    QtumDupeContractTest().main()
