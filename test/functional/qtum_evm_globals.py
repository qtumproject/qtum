#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.address import *
from test_framework.qtum import *
from test_framework.qtumconfig import *
from test_framework.blocktools import *

from test_framework.address import *
import sys
import random
import time

class QtumEVMGlobalsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [['-staking=1', '-muirglacierheight=100000'], ['-muirglacierheight=100000']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

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
        Function signatures:
        
            9950fc69: blockblockhash(uint256)
            18b66ea3: blockcoinbase()
            5df32c47: blockdifficulty()
            805f6831: blockgaslimit()
            05062247: blocknumber()
            8182a11f: blocktimestamp()
            cd84980e: chainid()
            e9ac31f2: msgdata()
            522a3926: msggas()
            61c99b92: msgsender()
            96acd236: msgsig()
            f8184f73: msgvalue()
            cc5ea9ad: setGlobals()
            d76c09ad: txgasprice()
            2c7622b0: txorigin()
        """
        self.node.sendtocontract(self.contract_address, "cc5ea9ad", 1, 20000000, QTUM_MIN_GAS_PRICE/COIN, sender)

        if use_staking:
            t = (self.node.getblock(self.node.getbestblockhash())['time']+100) & 0xfffffff0
            for n in self.nodes: n.setmocktime(t)

            blockcount = self.node.getblockcount()
            for t in range(t, t+100):
                for n in self.nodes: n.setmocktime(t)
                if blockcount < self.node.getblockcount():
                    break
                print('staking', t, self.node.getstakinginfo())
                time.sleep(1)
            else:
                assert(False)
            blockhash = self.node.getblockhash(blockcount+1)
            authorTxIndexAndVoutIndex = 1
        else:
            blockhash = self.node.generate(1)[0]
            authorTxIndexAndVoutIndex = 0
        

        block = self.node.getblock(blockhash)
        blocktxids = block['tx']
        coinbase_tx = self.node.decoderawtransaction(self.node.gettransaction(blocktxids[authorTxIndexAndVoutIndex])['hex'])
        # coinbase_pkh refers to either the coinbase for pow or coinstake for pos
        if use_staking:
            coinstake_pubkey = hex_str_to_bytes(coinbase_tx['vout'][authorTxIndexAndVoutIndex]['scriptPubKey']['asm'].split(' ')[0])
            coinbase_pkh = bytes_to_hex_str(hash160(coinstake_pubkey))
        else:
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
        
        # block.chainid
        print('  block.chainid')
        assert_equal(8890, int(self.get_contract_call_output("cd84980e"), 16))

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
        self.node = self.nodes[0]
        self.connect_nodes(0, 1)
        address = self.node.getnewaddress()
        generatesynchronized(self.node, 10 + COINBASE_MATURITY, address, self.nodes)

        """
        pragma solidity >=0.8.0;
        
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
            uint public chainid;
        
            function setGlobals() public payable {
                for(uint i = 0; i <= 257; ++i) {
                    blockblockhash[block.number - i] = blockhash(block.number - i);
                }
        
                blockcoinbase = block.coinbase;
        
                blockdifficulty  = block.prevrandao;
        
                blockgaslimit = block.gaslimit;
                
                blocknumber = block.number;
        
                blocktimestamp = block.timestamp;
                
                chainid = block.chainid;
        
                msgdata = msg.data;
        
                msggas = gasleft();
        
                msgsender = msg.sender;
        
                msgsig = msg.sig;
        
                msgvalue = msg.value;
        
                txgasprice = tx.gasprice;
        
                txorigin = tx.origin;
            }
        }

        """
        bytecode = "608060405234801561000f575f80fd5b50610c258061001d5f395ff3fe6080604052600436106100e7575f3560e01c80638182a11f11610089578063cd84980e11610058578063cd84980e146102ab578063d76c09ad146102d5578063e9ac31f2146102ff578063f8184f7314610329576100e7565b80638182a11f1461021157806396acd2361461023b5780639950fc6914610265578063cc5ea9ad146102a1576100e7565b8063522a3926116100c5578063522a3926146101695780635df32c471461019357806361c99b92146101bd578063805f6831146101e7576100e7565b806305062247146100eb57806318b66ea3146101155780632c7622b01461013f575b5f80fd5b3480156100f6575f80fd5b506100ff610353565b60405161010c9190610652565b60405180910390f35b348015610120575f80fd5b50610129610359565b60405161013691906106aa565b60405180910390f35b34801561014a575f80fd5b5061015361037c565b60405161016091906106aa565b60405180910390f35b348015610174575f80fd5b5061017d6103a1565b60405161018a9190610652565b60405180910390f35b34801561019e575f80fd5b506101a76103a7565b6040516101b49190610652565b60405180910390f35b3480156101c8575f80fd5b506101d16103ad565b6040516101de91906106aa565b60405180910390f35b3480156101f2575f80fd5b506101fb6103d2565b6040516102089190610652565b60405180910390f35b34801561021c575f80fd5b506102256103d8565b6040516102329190610652565b60405180910390f35b348015610246575f80fd5b5061024f6103de565b60405161025c91906106fd565b60405180910390f35b348015610270575f80fd5b5061028b60048036038101906102869190610744565b6103f1565b6040516102989190610787565b60405180910390f35b6102a9610406565b005b3480156102b6575f80fd5b506102bf61059c565b6040516102cc9190610652565b60405180910390f35b3480156102e0575f80fd5b506102e96105a2565b6040516102f69190610652565b60405180910390f35b34801561030a575f80fd5b506103136105a8565b604051610320919061082a565b60405180910390f35b348015610334575f80fd5b5061033d610634565b60405161034a9190610652565b60405180910390f35b60045481565b5f8054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b600b5f9054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b60075481565b60025481565b60085f9054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b60035481565b60055481565b600860149054906101000a900460e01b81565b6001602052805f5260405f205f915090505481565b5f5b610101811161044f57804361041d9190610877565b4060015f834361042d9190610877565b81526020019081526020015f208190555080610448906108aa565b9050610408565b50415f806101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055504460028190555045600381905550436004819055504260058190555046600c819055505f36600691826104c3929190610b22565b505a6007819055503360085f6101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505f357fffffffff0000000000000000000000000000000000000000000000000000000016600860146101000a81548163ffffffff021916908360e01c0217905550346009819055503a600a8190555032600b5f6101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550565b600c5481565b600a5481565b600680546105b590610955565b80601f01602080910402602001604051908101604052809291908181526020018280546105e190610955565b801561062c5780601f106106035761010080835404028352916020019161062c565b820191905f5260205f20905b81548152906001019060200180831161060f57829003601f168201915b505050505081565b60095481565b5f819050919050565b61064c8161063a565b82525050565b5f6020820190506106655f830184610643565b92915050565b5f73ffffffffffffffffffffffffffffffffffffffff82169050919050565b5f6106948261066b565b9050919050565b6106a48161068a565b82525050565b5f6020820190506106bd5f83018461069b565b92915050565b5f7fffffffff0000000000000000000000000000000000000000000000000000000082169050919050565b6106f7816106c3565b82525050565b5f6020820190506107105f8301846106ee565b92915050565b5f80fd5b6107238161063a565b811461072d575f80fd5b50565b5f8135905061073e8161071a565b92915050565b5f6020828403121561075957610758610716565b5b5f61076684828501610730565b91505092915050565b5f819050919050565b6107818161076f565b82525050565b5f60208201905061079a5f830184610778565b92915050565b5f81519050919050565b5f82825260208201905092915050565b5f5b838110156107d75780820151818401526020810190506107bc565b5f8484015250505050565b5f601f19601f8301169050919050565b5f6107fc826107a0565b61080681856107aa565b93506108168185602086016107ba565b61081f816107e2565b840191505092915050565b5f6020820190508181035f83015261084281846107f2565b905092915050565b7f4e487b71000000000000000000000000000000000000000000000000000000005f52601160045260245ffd5b5f6108818261063a565b915061088c8361063a565b92508282039050818111156108a4576108a361084a565b5b92915050565b5f6108b48261063a565b91507fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff82036108e6576108e561084a565b5b600182019050919050565b5f82905092915050565b7f4e487b71000000000000000000000000000000000000000000000000000000005f52604160045260245ffd5b7f4e487b71000000000000000000000000000000000000000000000000000000005f52602260045260245ffd5b5f600282049050600182168061096c57607f821691505b60208210810361097f5761097e610928565b5b50919050565b5f819050815f5260205f209050919050565b5f6020601f8301049050919050565b5f82821b905092915050565b5f600883026109e17fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff826109a6565b6109eb86836109a6565b95508019841693508086168417925050509392505050565b5f819050919050565b5f610a26610a21610a1c8461063a565b610a03565b61063a565b9050919050565b5f819050919050565b610a3f83610a0c565b610a53610a4b82610a2d565b8484546109b2565b825550505050565b5f90565b610a67610a5b565b610a72818484610a36565b505050565b5b81811015610a9557610a8a5f82610a5f565b600181019050610a78565b5050565b601f821115610ada57610aab81610985565b610ab484610997565b81016020851015610ac3578190505b610ad7610acf85610997565b830182610a77565b50505b505050565b5f82821c905092915050565b5f610afa5f1984600802610adf565b1980831691505092915050565b5f610b128383610aeb565b9150826002028217905092915050565b610b2c83836108f1565b67ffffffffffffffff811115610b4557610b446108fb565b5b610b4f8254610955565b610b5a828285610a99565b5f601f831160018114610b87575f8415610b75578287013590505b610b7f8582610b07565b865550610be6565b601f198416610b9586610985565b5f5b82811015610bbc57848901358255600182019150602085019450602081019050610b97565b86831015610bd95784890135610bd5601f891682610aeb565b8355505b6001600288020188555050505b5050505050505056fea2646970667358221220b2e1918d734a0a91404c986957651f9fc3b565ef079657229494c330769c740d64736f6c63430008150033"
        self.contract_address = self.node.createcontract(bytecode)['address']
        print('verify globals in PoW blocks')

        self.verify_evm_globals_test(use_staking=True)
        self.sync_all()
        
        generatesynchronized(self.node, 257, None, self.nodes)
        self.sync_all()

        for n in self.nodes:
            n.setmocktime((self.node.getblock(self.node.getbestblockhash())['time']+100) & 0xfffffff0)

        print(self.node.getblockcount())
        print('verify globals in PoS blocks')
        self.verify_evm_globals_test(use_staking=True)
        self.sync_all()

        generatesynchronized(self.node, 257, None, self.nodes)
        self.sync_all()

        for n in self.nodes:
            n.setmocktime((self.node.getblock(self.node.getbestblockhash())['time']+100) & 0xfffffff0)

        self.verify_evm_globals_test(use_staking=True)
        self.sync_all()

if __name__ == '__main__':
    QtumEVMGlobalsTest().main()
