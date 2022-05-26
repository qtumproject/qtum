#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.blocktools import *
from test_framework.p2p import *
from test_framework.address import *
from test_framework.qtum import *
from test_framework.script import *
import time
import pprint
pp = pprint.PrettyPrinter()

"""
qrc20name
qrc20symbol
qrc20totalsupply
qrc20decimals

qrc20balanceof
qrc20allowance
qrc20approve
qrc20transfer
qrc20transferfrom
qrc20burn
qrc20burnfrom
qrc20listtransactions
"""


class QtumQRC20Test(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [['-txindex', '-logevents']]*2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def qrc20name_test(self):
        assert_equal(self.node.qrc20name(self.contract_address), "QRC TEST")

    def qrc20symbol_test(self):
        assert_equal(self.node.qrc20symbol(self.contract_address), "QTC")

    def qrc20totalsupply_test(self):
        assert_equal(self.node.qrc20totalsupply(self.contract_address), f"{10**65}.00000000")

    def qrc20decimals_test(self):
        assert_equal(self.node.qrc20decimals(self.contract_address), 8)

    def double_spend_tx(self, origin_node, txid, signer_address, double_spending_node):
        tx_to_double_spend_raw_hex = origin_node.getrawtransaction(txid)
        f = io.BytesIO(hex_str_to_bytes(tx_to_double_spend_raw_hex))
        tx_to_double_spend = CTransaction()
        tx_to_double_spend.deserialize(f)
        signer_privkey = origin_node.dumpprivkey(signer_address)

        value = sum(vout.nValue for vout in tx_to_double_spend.vout)
        tx = CTransaction()
        tx.vin = [CTxIn(vin.prevout) for vin in tx_to_double_spend.vin]
        tx.vout = [CTxOut(value, scriptPubKey=CScript([OP_RETURN, b"\x00"]))]
        tx_signed_hex = double_spending_node.signrawtransactionwithkey(tx.serialize().hex(), [signer_privkey])['hex']
        double_spending_node.sendrawtransaction(tx_signed_hex)


    def run_test(self):
        self.node = self.nodes[0]
        self.reorg_node = self.nodes[1]
        self.creator = self.node.getnewaddress()
        self.receiver = self.reorg_node.getnewaddress()
        generatesynchronized(self.reorg_node, 50, self.receiver, self.nodes)
        generatesynchronized(self.node, 50, self.creator, self.nodes)
        generatesynchronized(self.node, COINBASE_MATURITY, self.creator, self.nodes)
        """
            pragma solidity ^0.8.1;
            import './SafeMath.sol';


            /**
                QRC20Token Standard Token implementation
            */
            contract QRC20Token is SafeMath {
                string public constant standard = 'Token 0.1';
                uint8 public constant decimals = 8; // it's recommended to set decimals to 8 in QTUM

                // you need change the following three values
                string public constant name = 'QRC TEST';
                string public constant symbol = 'QTC';
                //Default assumes totalSupply can't be over max (2^256 - 1).
                //you need multiply 10^decimals by your real total supply.
                uint256 public totalSupply = 10**65 * 10**uint256(decimals);

                mapping (address => uint256) public balanceOf;
                mapping (address => mapping (address => uint256)) public allowance;

                event Transfer(address indexed _from, address indexed _to, uint256 _value);
                event Approval(address indexed _owner, address indexed _spender, uint256 _value);

                constructor() {
                    balanceOf[msg.sender] = totalSupply;
                }

                // validates an address - currently only checks that it isn't null
                modifier validAddress(address _address) {
                    require(_address != address(0x0), "Address is NULL");
                    _;
                }

                function transfer(address _to, uint256 _value)
                public
                validAddress(_to)
                returns (bool success)
                {
                    balanceOf[msg.sender] = safeSub(balanceOf[msg.sender], _value);
                    balanceOf[_to] = safeAdd(balanceOf[_to], _value);
                    emit Transfer(msg.sender, _to, _value);
                    return true;
                }

                function transferFrom(address _from, address _to, uint256 _value)
                public
                validAddress(_from)
                validAddress(_to)
                returns (bool success)
                {
                    allowance[_from][msg.sender] = safeSub(allowance[_from][msg.sender], _value);
                    balanceOf[_from] = safeSub(balanceOf[_from], _value);
                    balanceOf[_to] = safeAdd(balanceOf[_to], _value);
                    emit Transfer(_from, _to, _value);
                    return true;
                }

                function approve(address _spender, uint256 _value)
                public
                validAddress(_spender)
                returns (bool success)
                {
                    // To change the approve amount you first have to reduce the addresses`
                    //  allowance to zero by calling `approve(_spender, 0)` if it is not
                    //  already 0 to mitigate the race condition described here:
                    //  https://github.com/ethereum/EIPs/issues/20#issuecomment-263524729
                    require(_value == 0 || allowance[msg.sender][_spender] == 0);
                    allowance[msg.sender][_spender] = _value;
                    emit Approval(msg.sender, _spender, _value);
                    return true;
                }
            }        
        """
        bytecode = "6080604052600860ff16600a620000179190620000f7565b7af316271c7fc3908a8bef464e3945ef7a25360a00000000000000006200003f919062000234565b6000553480156200004f57600080fd5b50600054600160003373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002081905550620002db565b6000808291508390505b6001851115620000ee57808604811115620000c657620000c56200029f565b5b6001851615620000d65780820291505b8081029050620000e685620002ce565b9450620000a6565b94509492505050565b6000620001048262000295565b9150620001118362000295565b9250620001407fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff848462000148565b905092915050565b6000826200015a57600190506200022d565b816200016a57600090506200022d565b81600181146200018357600281146200018e57620001c4565b60019150506200022d565b60ff841115620001a357620001a26200029f565b5b8360020a915084821115620001bd57620001bc6200029f565b5b506200022d565b5060208310610133831016604e8410600b8410161715620001fe5782820a905083811115620001f857620001f76200029f565b5b6200022d565b6200020d84848460016200009c565b925090508184048111156200022757620002266200029f565b5b81810290505b9392505050565b6000620002418262000295565b91506200024e8362000295565b9250817fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff04831182151516156200028a57620002896200029f565b5b828202905092915050565b6000819050919050565b7f4e487b7100000000000000000000000000000000000000000000000000000000600052601160045260246000fd5b60008160011c9050919050565b610f5380620002eb6000396000f3fe608060405234801561001057600080fd5b506004361061009e5760003560e01c80635a3b7e42116100665780635a3b7e421461015d57806370a082311461017b57806395d89b41146101ab578063a9059cbb146101c9578063dd62ed3e146101f95761009e565b806306fdde03146100a3578063095ea7b3146100c157806318160ddd146100f157806323b872dd1461010f578063313ce5671461013f575b600080fd5b6100ab610229565b6040516100b89190610ce0565b60405180910390f35b6100db60048036038101906100d69190610c00565b610262565b6040516100e89190610cc5565b60405180910390f35b6100f961045a565b6040516101069190610d22565b60405180910390f35b61012960048036038101906101249190610bb1565b610460565b6040516101369190610cc5565b60405180910390f35b6101476107d4565b6040516101549190610d3d565b60405180910390f35b6101656107d9565b6040516101729190610ce0565b60405180910390f35b61019560048036038101906101909190610b4c565b610812565b6040516101a29190610d22565b60405180910390f35b6101b361082a565b6040516101c09190610ce0565b60405180910390f35b6101e360048036038101906101de9190610c00565b610863565b6040516101f09190610cc5565b60405180910390f35b610213600480360381019061020e9190610b75565b610a5e565b6040516102209190610d22565b60405180910390f35b6040518060400160405280600881526020017f515243205445535400000000000000000000000000000000000000000000000081525081565b600082600073ffffffffffffffffffffffffffffffffffffffff168173ffffffffffffffffffffffffffffffffffffffff1614156102d5576040517f08c379a00000000000000000000000000000000000000000000000000000000081526004016102cc90610d02565b60405180910390fd5b600083148061036057506000600260003373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002060008673ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002054145b61036957600080fd5b82600260003373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002060008673ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020819055508373ffffffffffffffffffffffffffffffffffffffff163373ffffffffffffffffffffffffffffffffffffffff167f8c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b925856040516104479190610d22565b60405180910390a3600191505092915050565b60005481565b600083600073ffffffffffffffffffffffffffffffffffffffff168173ffffffffffffffffffffffffffffffffffffffff1614156104d3576040517f08c379a00000000000000000000000000000000000000000000000000000000081526004016104ca90610d02565b60405180910390fd5b83600073ffffffffffffffffffffffffffffffffffffffff168173ffffffffffffffffffffffffffffffffffffffff161415610544576040517f08c379a000000000000000000000000000000000000000000000000000000000815260040161053b90610d02565b60405180910390fd5b6105ca600260008873ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002060003373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020016000205485610a83565b600260008873ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002060003373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002081905550610693600160008873ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020016000205485610a83565b600160008873ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020016000208190555061071f600160008773ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020016000205485610ad0565b600160008773ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020819055508473ffffffffffffffffffffffffffffffffffffffff168673ffffffffffffffffffffffffffffffffffffffff167fddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef866040516107bf9190610d22565b60405180910390a36001925050509392505050565b600881565b6040518060400160405280600981526020017f546f6b656e20302e31000000000000000000000000000000000000000000000081525081565b60016020528060005260406000206000915090505481565b6040518060400160405280600381526020017f515443000000000000000000000000000000000000000000000000000000000081525081565b600082600073ffffffffffffffffffffffffffffffffffffffff168173ffffffffffffffffffffffffffffffffffffffff1614156108d6576040517f08c379a00000000000000000000000000000000000000000000000000000000081526004016108cd90610d02565b60405180910390fd5b61091f600160003373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020016000205484610a83565b600160003373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020819055506109ab600160008673ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020016000205484610ad0565b600160008673ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020819055508373ffffffffffffffffffffffffffffffffffffffff163373ffffffffffffffffffffffffffffffffffffffff167fddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef85604051610a4b9190610d22565b60405180910390a3600191505092915050565b6002602052816000526040600020602052806000526040600020600091509150505481565b600081831015610abc577f4e487b7100000000000000000000000000000000000000000000000000000000600052600160045260246000fd5b8183610ac89190610dca565b905092915050565b6000808284610adf9190610d74565b905083811015610b18577f4e487b7100000000000000000000000000000000000000000000000000000000600052600160045260246000fd5b8091505092915050565b600081359050610b3181610eef565b92915050565b600081359050610b4681610f06565b92915050565b600060208284031215610b5e57600080fd5b6000610b6c84828501610b22565b91505092915050565b60008060408385031215610b8857600080fd5b6000610b9685828601610b22565b9250506020610ba785828601610b22565b9150509250929050565b600080600060608486031215610bc657600080fd5b6000610bd486828701610b22565b9350506020610be586828701610b22565b9250506040610bf686828701610b37565b9150509250925092565b60008060408385031215610c1357600080fd5b6000610c2185828601610b22565b9250506020610c3285828601610b37565b9150509250929050565b610c4581610e10565b82525050565b6000610c5682610d58565b610c608185610d63565b9350610c70818560208601610e53565b610c7981610eb5565b840191505092915050565b6000610c91600f83610d63565b9150610c9c82610ec6565b602082019050919050565b610cb081610e3c565b82525050565b610cbf81610e46565b82525050565b6000602082019050610cda6000830184610c3c565b92915050565b60006020820190508181036000830152610cfa8184610c4b565b905092915050565b60006020820190508181036000830152610d1b81610c84565b9050919050565b6000602082019050610d376000830184610ca7565b92915050565b6000602082019050610d526000830184610cb6565b92915050565b600081519050919050565b600082825260208201905092915050565b6000610d7f82610e3c565b9150610d8a83610e3c565b9250827fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff03821115610dbf57610dbe610e86565b5b828201905092915050565b6000610dd582610e3c565b9150610de083610e3c565b925082821015610df357610df2610e86565b5b828203905092915050565b6000610e0982610e1c565b9050919050565b60008115159050919050565b600073ffffffffffffffffffffffffffffffffffffffff82169050919050565b6000819050919050565b600060ff82169050919050565b60005b83811015610e71578082015181840152602081019050610e56565b83811115610e80576000848401525b50505050565b7f4e487b7100000000000000000000000000000000000000000000000000000000600052601160045260246000fd5b6000601f19601f8301169050919050565b7f41646472657373206973204e554c4c0000000000000000000000000000000000600082015250565b610ef881610dfe565b8114610f0357600080fd5b50565b610f0f81610e3c565b8114610f1a57600080fd5b5056fea2646970667358221220428f0675eabb8d19af3d0c2868ed3d1faf18c135df79935059e8f6da0fba00e864736f6c63430008020033"
        self.contract_address = self.node.createcontract(bytecode)['address']
        self.node.generatetoaddress(1, self.creator)
        self.sync_blocks()

        self.qrc20name_test()
        self.qrc20symbol_test()
        self.qrc20totalsupply_test()
        self.qrc20decimals_test()
        assert_equal(self.node.qrc20balanceof(self.contract_address, self.creator), f"{10**65}.00000000")

        self.disconnect_nodes(0, 1)
        for n in self.nodes:
            assert_equal(n.getpeerinfo(), [])

        self.node.qrc20transfer(self.contract_address, self.creator, self.receiver, "0.00000001")
        self.node.generate(1)
        tip = self.node.getblock(self.node.getbestblockhash())
        assert_equal(self.node.qrc20listtransactions(self.contract_address, self.creator, 0, 0), 
            [{
                'receiver': self.receiver, 
                'sender': self.creator,
                'amount': '-0.00000001', 
                'confirmations': 1, 
                'blockHash': tip['hash'],
                'blockNumber': 2102, 
                'blocktime': tip['time'], 
                'transactionHash': tip['tx'][1]
            }])

        assert_equal(self.node.qrc20listtransactions(self.contract_address, self.receiver, 0, 0), 
            [{
                'receiver': self.receiver, 
                'sender': self.creator,
                'amount': '0.00000001', 
                'confirmations': 1, 
                'blockHash': tip['hash'],
                'blockNumber': 2102, 
                'blocktime': tip['time'], 
                'transactionHash': tip['tx'][1]
            }])

        assert_equal(self.node.qrc20balanceof(self.contract_address, self.creator), f"{10**65-1}.99999999")
        assert_equal(float(self.node.qrc20balanceof(self.contract_address, self.receiver)), 0.00000001)


        # Double spend the qrc20 transaction to clear it from the mempool and reorg to clear the state
        self.double_spend_tx(self.node, tip['tx'][1], self.creator, self.reorg_node)
        self.reorg_node.generate(2)
        self.connect_nodes(0, 1)
        self.sync_blocks(self.nodes)
        assert_equal(self.node.qrc20listtransactions(self.contract_address, self.creator, 0, 0), [])
        assert_equal(self.reorg_node.qrc20listtransactions(self.contract_address, self.creator, 0, 0), [])
        assert_equal(self.reorg_node.qrc20balanceof(self.contract_address, self.creator), f"{10**65}.00000000")
        assert_equal(self.node.qrc20balanceof(self.contract_address, self.creator), f"{10**65}.00000000")
        assert_equal(float(self.reorg_node.qrc20balanceof(self.contract_address, self.receiver)), 0)
        assert_equal(float(self.node.qrc20balanceof(self.contract_address, self.receiver)), 0)


        # Make sure that really bignums actually work
        self.disconnect_nodes(0, 1)
        for n in self.nodes:
            assert_equal(n.getpeerinfo(), [])

        self.node.qrc20transfer(self.contract_address, self.creator, self.receiver, f"{10**65}.00000000")
        self.node.generate(1)
        #sync_blocks(self.nodes)
        tip = self.node.getblock(self.node.getbestblockhash())
        assert_equal(self.node.qrc20listtransactions(self.contract_address, self.receiver, 0, 0), 
            [{
                'receiver': self.receiver, 
                'sender': self.creator,
                'amount': f"{10**65}.00000000", 
                'confirmations': 1, 
                'blockHash': tip['hash'],
                'blockNumber': tip['height'], 
                'blocktime': tip['time'], 
                'transactionHash': tip['tx'][1]
            }])
        assert_equal(self.node.qrc20listtransactions(self.contract_address, self.creator, 0, 0), 
            [{
                'receiver': self.receiver, 
                'sender': self.creator,
                'amount': f"-{10**65}.00000000", 
                'confirmations': 1, 
                'blockHash': tip['hash'],
                'blockNumber': tip['height'], 
                'blocktime': tip['time'], 
                'transactionHash': tip['tx'][1]
            }])

        # Double spend the qrc20 transaction to clear it from the mempool and reorg to clear the state
        self.double_spend_tx(self.node, tip['tx'][1], self.creator, self.reorg_node)
        self.reorg_node.generate(2)
        self.connect_nodes(0, 1)
        self.sync_blocks(self.nodes)
        assert_equal(self.node.qrc20listtransactions(self.contract_address, self.creator, 0, 0), [])
        assert_equal(self.reorg_node.qrc20listtransactions(self.contract_address, self.creator, 0, 0), [])
        assert_equal(self.reorg_node.qrc20balanceof(self.contract_address, self.creator), f"{10**65}.00000000")
        assert_equal(self.node.qrc20balanceof(self.contract_address, self.creator), f"{10**65}.00000000")
        assert_equal(float(self.reorg_node.qrc20balanceof(self.contract_address, self.receiver)), 0)
        assert_equal(float(self.node.qrc20balanceof(self.contract_address, self.receiver)), 0)

        # Make sure that sending from an address that has no balance fails gracefully
        assert_raises_rpc_error(-1, "Not enough token balance", self.reorg_node.qrc20transfer, self.contract_address, self.receiver, self.creator, "0.1")

        # Make sure that sending from an address that we do not control fails gracefully
        assert_raises_rpc_error(-4, "Private key not available", self.reorg_node.qrc20transfer, self.contract_address, self.creator, self.receiver, "0.1")

        # Make sure that the allowance functionality works as intended
        self.approve_tx_receiver = self.node.getnewaddress()
        self.node.qrc20approve(self.contract_address, self.creator, self.receiver, "0.1")
        self.node.generatetoaddress(1, self.creator)
        self.sync_blocks()

        assert_equal(float(self.node.qrc20allowance(self.contract_address, self.creator, self.receiver)), 0.1)
        assert_raises_rpc_error(-1, "Not enough token allowance", self.reorg_node.qrc20transferfrom, self.contract_address, self.creator, self.receiver, self.approve_tx_receiver, "0.10000001")

        self.reorg_node.qrc20transferfrom(self.contract_address, self.creator, self.receiver, self.approve_tx_receiver, "0.01")
        self.sync_all()
        self.node.generatetoaddress(1, self.creator)
        self.sync_blocks()
        tip = self.node.getblock(self.node.getbestblockhash())
        assert_equal(self.reorg_node.qrc20listtransactions(self.contract_address, self.approve_tx_receiver, 0, 0), [{
            'amount': '0.01000000',
            'blockHash': tip['hash'],
            'blockNumber': 2107,
            'blocktime': tip['time'],
            'confirmations': 1,
            'receiver': self.approve_tx_receiver,
            'sender': self.creator,
            'transactionHash': tip['tx'][1]
        }])

        assert_equal(self.reorg_node.qrc20listtransactions(self.contract_address, self.creator, 0, 0), [{
            'amount': '-0.01000000',
            'blockHash': tip['hash'],
            'blockNumber': 2107,
            'blocktime': tip['time'],
            'confirmations': 1,
            'receiver': self.approve_tx_receiver,
            'sender': self.creator,
            'transactionHash': tip['tx'][1]
        }])

        assert_raises_rpc_error(-1, "Not enough token allowance", self.reorg_node.qrc20transferfrom, self.contract_address, self.creator, self.receiver, self.approve_tx_receiver, "0.09000001")

        self.reorg_node.qrc20transferfrom(self.contract_address, self.creator, self.receiver, self.approve_tx_receiver, "0.09000000")
        self.reorg_node.generate(1)
        self.sync_blocks()

        tip = self.node.getblock(self.node.getbestblockhash())
        prevtip = self.node.getblock(tip['previousblockhash'])
        assert_equal(self.reorg_node.qrc20listtransactions(self.contract_address, self.creator, 0, 0), [{
            'amount': '-0.01000000',
            'blockHash': prevtip['hash'],
            'blockNumber': 2107,
            'blocktime': prevtip['time'],
            'confirmations': 2,
            'receiver': self.approve_tx_receiver,
            'sender': self.creator,
            'transactionHash': prevtip['tx'][1]
        }, {'amount': '-0.09000000',
            'blockHash': tip['hash'],
            'blockNumber': 2108,
            'blocktime': tip['time'],
            'confirmations': 1,
            'receiver': self.approve_tx_receiver,
            'sender': self.creator,
            'transactionHash': tip['tx'][1]
        }])
        assert_equal(float(self.node.qrc20allowance(self.contract_address, self.creator, self.receiver)), 0)
        assert_equal(self.reorg_node.qrc20balanceof(self.contract_address, self.creator), f"{10**65-1}.90000000")
        assert_equal(self.node.qrc20balanceof(self.contract_address, self.approve_tx_receiver), f"0.10000000")




if __name__ == '__main__':
    QtumQRC20Test().main()
