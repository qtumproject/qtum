#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.qtum import activate_mpos
from test_framework.qtumconfig import COINBASE_MATURITY
from test_framework.address import byte_to_base58
from test_framework.messages import hash256
import time
import struct

class QtumCallContractTimestampTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [['-lastmposheight=999999']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.node = self.nodes[0]
        privkey = byte_to_base58(hash256(struct.pack('<I', 0)), 239)
        self.node.importprivkey(privkey)

        self.node.generatetoaddress(100 + COINBASE_MATURITY, "qSrM9K6FMhZ29Vkp8Rdk8Jp66bbfpjFETq")

        """
        pragma solidity ^0.4.11;
        contract Example {
            function timestamp() external returns(uint) {
                return now;
            }
        }
        """
        bytecode = "60606040523415600b57fe5b5b60928061001a6000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff168063b80777ea14603a575bfe5b3415604157fe5b6047605d565b6040518082815260200191505060405180910390f35b60004290505b905600a165627a7a7230582022b5728b8ca07de23857473e303660ad554d6344c64658ab692d741fa8753b380029"
        self.contract_address = self.node.createcontract(bytecode)['address']
        self.node.generate(1)
        now = int(time.time())
        expected_now = int(self.node.callcontract(self.contract_address, "b80777ea")['executionResult']['output'], 16)
        print(now, expected_now)
        assert(expected_now == now or expected_now == now+1)
        activate_mpos(self.node)
        self.node.setmocktime(0)
        now = int(time.time())
        expected_now = int(self.node.callcontract(self.contract_address, "b80777ea")['executionResult']['output'], 16)
        print(now, expected_now)
        assert(expected_now == now or expected_now == now+1)

if __name__ == '__main__':
    QtumCallContractTimestampTest().main()
