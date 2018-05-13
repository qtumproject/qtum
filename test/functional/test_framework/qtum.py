from .address import *

def convert_btc_address_to_qtum(addr, main=False):
    version, hsh, checksum = base58_to_byte(addr, 25)
    if version == 111:
        return keyhash_to_p2pkh(binascii.unhexlify(hsh), main)

    if version == 196:
        return scripthash_to_p2sh(binascii.unhexlify(hsh), main)
    assert(False)
