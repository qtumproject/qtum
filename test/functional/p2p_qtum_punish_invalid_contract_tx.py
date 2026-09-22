#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.qtum import *
from test_framework.qtumconfig import *
from test_framework.messages import msg_tx


class QtumP2PPunishInvalidContractTxTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-acceptnonstdtxn']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        self.generate(node, 10 + COINBASE_MATURITY)
        self.test_invalid_sender_script_punishment()
        self.test_gas_exceeds_limit_punishment()

    def test_invalid_sender_script_punishment(self):
        node = self.nodes[0]
        self.log.info("Test that peer is punished for sending tx with invalid sender script (TX_INVALID_SENDER_SCRIPT)")

        # Create a parent UTXO whose scriptPubKey is OP_TRUE*21 (not P2PKH/P2PK)
        parent_tx = CTransaction()
        parent_tx.vin = [make_vin(node, COIN + 1000000)]
        parent_tx.vout = [CTxOut(int(COIN), CScript([OP_TRUE] * 21))]
        parent_tx_hex = node.signrawtransactionwithwallet(bytes_to_hex_str(parent_tx.serialize()))['hex']
        parent_tx_id = node.sendrawtransaction(parent_tx_hex)
        self.generate(node, 1)

        # Spend the non-P2PKH UTXO in a tx with an OP_CREATE output.
        # CheckSenderScript() checks vin[0]'s prevout scriptPubKey; since it's
        # OP_TRUE*21 (not P2PKH/P2PK), it returns false -> TX_INVALID_SENDER_SCRIPT.
        bad_tx = CTransaction()
        bad_tx.vin = [CTxIn(COutPoint(int(parent_tx_id, 16), 0), scriptSig=CScript([OP_DROP] * 20), nSequence=0)]
        bad_tx.vout = [CTxOut(0, CScript([b"\x04", CScriptNum(1000000), CScriptNum(QTUM_MIN_GAS_PRICE), b"\x00", OP_CREATE]))]

        peer = node.add_p2p_connection(P2PInterface())
        with node.assert_debug_log(['Misbehaving']):
            peer.send_without_ping(msg_tx(bad_tx))
            peer.wait_for_disconnect(timeout=10)
        node.disconnect_p2ps()

    def test_gas_exceeds_limit_punishment(self):
        node = self.nodes[0]
        self.log.info("Test that peer is punished for sending tx with gas exceeding block limit (TX_GAS_EXCEEDS_LIMIT)")

        # Deploy a simple contract so we have a valid contract address for OP_CALL
        contract_bytecode = "60606040523415600e57600080fd5b5b605380601c6000396000f30060606040525b5b600160008082825401925050819055505b6103e85a11156024576017565b5b0000a165627a7a723058209aba4fa1dc462e2f52351eade3edc2974f25ddc404d415ce54df8d02eba21dd10029"
        contract_address = node.createcontract(contract_bytecode)['address']
        self.generate(node, 1)

        # Create a tx with 3 OP_CALL outputs, each with gas_limit=19998999.
        # Sum = 59996997 > 40000000 (default block gas limit) -> TX_GAS_EXCEEDS_LIMIT
        bad_tx = CTransaction()
        bad_tx.vin = [make_vin(node, int(20000 * COIN))]
        bad_tx.vout = [
            CTxOut(0, CScript([b"\x04", CScriptNum(19998999), CScriptNum(QTUM_MIN_GAS_PRICE), hex_str_to_bytes("00"), hex_str_to_bytes(contract_address), OP_CALL])),
            CTxOut(0, CScript([b"\x04", CScriptNum(19998999), CScriptNum(QTUM_MIN_GAS_PRICE), hex_str_to_bytes("00"), hex_str_to_bytes(contract_address), OP_CALL])),
            CTxOut(0, CScript([b"\x04", CScriptNum(19998999), CScriptNum(QTUM_MIN_GAS_PRICE), hex_str_to_bytes("00"), hex_str_to_bytes(contract_address), OP_CALL])),
        ]
        signed_tx_hex = node.signrawtransactionwithwallet(bytes_to_hex_str(bad_tx.serialize()))['hex']
        bad_tx.deserialize(BytesIO(hex_str_to_bytes(signed_tx_hex)))

        peer = node.add_p2p_connection(P2PInterface())
        with node.assert_debug_log(['Misbehaving']):
            peer.send_without_ping(msg_tx(bad_tx))
            peer.wait_for_disconnect(timeout=10)
        node.disconnect_p2ps()


if __name__ == '__main__':
    QtumP2PPunishInvalidContractTxTest(__file__).main()
