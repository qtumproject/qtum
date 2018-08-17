This document describes breaking changes between Qtum releases for both RPC APIs and for blockchain consensus.

# Sparknet (Testnet v1)

Initial release

# TBD (Testnet v2)

## Consensus Parameters/Rules

## EVM Behavior

* Made block.coinbase use block.vtx[1].vout[1] for the coinbase address of PoS blocks
* Added check to coinbase to ensure that it is 0 if not a standard pubkeyhash address

## RPC APIs

