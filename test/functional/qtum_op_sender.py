#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.qtum import *
from test_framework.address import *
from test_framework.blocktools import *
from test_framework.key import *


# If this is an OP_SENDER output, make sure that the signature is an empty byte vector
def GetOutputWithoutSenderSig(output):
    op_sender_index = -1
    for i, opcode in enumerate(CScript(output.scriptPubKey)):
        if opcode == OP_SENDER:
            op_sender_index = i
    if i > 0:
        operations = []
        for i, opcode in enumerate(CScript(output.scriptPubKey)):
            if i == op_sender_index - 1:
                operations.append(b'')
            else:
                operations.append(opcode)
        return CTxOut(output.nValue, CScript(operations))
    else:
        return CTxOut(output.nValue, output.scriptPubKey)

"""
    The signature construct is similar to the SegwitSignatureHash construct with the exception
    of using the target output in place of the target input and a change in the data of SIGHASH_ANYONECANPAY.
"""
def SignatureHashOutput(scriptCode, txTo, nOut, hashtype, amount):
    hashPrevouts = 0
    hashSequence = 0
    hashOutputs = 0

    if not (hashtype & SIGHASH_ANYONECANPAY):
        serialize_prevouts = bytes()
        for i in txTo.vin:
            serialize_prevouts += i.prevout.serialize()
        hashPrevouts = uint256_from_str(hash256(serialize_prevouts))
    else:
        hashPrevouts = uint256_from_str(hash256(txTo.vin[0].prevout.serialize()))
        hashSequence = uint256_from_str(hash256(struct.pack("<I", txTo.vin[0].nSequence)))

    if (not (hashtype & SIGHASH_ANYONECANPAY) and (hashtype & 0x1f) != SIGHASH_SINGLE and (hashtype & 0x1f) != SIGHASH_NONE):
        serialize_sequence = bytes()
        for i in txTo.vin:
            serialize_sequence += struct.pack("<I", i.nSequence)
        hashSequence = uint256_from_str(hash256(serialize_sequence))

    if ((hashtype & 0x1f) != SIGHASH_SINGLE and (hashtype & 0x1f) != SIGHASH_NONE):
        serialize_outputs = bytes()
        for o in txTo.vout:
            serialize_outputs += GetOutputWithoutSenderSig(o).serialize()
        hashOutputs = uint256_from_str(hash256(serialize_outputs))
    elif ((hashtype & 0x1f) == SIGHASH_SINGLE and nOut < len(txTo.vout)):
        # Isn't this just repeating the part of the signature done below?
        serialize_outputs = GetOutputWithoutSenderSig(txTo.vout[nOut]).serialize()
        hashOutputs = uint256_from_str(hash256(serialize_outputs))

    ss = bytes()
    ss += struct.pack("<i", txTo.nVersion)
    ss += ser_uint256(hashPrevouts)
    ss += ser_uint256(hashSequence)
    ss += GetOutputWithoutSenderSig(txTo.vout[nOut]).serialize()
    ss += ser_string(scriptCode)
    ss += struct.pack("<q", amount)
    ss += ser_uint256(hashOutputs)
    ss += struct.pack("<i", txTo.nLockTime)
    ss += struct.pack("<I", hashtype)

    return hash256(ss)

def sign_op_spend_outputs(tx, keys, hashtypes):
    key_index = 0
    for i in range(len(tx.vout)):
        outputOperations = [op for op in CScript(tx.vout[i].scriptPubKey)]
        if any(op == OP_SENDER for op in outputOperations):
            inputScriptCode = [OP_DUP, OP_HASH160, hash160(keys[key_index].get_pubkey().get_bytes()), OP_EQUALVERIFY, OP_CHECKSIG]
            sighash = SignatureHashOutput(CScript(inputScriptCode), tx, i, hashtypes[i], tx.vout[i].nValue)
            signature = keys[key_index].sign_ecdsa(sighash)
            outputOperations[2] = ser_string(CScript([signature + struct.pack('B', hashtypes[i]), keys[key_index].get_pubkey().get_bytes()]))
            tx.vout[key_index].scriptPubKey = CScript(outputOperations)
            key_index += 1
    return tx


# assume SIGHASH_ALL
def sign_transaction_sighash_all(tx, key):
    pre_input_sigs = []
    post_input_sigs = []
    for inpt in tx.vin:
        pre_input_sigs.append(CScript([key.get_pubkey().get_bytes(), OP_CHECKSIG]))

    for i in range(len(tx.vin)):
        for j in range(len(tx.vin)):
            if j != i:
                tx.vin[j].scriptSig = CScript([])
            else:
                tx.vin[j].scriptSig = pre_input_sigs[j]

        sigdata = tx.serialize() + b'\x01\x00\x00\x00'
        sighash = hash256(sigdata)
        sigtx = key.sign_ecdsa(sighash, low_s=True)
        post_input_sigs.append(CScript([sigtx + b'\x01']))

    for i in range(len(tx.vin)):
        tx.vin[i].scriptSig = post_input_sigs[i]

    return tx

def wif_to_ECKey(wif):
    _, privkey, _ = base58_to_byte(wif, 38)
    bytes_privkey = hex_str_to_bytes(str(privkey)[2:-1])
    key = ECKey()
    # Assume always compressed, ignore last byte which specifies compression
    key.set(bytes_privkey[:-1], True)
    return key

HASHTYPES = [SIGHASH_ALL, SIGHASH_SINGLE, SIGHASH_NONE, SIGHASH_ANYONECANPAY]

class QtumOpSenderTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [['-opsenderheight=1000000', '-acceptnonstdtxn']] * 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def create_op_sender_tx(self, keys, outputs, hashtypes, gasCost, publish=True):
        unspent = self.unspents.pop()
        amount = sum(output.nValue for output in outputs)
        change = int(unspent['amount'])*COIN - gasCost - amount
        wif = self.nodes[0].dumpprivkey(unspent['address'])
        vin_key = wif_to_ECKey(wif)
        tx = CTransaction()
        prevout = COutPoint(int(unspent['txid'], 16), unspent['vout'])
        tx.vin = [CTxIn(prevout, scriptSig=CScript([vin_key.get_pubkey().get_bytes(), OP_CHECKSIG]))]
        tx.vout = outputs[:]
        tx.vout.append(CTxOut(change, scriptPubKey=CScript([OP_TRUE])))
        # Sign the outputs
        tx = sign_op_spend_outputs(tx, keys, hashtypes)
        # Sign the inputs
        tx = sign_transaction_sighash_all(tx, vin_key)
        if publish:
            txid = self.nodes[0].sendrawtransaction(bytes_to_hex_str(tx.serialize()))
            self.nodes[0].getrawtransaction(txid)
            self.sync_all()
            self.nodes[1].getrawtransaction(txid)
        unspent['key'] = vin_key
        return tx, unspent
        
    def single_op_sender_op_create_tx_test(self):
        num_contracts = len(self.nodes[0].listcontracts().keys())
        bytecode = '60606040523415600b57fe5b5b60398060196000396000f30060606040525b600b5b5b565b0000a165627a7a72305820e3bed070fd3a81dd00e02efd22d18a3b47b70860155d6063e47e1e2674fc5acb0029'
        key = ECKey()
        key.set(hash256(b'\x01'), True)
        output = CTxOut(0, CScript([CScriptNum(1), hash160(key.get_pubkey().get_bytes()), b'', OP_SENDER, b'\x04', CScriptNum(1000000), CScriptNum(40), hex_str_to_bytes(bytecode), OP_CREATE]))

        for hashtype in HASHTYPES:
            self.create_op_sender_tx([key], [output], [hashtype], 40000000)

        self.nodes[0].generate(10)
        self.sync_all()
        assert_equal(len(self.nodes[0].listcontracts().keys()), num_contracts+len(HASHTYPES))
        assert_equal(self.nodes[0].listcontracts(), self.nodes[1].listcontracts())

    def single_op_sender_op_call_tx_test(self):
        contract_address = sorted(self.nodes[0].listcontracts().keys())[-1]
        key = ECKey()
        key.set(hash256(b'\x01'), True)
        output = CTxOut(COIN, CScript([CScriptNum(1), hash160(key.get_pubkey().get_bytes()), b'', OP_SENDER, 
                b'\x04', CScriptNum(1000000), CScriptNum(40), hex_str_to_bytes("00"), hex_str_to_bytes(contract_address), OP_CALL]))

        for hashtype in HASHTYPES:
            self.create_op_sender_tx([key], [output], [hashtype], 40000000)

        self.nodes[0].generate(10)
        self.sync_all()
        assert_equal(self.nodes[0].listcontracts()[contract_address], len(HASHTYPES))

    def all_sigtypes_outputs_in_single_tx_test(self):
        contract_address = sorted(self.nodes[0].listcontracts().keys())[-1]
        old_balance = self.nodes[0].listcontracts()[contract_address]
        outputs = []
        keys = []
        for i, hashtype in enumerate(HASHTYPES):
            key = ECKey()
            key.set(hash256(hex_str_to_bytes(hex(i)[2:].zfill(4))), True)
            output = CTxOut(COIN, CScript([CScriptNum(1), hash160(key.get_pubkey().get_bytes()), b'', OP_SENDER, 
                    b'\x04', CScriptNum(1000000), CScriptNum(40), hex_str_to_bytes("00"), hex_str_to_bytes(contract_address), OP_CALL]))
            keys.append(key)
            outputs.append(output)

        self.create_op_sender_tx(keys, outputs, HASHTYPES, len(outputs)*40000000)
        self.nodes[0].generate(10)
        assert_equal(self.nodes[0].listcontracts()[contract_address], old_balance+len(outputs))

    """
        Make sure that signed outputs cannot be reused in different transactions
    """
    def replay_sighash_op_sender_output_test(self):
        contract_address = sorted(self.nodes[0].listcontracts().keys())[-1]
        old_balance = self.nodes[0].listcontracts()[contract_address]
        key = ECKey()
        key.set(hash256(b'\x01'), True)
        for hashtype in HASHTYPES:
            output = CTxOut(COIN, CScript([CScriptNum(1), hash160(key.get_pubkey().get_bytes()), b'', OP_SENDER, 
                    b'\x04', CScriptNum(1000000), CScriptNum(40), hex_str_to_bytes("00"), hex_str_to_bytes(contract_address), OP_CALL]))
            old_tx, input_txout = self.create_op_sender_tx([key], [output], [hashtype], 40000000, publish=True)
            self.nodes[0].generate(1)

            unspent = self.unspents.pop()
            amount = COIN
            change = int(unspent['amount'])*COIN - 40000000 - amount
            wif = self.nodes[0].dumpprivkey(unspent['address'])
            vin_key = wif_to_ECKey(wif)
            tx = CTransaction()
            prevout = COutPoint(int(unspent['txid'], 16), unspent['vout'])
            tx.vin = [CTxIn(prevout, scriptSig=CScript([vin_key.get_pubkey().get_bytes(), OP_CHECKSIG]))]
            tx.vout = [old_tx.vout[0]]
            tx.vout.append(CTxOut(change, scriptPubKey=CScript([OP_TRUE])))
            # Sign the inputs
            tx = sign_transaction_sighash_all(tx, vin_key)
            assert_raises_rpc_error(-26, 'sender-output-script-verify-failed', self.nodes[0].sendrawtransaction, bytes_to_hex_str(tx.serialize()))

    def mixed_opsender_and_non_op_sender_tx_and_refund_verification_test(self):
        self.nodes[0].generate(1)
        assert_equal(self.nodes[0].getrawmempool(), [])
        old_contracts = self.nodes[0].listcontracts().keys()
        """
        pragma solidity ^0.5.2;

        contract Test {
            address private creator;
            mapping(address => uint) private balances;

            constructor() public {
                creator = msg.sender;
            }

            function getBalance(address key) external view returns (uint) {
                return balances[key];
            }

            function () payable external {
                balances[msg.sender] += msg.value;
            }

            function getCreator() external view returns (address) {
                return creator;
            }
        }
        f8b2cb4f: getBalance(address)
        0ee2cb10: getCreator()
        """
        bytecode = "608060405234801561001057600080fd5b50336000806101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055506101ef806100606000396000f3fe608060405260043610610046576000357c0100000000000000000000000000000000000000000000000000000000900480630ee2cb1014610095578063f8b2cb4f146100ec575b34600160003373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002060008282540192505081905550005b3480156100a157600080fd5b506100aa610151565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b3480156100f857600080fd5b5061013b6004803603602081101561010f57600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff16906020019092919050505061017a565b6040518082815260200191505060405180910390f35b60008060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16905090565b6000600160008373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002054905091905056fea165627a7a72305820cec11cb4c48d27aed8e6cd757d284387d8a017d594555d9b0d1b6f4d331441ac0029"
        key = ECKey()
        key.set(hash256(b'\x00'), True)
        outputs = [
            CTxOut(0, CScript([CScriptNum(1), hash160(key.get_pubkey().get_bytes()), b'', OP_SENDER, b'\x04', CScriptNum(1000000), CScriptNum(40), hex_str_to_bytes(bytecode), OP_CREATE])),
            CTxOut(0, CScript([b'\x04', CScriptNum(1000000), CScriptNum(40), hex_str_to_bytes(bytecode), OP_CREATE]))
        ]
        tx, input_txout = self.create_op_sender_tx([key], outputs, [SIGHASH_ALL], 2*40000000)
        block_hash = self.nodes[0].generate(1)[0]
        new_contracts = list(set(self.nodes[0].listcontracts().keys()) - set(old_contracts))
        assert_equal(len(new_contracts), 2)

        creators = [
            hex(int(self.nodes[0].callcontract(new_contracts[0], "0ee2cb10")['executionResult']['output'], 16))[2:].zfill(40),
            hex(int(self.nodes[0].callcontract(new_contracts[1], "0ee2cb10")['executionResult']['output'], 16))[2:].zfill(40)
        ]

        expected_creators = [
            bytes_to_hex_str(hash160(key.get_pubkey().get_bytes())),
            p2pkh_to_hex_hash(input_txout['address'])
        ]
        assert_equal(sorted(creators), sorted(expected_creators))

        # Make sure that gas is refunded to vin0's address
        coinbase_txid = self.nodes[0].getblock(block_hash)['tx'][0]
        coinbase_vout = self.nodes[0].decoderawtransaction(self.nodes[0].gettransaction(coinbase_txid, True)['hex'])['vout']
        assert_equal(input_txout['address'], coinbase_vout[1]['scriptPubKey']['address'])
        assert_equal(input_txout['address'], coinbase_vout[2]['scriptPubKey']['address'])

        contract_address = new_contracts[0]
        key = ECKey()
        key.set(hash256(b'\x01'), True)
        outputs = [
            CTxOut(COIN, CScript([CScriptNum(1), hash160(key.get_pubkey().get_bytes()), b'', OP_SENDER, b'\x04', CScriptNum(1000000), CScriptNum(40), hex_str_to_bytes("00"), hex_str_to_bytes(contract_address), OP_CALL])),
            CTxOut(COIN, CScript([b'\x04', CScriptNum(1000000), CScriptNum(40), hex_str_to_bytes("00"), hex_str_to_bytes(contract_address), OP_CALL]))
        ]
        tx, input_txout = self.create_op_sender_tx([key], outputs, [SIGHASH_ALL], 2*40000000)
        self.nodes[0].generate(1)
        for sender in [bytes_to_hex_str(hash160(key.get_pubkey().get_bytes())), p2pkh_to_hex_hash(input_txout['address'])]:
            balance = int(self.nodes[0].callcontract(new_contracts[0], "f8b2cb4f"+(sender.zfill(64)))['executionResult']['output'], 16)
            assert_equal(COIN, balance)

    def large_contract_deployment_tx_test(self):
        key = ECKey()
        key.set(hash256(b'\x01'), True)
        output = CTxOut(0, CScript([CScriptNum(1), hash160(key.get_pubkey().get_bytes()), b'', OP_SENDER, b'\x04', CScriptNum(1000000), CScriptNum(50), b'\x00'*100000, OP_CREATE]))
        tx, input_txout = self.create_op_sender_tx([key], [output], [SIGHASH_ALL], 50*1000000)
        txid = self.nodes[0].sendrawtransaction(bytes_to_hex_str(tx.serialize()))
        self.nodes[0].getrawtransaction(txid)
        self.sync_all()
        self.nodes[1].getrawtransaction(txid)

    def invalid_op_sender_signature_tx_test(self):
        contract_address = sorted(self.nodes[0].listcontracts().keys())[-1]
        key = ECKey()
        key.set(hash256(b'\x01'), False)
        output = CTxOut(COIN, CScript([CScriptNum(1), hash160(key.get_pubkey().get_bytes()), b'', OP_SENDER, 
                b'\x04', CScriptNum(1000000), CScriptNum(40), hex_str_to_bytes("00"), OP_CREATE]))

        for hashtype in HASHTYPES:
            tx, input_txout = self.create_op_sender_tx([key], [output], [hashtype], 40000000, False)
            ops = [op for op in tx.vout[0].scriptPubKey]
            signature = ops[2]
            # Try inverting every byte one by one in the signature, then
            for i in range(0, len(signature)):
                inverted_sig_byte = struct.pack('B', ~signature[i] & 0xff) 
                modified_signature = signature[:i] + inverted_sig_byte + signature[i+1:]
                ops[2] = modified_signature
                tx.vout[0].scriptPubKey = CScript(ops)
                tx = sign_transaction_sighash_all(tx, input_txout['key'])
                tx.rehash()
                # if we change either of these bytes, we get messed up data push sizes and trigger another error
                if i in [0, 1, 74 if len(signature) == 140 else 73]:
                    assert_raises_rpc_error(-26, 'bad-txns-invalid-sender-script', self.nodes[0].sendrawtransaction, bytes_to_hex_str(tx.serialize()))
                else:
                    assert_raises_rpc_error(-26, 'sender-output-script-verify-failed', self.nodes[0].sendrawtransaction, bytes_to_hex_str(tx.serialize()))

                tip = self.nodes[0].getblock(self.nodes[0].getbestblockhash())
                block = create_block(int(tip['hash'], 16), create_coinbase(tip['height'] + 1), tip['time'])
                block.vtx.append(tx)
                block.hashMerkleRoot = block.calc_merkle_root()
                block.solve()
                assert(self.nodes[0].submitblock(bytes_to_hex_str(block.serialize())) != None)
            
            # Make sure that the unmodified tx can be submitted without error
            ops[2] = signature
            tx.vout[0].scriptPubKey = CScript(ops)
            tx = sign_transaction_sighash_all(tx, input_txout['key'])
            txid = self.nodes[0].sendrawtransaction(bytes_to_hex_str(tx.serialize()))
            self.nodes[0].getrawtransaction(txid)
            self.sync_all()
            self.nodes[1].getrawtransaction(txid)


    def invalid_op_sender_address_tx_test(self):
        contract_address = sorted(self.nodes[0].listcontracts().keys())[-1]
        key = ECKey()
        key.set(hash256(b'\x01'), True)
        output = CTxOut(COIN, CScript([CScriptNum(1), hash160(key.get_pubkey().get_bytes()), b'', OP_SENDER, 
                b'\x04', CScriptNum(1000000), CScriptNum(40), hex_str_to_bytes("00"), OP_CREATE]))

        for hashtype in HASHTYPES:
            tx, input_txout = self.create_op_sender_tx([key], [output], [hashtype], 40000000, False)
            ops = [op for op in tx.vout[0].scriptPubKey]
            address = ops[1]
            # Try inverting every byte one by one in the address, then
            #  - send to mempool
            #  - submit in a block
            for i in range(0, len(address)):
                inverted_address_byte = struct.pack('B', ~address[i] & 0xff) 
                modified_address = address[:i] + inverted_address_byte + address[i+1:]
                ops[1] = modified_address
                tx.vout[0].scriptPubKey = CScript(ops)
                tx = sign_transaction_sighash_all(tx, input_txout['key'])
                assert_raises_rpc_error(-26, 'sender-output-script-verify-failed', self.nodes[0].sendrawtransaction, bytes_to_hex_str(tx.serialize()))
                tip = self.nodes[0].getblock(self.nodes[0].getbestblockhash())
                block = create_block(int(tip['hash'], 16), create_coinbase(tip['height'] + 1), tip['time'])
                block.vtx.append(tx)
                block.hashMerkleRoot = block.calc_merkle_root()
                block.solve()
                assert(self.nodes[0].submitblock(bytes_to_hex_str(block.serialize())) != None)
            
            # Make sure that the unmodified tx can be submitted without error
            ops[1] = address
            tx.vout[0].scriptPubKey = CScript(ops)
            tx = sign_transaction_sighash_all(tx, input_txout['key'])
            txid = self.nodes[0].sendrawtransaction(bytes_to_hex_str(tx.serialize()))
            self.nodes[0].getrawtransaction(txid)
            self.sync_all()
            self.nodes[1].getrawtransaction(txid)


    def non_standard_op_sender_test(self):
        unspent = self.unspents.pop()
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(unspent['txid'], 16), unspent['vout']), scriptSig=CScript([]))]
        tx.vout = [CTxOut(int(unspent['amount'])*COIN-100000, scriptPubKey=CScript([OP_SENDER]))]
        tx = rpc_sign_transaction(self.nodes[0], tx)
        assert_raises_rpc_error(-26, 'bad-txns-contract-nonstandard', self.nodes[0].sendrawtransaction, bytes_to_hex_str(tx.serialize()))


    def pre_hf_single_op_sender_op_create_tx_rejected_test(self):
        num_contracts = len(self.nodes[0].listcontracts().keys())
        bytecode = '60606040523415600b57fe5b5b60398060196000396000f30060606040525b600b5b5b565b0000a165627a7a72305820e3bed070fd3a81dd00e02efd22d18a3b47b70860155d6063e47e1e2674fc5acb0029'
        key = ECKey()
        key.set(hash256(b'\x01'), True)
        output = CTxOut(0, CScript([CScriptNum(1), hash160(key.get_pubkey().get_bytes()), b'', OP_SENDER, b'\x04', CScriptNum(1000000), CScriptNum(40), hex_str_to_bytes(bytecode), OP_CREATE]))

        for hashtype in HASHTYPES:
            tx, _ = self.create_op_sender_tx([key], [output], [hashtype], 40000000, publish=False)
            assert_raises_rpc_error(-26, 'bad-txns-invalid-sender', self.nodes[0].sendrawtransaction, bytes_to_hex_str(tx.serialize()))

    # Make sure that the OP_SENDER opcode cannot be used in a vin before HF
    # If it were possible, then the chain would split due to "old" nodes would interpret OP_SENDER as OP_UNKNOWN
    def pre_hf_op_sender_in_vin_rejected_test(self):
        unspent = self.unspents.pop()
        input_tx = CTransaction()
        input_tx.vin = [CTxIn(COutPoint(int(unspent['txid'], 16), unspent['vout']), scriptSig=CScript([]))]
        input_tx.vout = [CTxOut(int(unspent['amount'])*COIN-100000, scriptPubKey=CScript([OP_TRUE]))]
        input_tx = rpc_sign_transaction(self.nodes[0], input_tx)
        input_txid = self.nodes[0].sendrawtransaction(bytes_to_hex_str(input_tx.serialize()))

        spending_tx = CTransaction()
        spending_tx.vin = [CTxIn(COutPoint(int(input_txid, 16), 0), scriptSig=CScript([OP_SENDER]))]
        spending_tx.vout = [CTxOut(int(unspent['amount'])*COIN-200000, scriptPubKey=CScript([OP_TRUE]*40))]
        try:
            txid = self.nodes[0].sendrawtransaction(bytes_to_hex_str(spending_tx.serialize()))
            self.nodes[0].gettransaction(txid)
        except:
            # Success
            return
        # Sending a tx with OP_SENDER in the scriptSig should fail
        assert(False)

    def pre_hf_double_op_sender_in_vout_rejected_test(self):
        unspent = self.unspents.pop()
        input_tx = CTransaction()
        input_tx.vin = [CTxIn(COutPoint(int(unspent['txid'], 16), unspent['vout']), scriptSig=CScript([]))]
        input_tx.vout = [CTxOut(int(unspent['amount'])*COIN-100000, scriptPubKey=CScript([OP_TRUE, OP_SENDER, OP_SENDER]))]
        input_tx = rpc_sign_transaction(self.nodes[0], input_tx)
        input_txid = self.nodes[0].sendrawtransaction(bytes_to_hex_str(input_tx.serialize()))

        spending_tx = CTransaction()
        spending_tx.vin = [CTxIn(COutPoint(int(input_txid, 16), 0), scriptSig=CScript([]))]
        spending_tx.vout = [CTxOut(int(unspent['amount'])*COIN-200000, scriptPubKey=CScript([OP_TRUE]*40))]
        try:
            txid = self.nodes[0].sendrawtransaction(bytes_to_hex_str(spending_tx.serialize()))
            pp.pprint(self.nodes[0].gettransaction(txid, True))
        except:
            # Success
            return
        # Sending a tx with OP_SENDER in the scriptSig should fail
        assert(False)

    def run_test(self):
        wif = self.nodes[0].dumpprivkey(self.nodes[0].getnewaddress())
        key = wif_to_ECKey(wif)
        for i in range(100+COINBASE_MATURITY):
            block = create_block(int(self.nodes[0].getbestblockhash(), 16), create_coinbase(self.nodes[0].getblockcount()+1), int(time.time()))
            block.vtx[0].vout[0].scriptPubKey = CScript([key.get_pubkey().get_bytes(), OP_CHECKSIG])
            block.vtx[0].rehash()
            block.hashMerkleRoot = block.calc_merkle_root()
            block.solve()
            self.nodes[0].submitblock(bytes_to_hex_str(block.serialize()))

        self.unspents = self.nodes[0].listunspent()
        
        # Make sure that op_sender txs are not accepted before the HF
        self.pre_hf_single_op_sender_op_create_tx_rejected_test()
        self.pre_hf_op_sender_in_vin_rejected_test()
        self.pre_hf_double_op_sender_in_vout_rejected_test()
        self.nodes[0].generate(1)
        self.sync_all()

        # Trigger the HF
        self.restart_node(0, ['-opsenderheight=602', '-acceptnonstdtxn'])
        self.restart_node(1, ['-opsenderheight=602', '-acceptnonstdtxn'])
        self.connect_nodes(0, 1)
        
        self.single_op_sender_op_create_tx_test()
        self.single_op_sender_op_call_tx_test()
        self.all_sigtypes_outputs_in_single_tx_test()
        self.replay_sighash_op_sender_output_test()
        self.mixed_opsender_and_non_op_sender_tx_and_refund_verification_test()
        self.large_contract_deployment_tx_test()
        self.invalid_op_sender_signature_tx_test()
        self.invalid_op_sender_address_tx_test()


if __name__ == '__main__':
    QtumOpSenderTest().main()
