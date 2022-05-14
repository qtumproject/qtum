#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.address import *
from test_framework.qtum import *
import pprint
import subprocess

pp = pprint.PrettyPrinter()

class QtumEVMConstantinopleOpcodesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-txindex=1', '-logevents=1', '-constantinopleheight=1000000', '-muirglacierheight=1000000', '-londonheight=1000000']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def send(self, abi, value=0):
        self.last_txid = self.node.sendtocontract(self.contract_address, abi, value, 10000000)['txid']
        self.node.generate(1)

    def get_value_at_index(self, i):
        info = self.node.callcontract(self.contract_address, "a05d8c0b" + hex(i)[2:].zfill(64))
        #pp.pprint(info)
        ret = info['executionResult']['output']
        return ret

    def reset(self):
        self.node.sendtocontract(self.contract_address, 'd826f88f')
        # print(self.node.getblockcount())
        self.node.generate(1)

    def keccak256(self, data):
        abi = "b8612c0a"
        abi += hex(0x20)[2:].zfill(64)
        abi += hex(len(data) // 2)[2:].zfill(64)
        abi += data
        return self.node.callcontract(self.contract_address, abi)['executionResult']['output']

    def revert_test(self, should_fail):
        abi = "9ca24dff"
        self.send(abi)
        # Make sure that all gas was not consumed as in throw
        used_gas = self.node.gettransactionreceipt(self.last_txid)[0]['gasUsed']
        if should_fail:
            assert_equal(used_gas, 10000000)
        else:
            assert(used_gas < 100000)
            # Make sure that the state change was reverted
            assert_equal(self.get_value_at_index(0), "")

    def dynamicreturndata_test(self, should_fail):
        numValues = int(self.node.getbestblockhash()[1:2], 16)
        abi = "1bbfb682"
        self.send(abi)
        for i in range(numValues):
            ret = self.get_value_at_index(i)
            if should_fail:
                assert_equal(self.get_value_at_index(0), "")
            else:
                assert_equal(ret, "f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f" + hex(i)[2:])
        ret = self.get_value_at_index(numValues)
        assert_equal(ret, "")

    def staticcall_test(self, should_fail):
        abi = "9c9d8fbe"
        self.send(abi)
        if should_fail:
            assert_equal(self.get_value_at_index(0), "")
        else:
            assert_equal(self.get_value_at_index(0), "0000000000000000000000000000000000000000000000000000000000000001")

    def invalidstaticcall_test(self, should_fail):
        abi = "47729055"
        self.send(abi)
        if should_fail:
            assert_equal(self.get_value_at_index(0), "")
        else:
            assert_equal(self.get_value_at_index(0), "0000000000000000000000000000000000000000000000000000000000000000")

    def shl_test(self, should_fail):
        for i in range(50):
            x = random.randint(0, 256)
            y = random.randint(0, 255)
            self.reset()
            abi = "c5b319b2"
            abi += hex(x)[2:].zfill(64)
            abi += hex(1 << y)[2:].zfill(64)
            self.send(abi)
            if should_fail:
                assert_equal(self.get_value_at_index(0), "")
            else:
                assert_equal(self.get_value_at_index(0), hex(((1 << y) << x) & (2**256-1))[2:].zfill(64))

    def shr_test(self, should_fail):
        for i in range(50):
            x = random.randint(0, 256)
            y = random.randint(0, 255)
            self.reset()
            abi = "08a86e5f"
            abi += hex(x)[2:].zfill(64)
            abi += hex(1 << y)[2:].zfill(64)
            self.send(abi)
            if should_fail:
                assert_equal(self.get_value_at_index(0), "")
            else:
                assert_equal(self.get_value_at_index(0), hex(((1 << y) >> x) & (2**256-1))[2:].zfill(64))

    def sar_test(self, should_fail):
        cases = [
            "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001",
            "0000000000000000000000000000000000000000000000000000000000000001",

            "00000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000000001",
            "0000000000000000000000000000000000000000000000000000000000000000",

            "00000000000000000000000000000000000000000000000000000000000000018000000000000000000000000000000000000000000000000000000000000000",
            "c000000000000000000000000000000000000000000000000000000000000000",

            "00000000000000000000000000000000000000000000000000000000000000ff8000000000000000000000000000000000000000000000000000000000000000",
            "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",

            "00000000000000000000000000000000000000000000000000000000000001008000000000000000000000000000000000000000000000000000000000000000",
            "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",

            "00000000000000000000000000000000000000000000000000000000000001018000000000000000000000000000000000000000000000000000000000000000",
            "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",

            "0000000000000000000000000000000000000000000000000000000000000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
            "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",

            "0000000000000000000000000000000000000000000000000000000000000001ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
            "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",

            "00000000000000000000000000000000000000000000000000000000000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
            "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",

            "0000000000000000000000000000000000000000000000000000000000000100ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
            "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",

            "00000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000000000",
            "0000000000000000000000000000000000000000000000000000000000000000",

            "00000000000000000000000000000000000000000000000000000000000000fe4000000000000000000000000000000000000000000000000000000000000000",
            "0000000000000000000000000000000000000000000000000000000000000001",

            "00000000000000000000000000000000000000000000000000000000000000f87fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
            "000000000000000000000000000000000000000000000000000000000000007f",

            "00000000000000000000000000000000000000000000000000000000000000fe7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
            "0000000000000000000000000000000000000000000000000000000000000001",

            "00000000000000000000000000000000000000000000000000000000000000ff7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
            "0000000000000000000000000000000000000000000000000000000000000000",

            "00000000000000000000000000000000000000000000000000000000000001007fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
            "0000000000000000000000000000000000000000000000000000000000000000",
        ]
        i = 0
        while i < len(cases):
            self.reset()
            in_val = cases[i]
            out_val = cases[i+1]
            abi = "825d4ba3"
            abi += in_val
            # print('SAR', in_val)
            self.send(abi)
            if should_fail:
                assert_equal(self.get_value_at_index(0), "")
            else:
                assert_equal(self.get_value_at_index(0), out_val)

            i += 2

    def create2_test(self, should_fail, salt="00"):
        # PostConstantinopleOpCodesTest
        bytecode = "608060405234801561001057600080fd5b50610a57806100206000396000f3fe6080604052600436106100e85760003560e01c80639fcc18131161008a578063c5b319b211610059578063c5b319b214610473578063c9846c59146104b8578063d826f88f146104cf578063e5037fb1146104e6576100e8565b80639fcc181314610221578063a05d8c0b146102dc578063a1d98fa11461032b578063b8612c0a14610397576100e8565b80634f2f0adc116100c65780634f2f0adc1461015d578063825d4ba3146101ae5780639c9d8fbe146101f35780639ca24dff1461020a576100e8565b806308a86e5f146100ea5780631bbfb6821461012f5780634772905514610146575b005b3480156100f657600080fd5b5061012d6004803603604081101561010d57600080fd5b8101908080359060200190929190803590602001909291905050506104fd565b005b34801561013b57600080fd5b50610144610532565b005b34801561015257600080fd5b5061015b61061f565b005b34801561016957600080fd5b506101ac6004803603602081101561018057600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff1690602001909291905050506106c1565b005b3480156101ba57600080fd5b506101f1600480360360408110156101d157600080fd5b8101908080359060200190929190803590602001909291905050506106f7565b005b3480156101ff57600080fd5b5061020861072c565b005b34801561021657600080fd5b5061021f6107ce565b005b6102da6004803603602081101561023757600080fd5b810190808035906020019064010000000081111561025457600080fd5b82018360208201111561026657600080fd5b8035906020019184600183028401116401000000008311171561028857600080fd5b91908080601f016020809104026020016040519081016040528093929190818152602001838380828437600081840152601f19601f82011690508083019250505050505050919291929050505061081f565b005b3480156102e857600080fd5b50610315600480360360208110156102ff57600080fd5b8101908080359060200190929190505050610863565b6040518082815260200191505060405180910390f35b34801561033757600080fd5b50610340610883565b6040518080602001828103825283818151815260200191508051906020019060200280838360005b83811015610383578082015181840152602081019050610368565b505050509050019250505060405180910390f35b3480156103a357600080fd5b5061045d600480360360208110156103ba57600080fd5b81019080803590602001906401000000008111156103d757600080fd5b8201836020820111156103e957600080fd5b8035906020019184600183028401116401000000008311171561040b57600080fd5b91908080601f016020809104026020016040519081016040528093929190818152602001838380828437600081840152601f19601f820116905080830192505050505050509192919290505050610925565b6040518082815260200191505060405180910390f35b34801561047f57600080fd5b506104b66004803603604081101561049657600080fd5b810190808035906020019092919080359060200190929190505050610936565b005b3480156104c457600080fd5b506104cd61096b565b005b3480156104db57600080fd5b506104e46109b9565b005b3480156104f257600080fd5b506104fb6109c8565b005b80821c915060008290806001815401808255809150509060018203906000526020600020016000909192909190915055505050565b60006060600060405180807f72657475726e7661726c656e6774682829000000000000000000000000000000815250601101905060405180910390207bffffffffffffffffffffffffffffffffffffffffffffffffffffffff1916905060008090506040518281526040519350600080600483600030620186a05a03f13d95506040519450602086036020863e5050600090505b82518110156106195760008382815181106105dd57fe5b602002602001015190806001815401808255809150509060018203906000526020600020016000909192909190915055508060010190506105c6565b50505050565b600060405180807f6d757461626c6563616c6c6261636b2829000000000000000000000000000000815250601101905060405180910390207bffffffffffffffffffffffffffffffffffffffffffffffffffffffff19169050600060405182815260008060048330620186a05a03fa91505060008190806001815401808255809150509060018203906000526020600020016000909192909190915055505050565b6000813f905060008190806001815401808255809150509060018203906000526020600020016000909192909190915055505050565b80821d915060008290806001815401808255809150509060018203906000526020600020016000909192909190915055505050565b600060405180807f73746174696363616c6c6261636b282900000000000000000000000000000000815250601001905060405180910390207bffffffffffffffffffffffffffffffffffffffffffffffffffffffff19169050600060405182815260008060048330620186a05a03fa91505060008190806001815401808255809150509060018203906000526020600020016000909192909190915055505050565b60007f0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef9080600181540180825580915050906001820390600052602060002001600090919290919091505550600080fd5b60008082519050600081602085016000f591506000829080600181540180825580915050906001820390600052602060002001600090919290919091505550505050565b600080828154811061087157fe5b90600052602060002001549050919050565b60606000600f600143034060001c1690506060816040519080825280602002602001820160405280156108c55781602001602082028038833980820191505090505b50905060008090505b8281101561091c57807ff0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f01782828151811061090557fe5b6020026020010181815250508060010190506108ce565b50809250505090565b600081805190602001209050919050565b80821b915060008290806001815401808255809150509060018203906000526020600020016000909192909190915055505050565b60007f12121212121212121212121212121212121212121212121212121212121212129080600181540180825580915050906001820390600052602060002001600090919290919091505550565b6000806109c691906109e5565b565b60008090505b600a8110156109e2578060010190506109ce565b50565b5080546000825590600052602060002090810190610a039190610a06565b50565b610a2891905b80821115610a24576000816000905550600101610a0c565b5090565b9056fea165627a7a72305820db27ae89f2aafaaa420f35ae90480db051520a88007ede039df1c2302d284ec20029"
        abi = "9fcc1813"
        abi += hex(0x20)[2:].zfill(64)
        abi += hex(len(bytecode+salt) // 2)[2:].zfill(64)
        abi += bytecode+salt
        self.send(abi)
        codehash = self.keccak256(bytecode+salt)
        expected_create2_address = self.keccak256("ff"+self.contract_address+("0"*64)+codehash)[24:]
        if should_fail:
            assert(expected_create2_address not in self.node.listcontracts())
            return
        else:
            create2_address = self.get_value_at_index(0)[24:]
            assert(create2_address in self.node.listcontracts())
            assert_equal(expected_create2_address, create2_address)

        # Send some value to the create2 address, should succeed
        self.node.sendtocontract(create2_address, "00", 1)
        self.node.generate(1)
        if should_fail:
            assert(expected_create2_address not in self.node.listcontracts())
        else:
            assert_equal(self.node.listcontracts()[create2_address], 1)

        # Make sure that creating another exactly the same create2 contract fails
        self.reset()
        self.send(abi)
        if should_fail:
            assert_equal(self.get_value_at_index(0), "")
        else:
            assert_equal(self.get_value_at_index(0), "0"*64)

        # Make sure that the value associated with the create2 address remains the same
        if not should_fail:
            assert_equal(self.node.listcontracts()[create2_address], 1)

    def extcodehash_test(self, should_fail):
        for contract_address in self.node.listcontracts().keys():
            self.reset()
            contract_bytecode = self.node.getaccountinfo(contract_address)['code']
            expected_codehash = self.keccak256(contract_bytecode)

            abi = "4f2f0adc"
            abi += contract_address.zfill(64)
            self.send(abi)
            codehash = self.get_value_at_index(0)
            if should_fail:
                assert_equal(codehash, "")
            else:
                assert_equal(codehash, expected_codehash)

        # precompiled contract btc_ecrecover
        contract_address = '0000000000000000000000000000000000000085'
        # according to eip-1052, extcodehash(precompiled contract) should return hash of empty data or 0
        expected_codehash = self.keccak256('')

        self.reset()
        abi = "4f2f0adc"
        abi += contract_address.zfill(64)
        self.send(abi)
        codehash = self.get_value_at_index(0)
        if should_fail:
            assert_equal(codehash, "")
        else:
            try:
                assert_equal(codehash, '0'.zfill(64))
            except AssertionError:
                assert_equal(codehash, expected_codehash)

        # non-existent address
        contract_address = '0123456789abcdefedcba9876543210123456789'
        # according to eip-1052, extcodehash(non-existent address) should return 0
        expected_codehash = '0'.zfill(64)

        self.reset()
        abi = "4f2f0adc"
        abi += contract_address.zfill(64)
        self.send(abi)
        codehash = self.get_value_at_index(0)
        if should_fail:
            assert_equal(codehash, "")
        else:
            assert_equal(codehash, expected_codehash)


    def run_test(self):
        self.nodes[0].generate(COINBASE_MATURITY+50)
        self.node = self.nodes[0]
        """
        pragma solidity ^0.5;

        contract PostConstantinopleOpCodesTest {
            uint256[] private ret;

            function getRet(uint256 index) public view returns (uint256) {
                return ret[index];
            }

            function reset()  public {
                delete ret;
            }

            function staticcallback() pure public {
                // do nothing
                uint256 i;
                for (i = 0; i < 10;++i) {}
            }

            function mutablecallback() public {
                ret.push(0x1212121212121212121212121212121212121212121212121212121212121212);
            }

            function returnvarlength() public view returns (uint256[] memory) {
                uint256 numElements = uint256(blockhash(block.number - 1)) & 0xf;
                uint256[] memory arr = new uint256[](numElements);
                for(uint i = 0; i < numElements; ++i) {
                    arr[i] = 0xf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0 | i;
                }
                return arr;
            }

            function revert_test() public {
                ret.push(0x0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef);
                assembly {
                    revert(0, 0)
                }
            }

            function dynamicreturndata_test() public {
                uint256 retsize;
                uint256[] memory arr;
                bytes32 sig = bytes32(bytes4(keccak256("returnvarlength()")));
                uint256 i = 0;
                assembly {
                    let x := mload(0x40)
                    mstore(x, sig)
                    arr := mload(0x40)
                    let r := call(sub(gas, 100000), address, 0, x, 4, 0, 0)
                    retsize := returndatasize
                    arr := mload(0x40)
                    returndatacopy(arr, 0x20, sub(retsize, 0x20))
                }
                for(i = 0; i < arr.length;++i) {
                    ret.push(arr[i]);
                }
            }

            function staticcall_test() public {
                bytes32 sig = bytes32(bytes4(keccak256("staticcallback()")));
                uint256 retval;
                assembly {
                    let x := mload(0x40)
                    mstore(x, sig)
                    retval := staticcall(sub(gas, 100000), address, x, 4, 0, 0)
                }
                ret.push(retval);
            }

            function invalidstaticcall_test() public {
                bytes32 sig = bytes32(bytes4(keccak256("mutablecallback()")));
                uint256 retval;
                assembly {
                    let x := mload(0x40)
                    mstore(x, sig)
                    retval := staticcall(sub(gas, 100000), address, x, 4, 0, 0)
                }
                ret.push(retval);   
            }

            function shl_test(uint256 x, uint256 y) public {
                assembly {
                    x := shl(x, y)
                }
                ret.push(x);
            }

            function shr_test(uint256 x, uint256 y) public {
                assembly {
                    x := shr(x, y)
                }
                ret.push(x);
            }

            function sar_test(uint256 x, uint256 y) public {
                assembly {
                    x := sar(x, y)
                }
                ret.push(x);
            }

            function extcodehash_test(address a) public {
                uint256 r;
                assembly {
                    r := extcodehash(a)
                }
                ret.push(r);
            }

            function create2_test(bytes memory code) public payable {
                uint256 r;
                uint256 size = code.length;
                assembly {
                    r := create2(0, add(code, 0x20), size, 0x0)
                }
                ret.push(r);
            }

            // To avoid having to add a dependancy for the non-standard keccak256 used in the EVM
            // we simply call it here to verify results of extcodehash and create2
            function dokeccak256(bytes memory data) public view returns (bytes32) {
                return keccak256(data);
            }


            function() external payable {}
        }

        9fcc1813: create2_test(bytes)
        b8612c0a: dokeccak256(bytes)
        1bbfb682: dynamicreturndata_test()
        4f2f0adc: extcodehash_test(address)
        a05d8c0b: getRet(uint256)
        47729055: invalidstaticcall_test()
        c9846c59: mutablecallback()
        d826f88f: reset()
        a1d98fa1: returnvarlength()
        9ca24dff: revert_test()
        825d4ba3: sar_test(uint256,uint256)
        c5b319b2: shl_test(uint256,uint256)
        08a86e5f: shr_test(uint256,uint256)
        9c9d8fbe: staticcall_test()
        e5037fb1: staticcallback()
        """
        bytecode = "608060405234801561001057600080fd5b50610a57806100206000396000f3fe6080604052600436106100e85760003560e01c80639fcc18131161008a578063c5b319b211610059578063c5b319b214610473578063c9846c59146104b8578063d826f88f146104cf578063e5037fb1146104e6576100e8565b80639fcc181314610221578063a05d8c0b146102dc578063a1d98fa11461032b578063b8612c0a14610397576100e8565b80634f2f0adc116100c65780634f2f0adc1461015d578063825d4ba3146101ae5780639c9d8fbe146101f35780639ca24dff1461020a576100e8565b806308a86e5f146100ea5780631bbfb6821461012f5780634772905514610146575b005b3480156100f657600080fd5b5061012d6004803603604081101561010d57600080fd5b8101908080359060200190929190803590602001909291905050506104fd565b005b34801561013b57600080fd5b50610144610532565b005b34801561015257600080fd5b5061015b61061f565b005b34801561016957600080fd5b506101ac6004803603602081101561018057600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff1690602001909291905050506106c1565b005b3480156101ba57600080fd5b506101f1600480360360408110156101d157600080fd5b8101908080359060200190929190803590602001909291905050506106f7565b005b3480156101ff57600080fd5b5061020861072c565b005b34801561021657600080fd5b5061021f6107ce565b005b6102da6004803603602081101561023757600080fd5b810190808035906020019064010000000081111561025457600080fd5b82018360208201111561026657600080fd5b8035906020019184600183028401116401000000008311171561028857600080fd5b91908080601f016020809104026020016040519081016040528093929190818152602001838380828437600081840152601f19601f82011690508083019250505050505050919291929050505061081f565b005b3480156102e857600080fd5b50610315600480360360208110156102ff57600080fd5b8101908080359060200190929190505050610863565b6040518082815260200191505060405180910390f35b34801561033757600080fd5b50610340610883565b6040518080602001828103825283818151815260200191508051906020019060200280838360005b83811015610383578082015181840152602081019050610368565b505050509050019250505060405180910390f35b3480156103a357600080fd5b5061045d600480360360208110156103ba57600080fd5b81019080803590602001906401000000008111156103d757600080fd5b8201836020820111156103e957600080fd5b8035906020019184600183028401116401000000008311171561040b57600080fd5b91908080601f016020809104026020016040519081016040528093929190818152602001838380828437600081840152601f19601f820116905080830192505050505050509192919290505050610925565b6040518082815260200191505060405180910390f35b34801561047f57600080fd5b506104b66004803603604081101561049657600080fd5b810190808035906020019092919080359060200190929190505050610936565b005b3480156104c457600080fd5b506104cd61096b565b005b3480156104db57600080fd5b506104e46109b9565b005b3480156104f257600080fd5b506104fb6109c8565b005b80821c915060008290806001815401808255809150509060018203906000526020600020016000909192909190915055505050565b60006060600060405180807f72657475726e7661726c656e6774682829000000000000000000000000000000815250601101905060405180910390207bffffffffffffffffffffffffffffffffffffffffffffffffffffffff1916905060008090506040518281526040519350600080600483600030620186a05a03f13d95506040519450602086036020863e5050600090505b82518110156106195760008382815181106105dd57fe5b602002602001015190806001815401808255809150509060018203906000526020600020016000909192909190915055508060010190506105c6565b50505050565b600060405180807f6d757461626c6563616c6c6261636b2829000000000000000000000000000000815250601101905060405180910390207bffffffffffffffffffffffffffffffffffffffffffffffffffffffff19169050600060405182815260008060048330620186a05a03fa91505060008190806001815401808255809150509060018203906000526020600020016000909192909190915055505050565b6000813f905060008190806001815401808255809150509060018203906000526020600020016000909192909190915055505050565b80821d915060008290806001815401808255809150509060018203906000526020600020016000909192909190915055505050565b600060405180807f73746174696363616c6c6261636b282900000000000000000000000000000000815250601001905060405180910390207bffffffffffffffffffffffffffffffffffffffffffffffffffffffff19169050600060405182815260008060048330620186a05a03fa91505060008190806001815401808255809150509060018203906000526020600020016000909192909190915055505050565b60007f0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef9080600181540180825580915050906001820390600052602060002001600090919290919091505550600080fd5b60008082519050600081602085016000f591506000829080600181540180825580915050906001820390600052602060002001600090919290919091505550505050565b600080828154811061087157fe5b90600052602060002001549050919050565b60606000600f600143034060001c1690506060816040519080825280602002602001820160405280156108c55781602001602082028038833980820191505090505b50905060008090505b8281101561091c57807ff0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f01782828151811061090557fe5b6020026020010181815250508060010190506108ce565b50809250505090565b600081805190602001209050919050565b80821b915060008290806001815401808255809150509060018203906000526020600020016000909192909190915055505050565b60007f12121212121212121212121212121212121212121212121212121212121212129080600181540180825580915050906001820390600052602060002001600090919290919091505550565b6000806109c691906109e5565b565b60008090505b600a8110156109e2578060010190506109ce565b50565b5080546000825590600052602060002090810190610a039190610a06565b50565b610a2891905b80821115610a24576000816000905550600101610a0c565b5090565b9056fea165627a7a72305820db27ae89f2aafaaa420f35ae90480db051520a88007ede039df1c2302d284ec20029"
        self.contract_address = self.node.createcontract(bytecode)['address']
        self.node.generate(1)
        #self.restart_node(0, ['-qip7height=1001', '-logevents'])
        self.log.info('Test opcodes before constantinople')
        self.reset()
        self.revert_test(should_fail=True)

        self.reset()
        self.dynamicreturndata_test(should_fail=True)
        
        self.reset()
        self.staticcall_test(should_fail=True)
        
        self.reset()
        self.invalidstaticcall_test(should_fail=True)
        
        self.reset()
        self.shl_test(should_fail=True)
        
        self.reset()
        self.shr_test(should_fail=True)
        
        self.reset()
        self.sar_test(should_fail=True)

        self.reset()
        self.create2_test(should_fail=True)

        self.reset()
        self.extcodehash_test(should_fail=True)

        switch_height1 = self.node.getblockcount()
        self.log.info('Test opcodes after constantinople (height: %s)', switch_height1)
        self.restart_node(0, ['-constantinopleheight={}'.format(switch_height1), '-reduceblocktimeheight={}'.format(switch_height1), '-logevents', '-londonheight=1000000'])

        self.reset()
        self.revert_test(should_fail=False)
        
        self.reset()
        self.dynamicreturndata_test(should_fail=False)
        
        self.reset()
        self.staticcall_test(should_fail=False)
        
        self.reset()
        self.invalidstaticcall_test(should_fail=False)
        
        self.reset()
        self.shl_test(should_fail=False)
        
        self.reset()
        self.shr_test(should_fail=False)
        
        self.reset()
        self.sar_test(should_fail=False)

        self.reset()
        self.create2_test(should_fail=False)

        self.reset()
        self.extcodehash_test(should_fail=False)
        
        switch_height2 = self.node.getblockcount()
        self.log.info('Test opcodes after london (height: %s)', switch_height2)
        self.restart_node(0, ['-constantinopleheight={}'.format(switch_height1), '-reduceblocktimeheight={}'.format(switch_height1), '-logevents', '-londonheight={}'.format(switch_height2)])

        self.reset()
        self.revert_test(should_fail=False)
        
        self.reset()
        self.dynamicreturndata_test(should_fail=False)
        
        self.reset()
        self.staticcall_test(should_fail=False)
        
        self.reset()
        self.invalidstaticcall_test(should_fail=False)
        
        self.reset()
        self.shl_test(should_fail=False)
        
        self.reset()
        self.shr_test(should_fail=False)
        
        self.reset()
        self.sar_test(should_fail=False)

        self.reset()
        self.create2_test(should_fail=False, salt="01")

        # self.reset()
        self.extcodehash_test(should_fail=False)

if __name__ == '__main__':
    QtumEVMConstantinopleOpcodesTest().main()
