#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.qtum import *
from test_framework.address import *
from test_framework.blocktools import *
import io

"""
Note, these tests do not test the functionality of the DGP template contract itself, for tests for the DGP template, see qtum-dgp.py
"""
class QtumDGPGasSchedule(BitcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [['-dgpevm'], ['-dgpstorage']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()


    def create_proposal_contract(self):
        """
        pragma solidity ^0.4.11;
        contract gasSchedule{
            uint32[39] _gasSchedule=[
                10, //0: tierStepGas0
                10, //1: tierStepGas1
                10, //2: tierStepGas2
                10, //3: tierStepGas3
                10, //4: tierStepGas4
                10, //5: tierStepGas5
                10, //6: tierStepGas6
                10, //7: tierStepGas7
                10, //8: expGas
                50, //9: expByteGas
                30, //10: sha3Gas
                6, //11: sha3WordGas
                200, //12: sloadGas
                201, //13: sstoreSetGas
                5000, //14: sstoreResetGas
                15000, //15: sstoreRefundGas
                1, //16: jumpdestGas
                374000, //17: logGas
                8, //18: logDataGas
                375, //19: logTopicGas
                32000, //20: createGas
                700, //21: callGas
                2300, //22: callStipend
                9000, //23: callValueTransferGas
                25000, //24: callNewAccountGas
                24000, //25: suicideRefundGas
                3, //26: memoryGas
                512, //27: quadCoeffDiv
                200, //28: createDataGas
                21000, //29: txGas
                53000, //30: txCreateGas
                4, //31: txDataZeroGas
                68, //32: txDataNonZeroGas
                3, //33: copyGas
                700, //34: extcodesizeGas
                700, //35: extcodecopyGas
                400, //36: balanceGas
                5000, //37: suicideGas
                24576 //38: maxCodeSize
            ];
            function getSchedule() constant returns(uint32[39] _schedule){
                return _gasSchedule;
            }
        }
        """

        contract_data = self.node.createcontract("60806040526104e060405190810160405280600a62ffffff168152602001600a62ffffff168152602001600a62ffffff168152602001600a62ffffff168152602001600a62ffffff168152602001600a62ffffff168152602001600a62ffffff168152602001600a62ffffff168152602001600a62ffffff168152602001603262ffffff168152602001601e62ffffff168152602001600662ffffff16815260200160c862ffffff16815260200160c962ffffff16815260200161138862ffffff168152602001613a9862ffffff168152602001600162ffffff1681526020016205b4f062ffffff168152602001600862ffffff16815260200161017762ffffff168152602001617d0062ffffff1681526020016102bc62ffffff1681526020016108fc62ffffff16815260200161232862ffffff1681526020016161a862ffffff168152602001615dc062ffffff168152602001600362ffffff16815260200161020062ffffff16815260200160c862ffffff16815260200161520862ffffff16815260200161cf0862ffffff168152602001600462ffffff168152602001604462ffffff168152602001600362ffffff1681526020016102bc62ffffff1681526020016102bc62ffffff16815260200161019062ffffff16815260200161138862ffffff16815260200161600062ffffff168152506000906027610206929190610219565b5034801561021357600080fd5b506102ee565b8260276007016008900481019282156102aa5791602002820160005b8382111561027857835183826101000a81548163ffffffff021916908362ffffff1602179055509260200192600401602081600301049283019260010302610235565b80156102a85782816101000a81549063ffffffff0219169055600401602081600301049283019260010302610278565b505b5090506102b791906102bb565b5090565b6102eb91905b808211156102e757600081816101000a81549063ffffffff0219169055506001016102c1565b5090565b90565b610160806102fd6000396000f300608060405260043610610041576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806326fadbe214610046575b600080fd5b34801561005257600080fd5b5061005b610099565b6040518082602760200280838360005b8381101561008657808201518184015260208101905061006b565b5050505090500191505060405180910390f35b6100a1610110565b6000602780602002604051908101604052809291908260278015610106576020028201916000905b82829054906101000a900463ffffffff1663ffffffff16815260200190600401906020826003010492830192600103820291508084116100c95790505b5050505050905090565b6104e0604051908101604052806027906020820280388339808201915050905050905600a165627a7a723058205e249731b14c9492ca6a161a7342bd0796c89a7eea6a30255be7fe5c0ee8995a0029", 10000000)
        self.proposal_address = contract_data['address']


    def run_test(self):
        # Generate some blocks to make sure we have enough spendable outputs
        self.node = self.nodes[0]
        generatesynchronized(self.node, 1+COINBASE_MATURITY, None, self.nodes)
        self.node.sendtoaddress(self.nodes[1].getnewaddress(), INITIAL_BLOCK_REWARD-1)
        generatesynchronized(self.node, COINBASE_MATURITY, None, self.nodes)
        self.BLOCK_SIZE_DGP = DGPState(self.node, "0000000000000000000000000000000000000080")
        # Start off by setting ourself as admin
        admin_address = self.node.getnewaddress()

        # Set ourself up as admin
        self.BLOCK_SIZE_DGP.send_set_initial_admin(admin_address)
        self.node.generate(1)
        self.create_proposal_contract()
        self.BLOCK_SIZE_DGP.send_add_address_proposal(self.proposal_address, 2, admin_address)
        self.node.generate(2) # We need to generate 2 blocks now for it to activate

        for i in range(1, self.node.getblockcount()+1):
            block_raw = self.node.getblock(self.node.getblockhash(i), False)
            self.nodes[1].submitblock(block_raw)
        """
        pragma solidity ^0.4.0;
        contract Test {
            mapping(uint => uint) store;
            constructor() public {
                for(uint i = 1; i < 100; ++i) {
                    store[i] = i;
                }
            }
        }
        """
        bytecode = "6080604052348015600f57600080fd5b506000600190505b606481101560405780600080838152602001908152602001600020819055508060010190506017565b50603580604e6000396000f3006080604052600080fd00a165627a7a72305820322822a2891bf2231763465379f848c02acc8a6b6a468faed8193987b355199a0029"

        for node, opt in zip(self.nodes, ['dgpevm', 'dgpstorage']):
            contract_address = node.createcontract(bytecode, 1000000)['address']
            node.generate(1)
            print('dgp worked with ', opt, contract_address in node.listcontracts())
            self.sync_all()

if __name__ == '__main__':
    QtumDGPGasSchedule().main()
