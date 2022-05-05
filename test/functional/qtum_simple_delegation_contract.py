#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.qtum import *
from test_framework.qtumconfig import *
from test_framework.util import *

class QtumSimpleDelegationContractTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [['-txindex=1', '-logevents=1'], ['-txindex=1', '-logevents=1', '-superstaking=1', '-staking=1']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        for n in self.nodes: n.setmocktime(int(time.time()) - 1000000)

        self.delegator = self.nodes[0]
        delegator_address = self.delegator.getnewaddress()
        delegator_address_hex = p2pkh_to_hex_hash(delegator_address)

        self.staker = self.nodes[1]
        staker_address = self.staker.getnewaddress()
        staker_privkey = self.staker.dumpprivkey(staker_address)
        staker_eckey = wif_to_ECKey(staker_privkey)

        self.staker.generatetoaddress(1, staker_address)
        self.sync_all()
        generatesynchronized(self.delegator, COINBASE_MATURITY+100, delegator_address, self.nodes)
        self.sync_all()
        generatesynchronized(self.staker, COINBASE_MATURITY, staker_address, self.nodes)
        self.sync_all()

        # Proof of delegation
        pod = create_POD(self.delegator, delegator_address, staker_address)
        
        # only 1 percent will go to the delegator
        fee = 99
        # send the evm tx that delegates the address to the staker address
        delegate_to_staker(self.delegator, delegator_address, staker_address, fee, pod)
        self.delegator.generatetoaddress(1, delegator_address)
        self.sync_all()

        delegator_prevouts = collect_prevouts(self.delegator)
        staker_prevouts = collect_prevouts(self.staker, min_confirmations=0, min_amount=100)
        staker_prevout_for_nas = staker_prevouts.pop(0)[0]

        tip = self.staker.getblock(self.staker.getbestblockhash())
        nTime = (tip['time'] + TIMESTAMP_MASK+1) & (0xffffffff - TIMESTAMP_MASK)
        for n in self.nodes: n.setmocktime(nTime+100)
        block = create_delegated_pos_block(self.staker, staker_eckey, staker_prevout_for_nas, delegator_address_hex, pod, fee, delegator_prevouts, nFees=0, nTime=nTime)
        assert_equal(self.staker.submitblock(bytes_to_hex_str(block.serialize())), None)
        assert_equal(self.staker.getbestblockhash(), block.hash)



if __name__ == '__main__':
    QtumSimpleDelegationContractTest().main()
