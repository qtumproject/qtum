# ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
# Copyright 2019 Pawel Bylica.
# Licensed under the Apache License, Version 2.0.

import unittest

import ethash


class TestEthash(unittest.TestCase):
    epoch_number = 0
    nonce = 0x4242424242424242
    header_hash = bytes.fromhex(
        '2a8de2adf89af77358250bf908bf04ba94a6e8c3ba87775564a41d269a05e4ce')
    mix_hash = bytes.fromhex(
        '58f759ede17a706c93f13030328bcea40c1d1341fb26f2facd21ceb0dae57017')
    final_hash = bytes.fromhex(
        'dd47fd2d98db51078356852d7c4014e6a5d6c387c35f40e2875b74a256ed7906')

    def test_keccak(self):
        h256 = ('c5d2460186f7233c927e7db2dcc703c0'
                'e500b653ca82273b7bfad8045d85a470')
        h512 = ('0eab42de4c3ceb9235fc91acffe746b2'
                '9c29a8c366b7c60e4e67c466f36a4304'
                'c00fa9caf9d87976ba469bcbe06713b4'
                '35f091ef2769fb160cdab33d3670680e')

        self.assertEqual(ethash.keccak_256(b'').hex(), h256)
        self.assertEqual(ethash.keccak_512(b'').hex(), h512)

    def test_hash(self):
        f, m = ethash.hash(0, self.header_hash, self.nonce)
        self.assertEqual(m, self.mix_hash)
        self.assertEqual(f, self.final_hash)

    def test_verify(self):
        t = ethash.verify(0, self.header_hash, self.mix_hash, self.nonce,
                          self.final_hash)
        self.assertTrue(t)
        self.assertEqual(type(t), bool)


if __name__ == '__main__':
    unittest.main()
