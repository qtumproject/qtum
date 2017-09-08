#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.address import *
from test_framework.qtum import *
from test_framework.blocktools import *

from test_framework.address import *
import sys
import random
import time

class QtumEVMGlobalsTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [['-staking=1'], []])
        self.is_network_split = False
        self.node = self.nodes[0]
        connect_nodes_bi(self.nodes, 0, 1)


    def get_contract_call_output(self, abi_param):
        out = self.node.callcontract(self.contract_address, abi_param)
        assert_equal(out['executionResult']['excepted'], 'None')
        return out['executionResult']['output']

    def verify_evm_globals_test(self, use_staking=False):
        sender = self.node.getnewaddress()
        sender_pkh = p2pkh_to_hex_hash(sender)
        self.node.sendtoaddress(sender, 1)
        self.node.generate(1)
        """
            9950fc69 blockblockhash(uint256)
            18b66ea3 blockcoinbase()
            5df32c47 blockdifficulty()
            805f6831 blockgaslimit()
            05062247 blocknumber()
            8182a11f blocktimestamp()
            e9ac31f2 msgdata()
            522a3926 msggas()
            61c99b92 msgsender()
            96acd236 msgsig()
            f8184f73 msgvalue()
            cc5ea9ad setGlobals()
            d76c09ad txgasprice()
            2c7622b0 txorigin()
        """
        self.node.sendtocontract(self.contract_address, "cc5ea9ad", 1, 20000000, QTUM_MIN_GAS_PRICE/COIN, sender)

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

        #for i in range(self.node.getblockcount(), 0, -1):
        #    print(i, self.get_contract_call_output("9950fc69" + hex(i)[2:].zfill(64)))

        # Out of range blocks (current and too old)
        for i in [0, 257]:
            blockindex = self.node.getblockcount()-i
            evm_blockhash = self.get_contract_call_output("9950fc69" + hex(blockindex)[2:].zfill(64))
            assert_equal(int(evm_blockhash, 16), 0)

        print('  block.blockhash(block.number - i) 0<i<257')
        for i in range(1, 257):
            blockindex = self.node.getblockcount()-i
            evm_blockhash = self.get_contract_call_output("9950fc69" + hex(blockindex)[2:].zfill(64))
            rpc_blockhash = bytes_to_hex_str(hex_str_to_bytes(self.node.getblockhash(blockindex))[::-1])
            assert_equal(evm_blockhash, rpc_blockhash)

        # block.coinbase
        print('  block.coinbase')
        assert_equal(coinbase_pkh, self.get_contract_call_output("18b66ea3")[24:])

        # block.difficulty
        print('  block.difficulty')
        assert_equal(int(block['bits'], 16), int(self.get_contract_call_output("5df32c47"), 16))

        # block.gaslimit
        print('  block.gaslimit')
        assert_equal(40000000, int(self.get_contract_call_output("805f6831"), 16))

        # block.number
        print('  block.number')
        assert_equal(block['height'], int(self.get_contract_call_output("05062247"), 16))

        # block.timestamp
        print('  block.timestamp')
        assert_equal(block['time'], int(self.get_contract_call_output("8182a11f"), 16))

        # msg.gas
        print('  msg.gas')
        assert(int(self.get_contract_call_output("522a3926"), 16))

        # msg.sender
        print('  msg.sender')
        assert_equal(sender_pkh, self.get_contract_call_output("61c99b92")[24:])

        # msg.sig, function signature
        print('  msg.sig')
        assert_equal("cc5ea9ad", self.get_contract_call_output("96acd236")[0:8])

        # msg.value
        print('  msg.value')
        assert_equal(int(COIN), int(self.get_contract_call_output("f8184f73"), 16))

        # tx.gasprice
        print('  tx.gasprice')
        assert_equal(QTUM_MIN_GAS_PRICE, int(self.get_contract_call_output("d76c09ad"), 16))

        # tx.origin
        print('  tx.origin')
        assert_equal(sender_pkh, self.get_contract_call_output("2c7622b0")[24:])
        self.sync_all()


    def run_test(self):
        self.node.generate(10 + COINBASE_MATURITY)

        """
        pragma solidity ^0.4.12;


        contract TestEVM {
            address public blockcoinbase;
            mapping(uint => bytes32) public blockblockhash;
            uint public blockdifficulty;
            uint public blockgaslimit;
            uint public blocknumber;
            uint public blocktimestamp;
            bytes public msgdata;
            uint public msggas;
            address public msgsender;
            bytes4 public msgsig;
            uint public msgvalue;
            uint public txgasprice;
            address public txorigin;

            function setGlobals() payable {
                for(uint i = 0; i <= 257; ++i) {
                    blockblockhash[block.number - i] = block.blockhash(block.number - i);
                }

                blockcoinbase = block.coinbase;

                blockdifficulty  = block.difficulty;

                blockgaslimit = block.gaslimit;
                
                blocknumber = block.number;

                blocktimestamp = block.timestamp;

                msgdata = msg.data;

                msggas = msg.gas;

                msgsender = msg.sender;

                msgsig = msg.sig;

                msgvalue = msg.value;

                txgasprice = tx.gasprice;

                txorigin = tx.origin;
            }
        }
        """
        bytecode = "6060604052341561000f57600080fd5b5b6108298061001f6000396000f300606060405236156100ce576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806305062247146100d357806318b66ea3146100fc5780632c7622b014610151578063522a3926146101a65780635df32c47146101cf57806361c99b92146101f8578063805f68311461024d5780638182a11f1461027657806396acd2361461029f5780639950fc6914610306578063cc5ea9ad14610345578063d76c09ad1461034f578063e9ac31f214610378578063f8184f7314610407575b600080fd5b34156100de57600080fd5b6100e6610430565b6040518082815260200191505060405180910390f35b341561010757600080fd5b61010f610436565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b341561015c57600080fd5b61016461045b565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b34156101b157600080fd5b6101b9610481565b6040518082815260200191505060405180910390f35b34156101da57600080fd5b6101e2610487565b6040518082815260200191505060405180910390f35b341561020357600080fd5b61020b61048d565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b341561025857600080fd5b6102606104b3565b6040518082815260200191505060405180910390f35b341561028157600080fd5b6102896104b9565b6040518082815260200191505060405180910390f35b34156102aa57600080fd5b6102b26104bf565b60405180827bffffffffffffffffffffffffffffffffffffffffffffffffffffffff19167bffffffffffffffffffffffffffffffffffffffffffffffffffffffff1916815260200191505060405180910390f35b341561031157600080fd5b61032760048080359060200190919050506104ee565b60405180826000191660001916815260200191505060405180910390f35b61034d610506565b005b341561035a57600080fd5b6103626106ae565b6040518082815260200191505060405180910390f35b341561038357600080fd5b61038b6106b4565b6040518080602001828103825283818151815260200191508051906020019080838360005b838110156103cc5780820151818401525b6020810190506103b0565b50505050905090810190601f1680156103f95780820380516001836020036101000a031916815260200191505b509250505060405180910390f35b341561041257600080fd5b61041a610752565b6040518082815260200191505060405180910390f35b60045481565b6000809054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b600b60009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b60075481565b60025481565b600860009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b60035481565b60055481565b600860149054906101000a90047c01000000000000000000000000000000000000000000000000000000000281565b60016020528060005260406000206000915090505481565b6000600190505b61010081111515610545578043034060016000834303815260200190815260200160002081600019169055505b80600101905061050d565b416000806101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555044600281905550456003819055504360048190555042600581905550600036600691906105b3929190610758565b505a60078190555033600860006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055506000357fffffffff0000000000000000000000000000000000000000000000000000000016600860146101000a81548163ffffffff02191690837c010000000000000000000000000000000000000000000000000000000090040217905550346009819055503a600a8190555032600b60006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b50565b600a5481565b60068054600181600116156101000203166002900480601f01602080910402602001604051908101604052809291908181526020018280546001816001161561010002031660029004801561074a5780601f1061071f5761010080835404028352916020019161074a565b820191906000526020600020905b81548152906001019060200180831161072d57829003601f168201915b505050505081565b60095481565b828054600181600116156101000203166002900490600052602060002090601f016020900481019282601f1061079957803560ff19168380011785556107c7565b828001600101855582156107c7579182015b828111156107c65782358255916020019190600101906107ab565b5b5090506107d491906107d8565b5090565b6107fa91905b808211156107f65760008160009055506001016107de565b5090565b905600a165627a7a7230582035fd01aecdb713f1bf582f346a4ffe69efe883d4c8744bbd5262505e0d13dfb20029"
        self.contract_address = self.node.createcontract(bytecode)['address']
        print('verify globals in PoW blocks')

        self.verify_evm_globals_test(use_staking=False)
        self.sync_all()
        
        self.node.generate(257)
        self.sync_all()

        for n in self.nodes:
            n.setmocktime((self.node.getblock(self.node.getbestblockhash())['time']+100) & 0xfffffff0)

        print('verify globals in PoS blocks')
        self.verify_evm_globals_test(use_staking=True)
        self.sync_all()

        self.node.generate(257)
        self.sync_all()

        print('verify globals in MPoS blocks')
        self.node.generate(4999 - self.node.getblockcount())
        self.sync_all()

        for n in self.nodes:
            n.setmocktime((self.node.getblock(self.node.getbestblockhash())['time']+100) & 0xfffffff0)

        self.verify_evm_globals_test(use_staking=True)
        self.sync_all()

if __name__ == '__main__':
    QtumEVMGlobalsTest().main()
