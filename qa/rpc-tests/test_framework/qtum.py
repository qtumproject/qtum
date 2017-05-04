# helpers for qtum
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *

def make_vin(node, value):
    addr = node.getrawchangeaddress()
    txid_hex = node.sendtoaddress(addr, value/COIN)
    txid = int(txid_hex, 16)
    node.generate(1)
    raw_tx = node.getrawtransaction(txid_hex, 1)

    for vout_index, txout in enumerate(raw_tx['vout']):
        if txout['scriptPubKey']['addresses'] == [addr]:
            break
    else:
        assert False

    return CTxIn(COutPoint(txid, vout_index), nSequence=0)

def make_op_call_output(value, version, gas_limit, gas_price, data, contract):
    scriptPubKey = CScript()
    scriptPubKey += version
    scriptPubKey += gas_limit
    scriptPubKey += gas_price
    scriptPubKey += data
    scriptPubKey += contract
    scriptPubKey += OP_CALL
    return CTxOut(value, scriptPubKey)

def make_transaction(node, vin, vout):
    tx = CTransaction()
    tx.vin = vin
    tx.vout = vout
    tx.rehash()
    
    unsigned_raw_tx = bytes_to_hex_str(tx.serialize_without_witness())
    signed_raw_tx = node.signrawtransaction(unsigned_raw_tx)['hex']
    return signed_raw_tx

def assert_vout(tx, expected_vout):
    assert_equal(len(tx['vout']), len(expected_vout))
    matches = []
    for expected in expected_vout:
        for i in range(len(tx['vout'])):
            if i not in matches and expected[0] == tx['vout'][i]['value'] and expected[1] == tx['vout'][i]['scriptPubKey']['type']:
                matches.append(i)
                break
    assert_equal(len(matches), len(expected_vout))

def assert_vin(tx, expected_vin):
    assert_equal(len(tx['vin']), len(expected_vin))
    matches = []
    for expected in expected_vin:
        for i in range(len(tx['vin'])):
            if i not in matches and expected[0] == tx['vin'][i]['scriptSig']['asm']:
                matches.append(i)
                break
    assert_equal(len(matches), len(expected_vin))
