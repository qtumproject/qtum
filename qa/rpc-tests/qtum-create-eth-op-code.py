#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.qtum import *
from test_framework.address import *


class QtumCreateEthOpCodeTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [[]])
        self.node = self.nodes[0]

    def assert_address_with_value_in_unspents(self, address, value):
        for unspent in self.node.listunspent():
            if unspent['address'] == address:
                assert_equal(unspent['amount'], value)
                break
        else:
            assert(False)


    def create_contract_with_value_from_contract_test(self):
        """
        pragma solidity ^0.4.7;

        contract Factory {
            bytes code;
            address public newAddress;
            function create() payable {
                code = hex"60606040525b5b5b60358060146000396000f30060606040525b5b5b0000a165627a7a72305820c09bfe42796663bc047f817fd76fe3537f040acc4a39c783c9f41493c88dd24d0029";
                bytes memory mem_code = code;
                address newAddr;
                assembly {
                    newAddr := create(100, add(mem_code,0x20), mload(mem_code))
                }
                newAddress = newAddr;
            }
        }
        """
        factory_with_value_contract_address = self.node.createcontract("6060604052341561000f57600080fd5b5b6103448061001f6000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff168063ccdb3f4514610049578063efc81a8c1461009e575b600080fd5b341561005457600080fd5b61005c6100a8565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b6100a66100ce565b005b600160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b6100d661025f565b6000608060405190810160405280604981526020017f60606040525b5b5b60358060146000396000f30060606040525b5b5b0000a16581526020017f627a7a72305820c09bfe42796663bc047f817fd76fe3537f040acc4a39c783c981526020017ff41493c88dd24d0029000000000000000000000000000000000000000000000081525060009080519060200190610170929190610273565b5060008054600181600116156101000203166002900480601f0160208091040260200160405190810160405280929190818152602001828054600181600116156101000203166002900480156102075780601f106101dc57610100808354040283529160200191610207565b820191906000526020600020905b8154815290600101906020018083116101ea57829003601f168201915b505050505091508151602083016064f0905080600160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b5050565b602060405190810160405280600081525090565b828054600181600116156101000203166002900490600052602060002090601f016020900481019282601f106102b457805160ff19168380011785556102e2565b828001600101855582156102e2579182015b828111156102e15782518255916020019190600101906102c6565b5b5090506102ef91906102f3565b5090565b61031591905b808211156103115760008160009055506001016102f9565b5090565b905600a165627a7a72305820154b57e8d991c23341604ced78f201aeb717e5d5678ae7c4941fa4eac267dfc60029")['address']
        self.node.generate(1)
        deployed_contracts = self.node.listcontracts()
        # Make sure that the tx was mined
        assert_equal(len(self.node.getrawmempool()), 0)
        # Make sure that the contract was deployed
        assert_equal(deployed_contracts[factory_with_value_contract_address], 0)

        # Next, attempt to create the contract via the "create" method
        txid = self.node.sendtocontract(factory_with_value_contract_address, "efc81a8c", 1000, 1000000, 0.000001)['txid']
        blockhash = self.node.generate(1)[0]
        # Make sure that the tx was mined
        assert_equal(len(self.node.getrawmempool()), 0)
        # Make sure that the contract was NOT created
        assert_equal(self.node.listcontracts(), deployed_contracts)

        # Make sure that the call to create resulted in an out of gas exception (all gas will have been assigned to the miner)
        # The total gas is equal to 1 qtum (10^6 * 10^2) + a minor txfee
        block = self.node.getblock(blockhash)
        coinbase_tx = self.node.getrawtransaction(block['tx'][0], True)
        assert(coinbase_tx['vout'][0]['value'] >= 20000+1)
        
        # Since the call to the contract threw an out of gas exception the origin contract should have a zero balance
        assert_equal(deployed_contracts[factory_with_value_contract_address], 0)

    def create_contract_without_value_from_contract_test(self):
        # Below we make sure that calls work as expected on contracts created by other contracts.
        """
        pragma solidity ^0.4.12;

        contract Factory {
            address public newAddress;
            function create() payable {
                newAddress = new Test();
            }

            function() payable {}
        }

        contract Test {
            uint public check;
            
            function Test() payable {}
            
            function destroy() payable {
                suicide(msg.sender);
            }
            function sendTo(address other, uint value) {
                other.transfer(value);
            }
            
            function checkOtherAddress(address other) {
                address myOwnAddress = Factory(other).newAddress();
                if(myOwnAddress == address(this)) {
                    check = 1;
                } else {
                    check = 2;
                }
            }

            function() payable {}
        }
        """
        # We create the Factory contract, which will later be used to create the Test contract.
        factory_contract_bytecode = "6060604052341561000f57600080fd5b5b6104028061001f6000396000f3006060604052361561004a576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff168063ccdb3f451461004e578063efc81a8c146100a3575b5b5b005b341561005957600080fd5b6100616100ad565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b6100ab6100d2565b005b6000809054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b6100da610132565b604051809103906000f08015156100f057600080fd5b6000806101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b565b6040516102948061014383390190560060606040525b5b5b61027e806100166000396000f30060606040523615610060576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff168063541227e11461006457806383197ef01461009d578063919840ad146100a75780639e1a00aa146100d0575b5b5b005b341561006f57600080fd5b61009b600480803573ffffffffffffffffffffffffffffffffffffffff16906020019091905050610112565b005b6100a56101ec565b005b34156100b257600080fd5b6100ba610207565b6040518082815260200191505060405180910390f35b34156100db57600080fd5b610110600480803573ffffffffffffffffffffffffffffffffffffffff1690602001909190803590602001909190505061020d565b005b60008173ffffffffffffffffffffffffffffffffffffffff1663ccdb3f456000604051602001526040518163ffffffff167c0100000000000000000000000000000000000000000000000000000000028152600401602060405180830381600087803b151561018057600080fd5b6102c65a03f1151561019157600080fd5b5050506040518051905090503073ffffffffffffffffffffffffffffffffffffffff168173ffffffffffffffffffffffffffffffffffffffff1614156101de5760016000819055506101e7565b60026000819055505b5b5050565b3373ffffffffffffffffffffffffffffffffffffffff16ff5b565b60005481565b8173ffffffffffffffffffffffffffffffffffffffff166108fc829081150290604051600060405180830381858888f19350505050151561024d57600080fd5b5b50505600a165627a7a72305820fa7432274a811bb14b3a9182e51715a333ce275636c08ab171c44fa197f20d6c0029a165627a7a723058201d26b91d6e884c90c5d2582dc21c991a7552a6538030c5599f555fb0b7eacd450029"
        self.factory_contract_address = self.node.createcontract(factory_contract_bytecode)['address']
        self.node.generate(1)
        assert_equal(self.node.listcontracts()[self.factory_contract_address], 0)

        # Call create(), creating the Test contract
        self.node.sendtocontract(self.factory_contract_address, "efc81a8c", 0, 1000000, 0.000001)
        self.node.generate(1)

        # Fetch the address of the newly created contract via calling the newAddress() method
        output = self.node.callcontract(self.factory_contract_address, "ccdb3f45")['executionResult']['output']
        self.test_contract_address = output[24:]
        assert_equal(self.node.listcontracts()[self.test_contract_address], 0)


    def check_value_transfers_from_and_to_contract_test(self):
        # Send some coins to the contract
        self.node.sendtocontract(self.test_contract_address, "00", 100)
        self.node.generate(1)
        assert_equal(self.node.listcontracts()[self.test_contract_address], 100)

        # Transfer 50 qtum from the contract via p2pkh to an address of our choice
        receiver_address = self.node.getnewaddress()
        h160addr = str(base58_to_byte(receiver_address, 25)[1])[2:-1]
        data = "9e1a00aa"
        data += h160addr.zfill(64)
        data += hex(int(50*COIN))[2:].zfill(64)
        self.node.sendtocontract(self.test_contract_address, data)
        self.node.generate(1)
        assert_equal(self.node.listcontracts()[self.test_contract_address], 50)
        self.assert_address_with_value_in_unspents(receiver_address, 50)

        # Transfer 50 qtum from the contract via OP_CALL to its parent contract (the Factory contract)
        receiver_address = self.node.getnewaddress()
        h160addr = str(base58_to_byte(receiver_address, 25)[1])[2:-1]
        data = "9e1a00aa"
        data += self.factory_contract_address.zfill(64)
        data += hex(int(50*COIN))[2:].zfill(64)
        self.node.sendtocontract(self.test_contract_address, data)
        self.node.generate(1)
        assert_equal(self.node.listcontracts()[self.test_contract_address], 0)
        assert_equal(self.node.listcontracts()[self.factory_contract_address], 50)


    def check_calls_to_contract_test(self):
        self.node.sendtocontract(self.test_contract_address, "541227e1" + self.factory_contract_address.zfill(64))
        self.node.generate(1)
        output = self.node.callcontract(self.test_contract_address, "919840ad")['executionResult']['output']
        assert_equal(int(output, 16), 1)

    def check_suicide_test(self):
        sender = self.node.getnewaddress()
        self.node.sendtoaddress(sender, 1)
        self.node.generate(1)
        self.node.sendtocontract(self.test_contract_address, "83197ef0", 0, 1000000, 0.000001, sender)
        self.node.generate(1)
        # Make sure that the contract is no longer calleable, i.e., does not exist.
        assert_raises(JSONRPCException, self.node.sendtocontract, self.test_contract_address, "00")

    def run_test(self):
        self.node.generate(10+COINBASE_MATURITY)
        
        print('Checking that contracts cannot be created from other contracts with a default value')
        self.create_contract_with_value_from_contract_test()
        
        print('Checking that contracts can be created from other contract without a default value')
        self.create_contract_without_value_from_contract_test()
        print('Checking that value transfers via op_call and p2pkh works as expected for the created "subcontract"')
        self.check_value_transfers_from_and_to_contract_test()
        print('Checking that calls to other contracts works as expected for the created "subcontract"')
        self.check_calls_to_contract_test()
        print('Checking that suicides works as expected for the created "subcontract"')
        self.check_suicide_test()

if __name__ == '__main__':
    QtumCreateEthOpCodeTest().main()
