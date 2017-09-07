#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.qtum import *
from test_framework.address import *
import time

class QtumCombinedOutputsExceedGasLimitTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        self.node = self.nodes[0]

    def run_test(self):
        self.node.generate(10+COINBASE_MATURITY)
        """
        pragma solidity ^0.4.12;

        contract Test {
            uint stateChanger;
            function () payable {
                stateChanger += 1;
                while(msg.gas > 1000) {}
            }
        }
        """        
        contract_bytecode = "60606040523415600e57600080fd5b5b605380601c6000396000f30060606040525b5b600160008082825401925050819055505b6103e85a11156024576017565b5b0000a165627a7a723058209aba4fa1dc462e2f52351eade3edc2974f25ddc404d415ce54df8d02eba21dd10029"
        contract_address = self.node.createcontract(contract_bytecode)['address']
        self.node.generate(1)
        tx = CTransaction()
        tx.vin = [make_vin(self.node, int(20000*COIN))]
        tx.vout = [
            CTxOut(0, CScript([b"\x04", CScriptNum(19998999), CScriptNum(1), hex_str_to_bytes("00"), hex_str_to_bytes(contract_address), OP_CALL])),
            CTxOut(0, CScript([b"\x04", CScriptNum(19998999), CScriptNum(1), hex_str_to_bytes("00"), hex_str_to_bytes(contract_address), OP_CALL])),
            CTxOut(0, CScript([b"\x04", CScriptNum(19998999), CScriptNum(1), hex_str_to_bytes("00"), hex_str_to_bytes(contract_address), OP_CALL]))
        ]
        signed_tx_raw = self.node.signrawtransaction(bytes_to_hex_str(tx.serialize()))['hex']
        #
        assert_raises(JSONRPCException, self.node.sendrawtransaction, signed_tx_raw)
        block_count = self.node.getblockcount()
        self.node.generate(2)
        assert_equal(self.node.getblockcount(), block_count+2)

if __name__ == '__main__':
    QtumCombinedOutputsExceedGasLimitTest().main()
