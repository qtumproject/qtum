# Copyright (c) 2013-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

DIST_SUBDIRS = secp256k1 univalue

AM_LDFLAGS = $(PTHREAD_CFLAGS) $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS) $(GPROF_LDFLAGS) $(SANITIZER_LDFLAGS)
AM_CXXFLAGS = $(DEBUG_CXXFLAGS) $(HARDENED_CXXFLAGS) $(WARN_CXXFLAGS) $(NOWARN_CXXFLAGS) $(ERROR_CXXFLAGS) $(GPROF_CXXFLAGS) $(SANITIZER_CXXFLAGS) -DETH_FATDB=1
AM_CPPFLAGS = $(DEBUG_CPPFLAGS) $(HARDENED_CPPFLAGS) -DQTUM_BUILD
AM_LIBTOOLFLAGS = --preserve-dup-deps
EXTRA_LIBRARIES =

if EMBEDDED_UNIVALUE
LIBUNIVALUE = univalue/libunivalue.la

$(LIBUNIVALUE): $(wildcard univalue/lib/*) $(wildcard univalue/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)
else
LIBUNIVALUE = $(UNIVALUE_LIBS)
endif

BITCOIN_INCLUDES=-I$(builddir) $(BDB_CPPFLAGS) $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS) $(CRYPTO_CFLAGS) $(SSL_CFLAGS) -I$(srcdir)/cpp-ethereum/utils

BITCOIN_INCLUDES += -I$(srcdir)/secp256k1/include
BITCOIN_INCLUDES += -I$(srcdir)/libff/libff
BITCOIN_INCLUDES += -I$(srcdir)/libff
BITCOIN_INCLUDES += $(UNIVALUE_CFLAGS)

BITCOIN_INCLUDES += -I$(srcdir)/cpp-ethereum
BITCOIN_INCLUDES += -I$(srcdir)/cpp-ethereum/evmc/include
BITCOIN_INCLUDES += -I$(srcdir)/cpp-ethereum/ethash/include
BITCOIN_INCLUDES += -I$(srcdir)/cpp-ethereum/utils/libscrypt

LIBBITCOIN_SERVER=libbitcoin_server.a
LIBBITCOIN_COMMON=libbitcoin_common.a
LIBBITCOIN_CONSENSUS=libbitcoin_consensus.a
LIBBITCOIN_CLI=libbitcoin_cli.a
LIBBITCOIN_UTIL=libbitcoin_util.a
LIBBITCOIN_CRYPTO_BASE=crypto/libbitcoin_crypto_base.a
LIBBITCOINQT=qt/libbitcoinqt.a
LIBSECP256K1=secp256k1/libsecp256k1.la
LIBCRYPTOPP=cryptopp/libcryptopp.a
LIBFF=libff/libff.a


if ENABLE_ZMQ
LIBBITCOIN_ZMQ=libbitcoin_zmq.a
endif
if BUILD_BITCOIN_LIBS
LIBBITCOINCONSENSUS=libbitcoinconsensus.la
endif
if ENABLE_WALLET
LIBBITCOIN_WALLET=libbitcoin_wallet.a
LIBBITCOIN_WALLET_TOOL=libbitcoin_wallet_tool.a
endif

LIBBITCOIN_CRYPTO= $(LIBBITCOIN_CRYPTO_BASE)
if ENABLE_SSE41
LIBBITCOIN_CRYPTO_SSE41 = crypto/libbitcoin_crypto_sse41.a
LIBBITCOIN_CRYPTO += $(LIBBITCOIN_CRYPTO_SSE41)
endif
if ENABLE_AVX2
LIBBITCOIN_CRYPTO_AVX2 = crypto/libbitcoin_crypto_avx2.a
LIBBITCOIN_CRYPTO += $(LIBBITCOIN_CRYPTO_AVX2)
endif
if ENABLE_SHANI
LIBBITCOIN_CRYPTO_SHANI = crypto/libbitcoin_crypto_shani.a
LIBBITCOIN_CRYPTO += $(LIBBITCOIN_CRYPTO_SHANI)
endif

$(LIBSECP256K1): $(wildcard secp256k1/src/*.h) $(wildcard secp256k1/src/*.c) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES += \
  $(LIBBITCOIN_CRYPTO) \
  $(LIBBITCOIN_UTIL) \
  $(LIBBITCOIN_COMMON) \
  $(LIBBITCOIN_CONSENSUS) \
  $(LIBBITCOIN_SERVER) \
  $(LIBBITCOIN_CLI) \
  $(LIBBITCOIN_WALLET) \
  $(LIBBITCOIN_WALLET_TOOL) \
  $(LIBBITCOIN_ZMQ)

lib_LTLIBRARIES = $(LIBBITCOINCONSENSUS)

bin_PROGRAMS =
noinst_PROGRAMS =
TESTS =
BENCHMARKS =

if BUILD_BITCOIND
  bin_PROGRAMS += qtumd
endif

if BUILD_BITCOIN_CLI
  bin_PROGRAMS += qtum-cli
endif
if BUILD_BITCOIN_TX
  bin_PROGRAMS += qtum-tx
endif
if ENABLE_WALLET
if BUILD_BITCOIN_WALLET
  bin_PROGRAMS += qtum-wallet
endif
endif

.PHONY: FORCE check-symbols check-security
# bitcoin core #
BITCOIN_CORE_H = \
  addrdb.h \
  addrman.h \
  attributes.h \
  banman.h \
  base58.h \
  bech32.h \
  bloom.h \
  blockencodings.h \
  blockfilter.h \
  chain.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  checkpoints.h \
  checkqueue.h \
  clientversion.h \
  coins.h \
  compat.h \
  compat/assumptions.h \
  compat/byteswap.h \
  compat/endian.h \
  compat/sanity.h \
  compressor.h \
  consensus/consensus.h \
  consensus/tx_verify.h \
  core_io.h \
  core_memusage.h \
  cuckoocache.h \
  fs.h \
  httprpc.h \
  httpserver.h \
  index/base.h \
  index/txindex.h \
  indirectmap.h \
  init.h \
  interfaces/chain.h \
  interfaces/handler.h \
  interfaces/node.h \
  interfaces/wallet.h \
  key.h \
  key_io.h \
  keystore.h \
  dbwrapper.h \
  limitedmap.h \
  logging.h \
  memusage.h \
  merkleblock.h \
  miner.h \
  net.h \
  net_processing.h \
  netaddress.h \
  netbase.h \
  netmessagemaker.h \
  node/transaction.h \
  noui.h \
  optional.h \
  outputtype.h \
  policy/feerate.h \
  policy/fees.h \
  policy/policy.h \
  policy/rbf.h \
  pow.h \
  pos.h \
  protocol.h \
  psbt.h \
  random.h \
  reverse_iterator.h \
  reverselock.h \
  rpc/blockchain.h \
  rpc/client.h \
  rpc/mining.h \
  rpc/protocol.h \
  rpc/server.h \
  rpc/rawtransaction.h \
  rpc/register.h \
  rpc/util.h \
  scheduler.h \
  script/descriptor.h \
  script/ismine.h \
  script/sigcache.h \
  script/sign.h \
  script/standard.h \
  shutdown.h \
  streams.h \
  support/allocators/secure.h \
  support/allocators/zeroafterfree.h \
  support/cleanse.h \
  support/events.h \
  support/lockedpool.h \
  sync.h \
  threadsafety.h \
  threadinterrupt.h \
  timedata.h \
  torcontrol.h \
  txdb.h \
  txmempool.h \
  ui_interface.h \
  undo.h \
  util/bip32.h \
  util/bytevectorhash.h \
  util/system.h \
  util/memory.h \
  util/moneystr.h \
  util/time.h \
  util/convert.h \
  validation.h \
  validationinterface.h \
  versionbits.h \
  versionbitsinfo.h \
  walletinitinterface.h \
  wallet/coincontrol.h \
  wallet/crypter.h \
  wallet/db.h \
  wallet/feebumper.h \
  wallet/fees.h \
  wallet/psbtwallet.h \
  wallet/rpcwallet.h \
  wallet/wallet.h \
  wallet/walletdb.h \
  wallet/wallettool.h \
  wallet/walletutil.h \
  wallet/coinselection.h \
  warnings.h \
  zmq/zmqabstractnotifier.h \
  zmq/zmqconfig.h\
  zmq/zmqnotificationinterface.h \
  zmq/zmqpublishnotifier.h \
  zmq/zmqrpc.h \
  qtum/qtumstate.h \
  qtum/qtumtransaction.h \
  qtum/qtumDGP.h \
  qtum/storageresults.h \
  qtum/qtumutils.h


obj/build.h: FORCE
	@$(MKDIR_P) $(builddir)/obj
	@$(top_srcdir)/share/genbuild.sh "$(abs_top_builddir)/src/obj/build.h" \
	  "$(abs_top_srcdir)"
libbitcoin_util_a-clientversion.$(OBJEXT): obj/build.h

# server: shared between bitcoind and bitcoin-qt
libbitcoin_server_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(MINIUPNPC_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libbitcoin_server_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_server_a_SOURCES = \
  addrdb.cpp \
  addrman.cpp \
  banman.cpp \
  bloom.cpp \
  blockencodings.cpp \
  blockfilter.cpp \
  chain.cpp \
  checkpoints.cpp \
  consensus/tx_verify.cpp \
  httprpc.cpp \
  httpserver.cpp \
  index/base.cpp \
  index/txindex.cpp \
  interfaces/chain.cpp \
  interfaces/handler.cpp \
  interfaces/node.cpp \
  init.cpp \
  dbwrapper.cpp \
  merkleblock.cpp \
  miner.cpp \
  net.cpp \
  net_processing.cpp \
  node/transaction.cpp \
  noui.cpp \
  outputtype.cpp \
  policy/fees.cpp \
  policy/policy.cpp \
  policy/rbf.cpp \
  pow.cpp \
  pos.cpp \
  rest.cpp \
  rpc/blockchain.cpp \
  rpc/mining.cpp \
  rpc/misc.cpp \
  rpc/net.cpp \
  rpc/rawtransaction.cpp \
  rpc/server.cpp \
  rpc/util.cpp \
  script/sigcache.cpp \
  shutdown.cpp \
  timedata.cpp \
  torcontrol.cpp \
  txdb.cpp \
  txmempool.cpp \
  ui_interface.cpp \
  validation.cpp \
  validationinterface.cpp \
  versionbits.cpp \
  qtum/qtumstate.cpp \
  qtum/qtumtransaction.cpp \
  qtum/qtumDGP.cpp \
  consensus/consensus.cpp \
  qtum/storageresults.cpp \
  $(BITCOIN_CORE_H)

if !ENABLE_WALLET
libbitcoin_server_a_SOURCES += dummywallet.cpp
endif

if ENABLE_ZMQ
libbitcoin_zmq_a_CPPFLAGS = $(BITCOIN_INCLUDES) $(ZMQ_CFLAGS)
libbitcoin_zmq_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_zmq_a_SOURCES = \
  zmq/zmqabstractnotifier.cpp \
  zmq/zmqnotificationinterface.cpp \
  zmq/zmqpublishnotifier.cpp \
  zmq/zmqrpc.cpp
endif


# wallet: shared between bitcoind and bitcoin-qt, but only linked
# when wallet enabled
libbitcoin_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libbitcoin_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_wallet_a_SOURCES = \
  interfaces/wallet.cpp \
  wallet/coincontrol.cpp \
  wallet/crypter.cpp \
  wallet/db.cpp \
  wallet/feebumper.cpp \
  wallet/fees.cpp \
  wallet/init.cpp \
  wallet/psbtwallet.cpp \
  wallet/rpcdump.cpp \
  wallet/rpcwallet.cpp \
  wallet/wallet.cpp \
  wallet/walletdb.cpp \
  wallet/walletutil.cpp \
  wallet/coinselection.cpp \
  $(BITCOIN_CORE_H)

libbitcoin_wallet_tool_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libbitcoin_wallet_tool_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_wallet_tool_a_SOURCES = \
  wallet/wallettool.cpp \
  $(BITCOIN_CORE_H)

# crypto primitives library
crypto_libbitcoin_crypto_base_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libbitcoin_crypto_base_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -fPIC
crypto_libbitcoin_crypto_base_a_CFLAGS = $(AM_CXXFLAGS) -fPIC
crypto_libbitcoin_crypto_base_a_SOURCES = \
  crypto/aes.cpp \
  crypto/aes.h \
  crypto/chacha20.h \
  crypto/chacha20.cpp \
  crypto/common.h \
  crypto/hmac_sha256.cpp \
  crypto/hmac_sha256.h \
  crypto/hmac_sha512.cpp \
  crypto/hmac_sha512.h \
  crypto/ripemd160.cpp \
  crypto/ripemd160.h \
  crypto/sha1.cpp \
  crypto/sha1.h \
  crypto/sha256.cpp \
  crypto/sha256.h \
  crypto/sha512.cpp \
  crypto/sha512.h \
  crypto/siphash.cpp \
  crypto/siphash.h \
  cpp-ethereum/utils/libscrypt/b64.c \
  cpp-ethereum/utils/libscrypt/b64.h \
  cpp-ethereum/utils/libscrypt/crypto-mcf.c \
  cpp-ethereum/utils/libscrypt/crypto_scrypt-check.c \
  cpp-ethereum/utils/libscrypt/crypto_scrypt-hash.c \
  cpp-ethereum/utils/libscrypt/crypto_scrypt-hexconvert.c \
  cpp-ethereum/utils/libscrypt/crypto_scrypt-hexconvert.h \
  cpp-ethereum/utils/libscrypt/crypto_scrypt-nosse.c \
  cpp-ethereum/utils/libscrypt/crypto-scrypt-saltgen.c \
  cpp-ethereum/utils/libscrypt/libscrypt.h \
  cpp-ethereum/utils/libscrypt/sha256.c \
  cpp-ethereum/utils/libscrypt/sha256.h \
  cpp-ethereum/utils/libscrypt/slowequals.c \
  cpp-ethereum/utils/libscrypt/slowequals.h \
  cpp-ethereum/utils/libscrypt/sysendian.h

if USE_ASM
crypto_libbitcoin_crypto_base_a_SOURCES += crypto/sha256_sse4.cpp
endif

crypto_libbitcoin_crypto_sse41_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libbitcoin_crypto_sse41_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libbitcoin_crypto_sse41_a_CXXFLAGS += $(SSE41_CXXFLAGS)
crypto_libbitcoin_crypto_sse41_a_CPPFLAGS += -DENABLE_SSE41
crypto_libbitcoin_crypto_sse41_a_SOURCES = crypto/sha256_sse41.cpp

crypto_libbitcoin_crypto_avx2_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libbitcoin_crypto_avx2_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libbitcoin_crypto_avx2_a_CXXFLAGS += $(AVX2_CXXFLAGS)
crypto_libbitcoin_crypto_avx2_a_CPPFLAGS += -DENABLE_AVX2
crypto_libbitcoin_crypto_avx2_a_SOURCES = crypto/sha256_avx2.cpp

crypto_libbitcoin_crypto_shani_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libbitcoin_crypto_shani_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libbitcoin_crypto_shani_a_CXXFLAGS += $(SHANI_CXXFLAGS)
crypto_libbitcoin_crypto_shani_a_CPPFLAGS += -DENABLE_SHANI
crypto_libbitcoin_crypto_shani_a_SOURCES = crypto/sha256_shani.cpp

# consensus: shared between all executables that validate any consensus rules.
libbitcoin_consensus_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libbitcoin_consensus_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -fPIC
libbitcoin_consensus_a_SOURCES = \
  amount.h \
  arith_uint256.cpp \
  arith_uint256.h \
  consensus/merkle.cpp \
  consensus/merkle.h \
  consensus/params.h \
  consensus/validation.h \
  hash.cpp \
  hash.h \
  prevector.h \
  primitives/block.cpp \
  primitives/block.h \
  primitives/transaction.cpp \
  primitives/transaction.h \
  pubkey.cpp \
  pubkey.h \
  script/bitcoinconsensus.cpp \
  script/interpreter.cpp \
  script/interpreter.h \
  script/script.cpp \
  script/script.h \
  script/script_error.cpp \
  script/script_error.h \
  serialize.h \
  span.h \
  tinyformat.h \
  uint256.cpp \
  uint256.h \
  util/strencodings.cpp \
  util/strencodings.h \
  version.h

# common: shared between bitcoind, and bitcoin-qt and non-server tools
libbitcoin_common_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) -DQTUM_BUILD
libbitcoin_common_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_common_a_CFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -std=c99
libbitcoin_common_a_SOURCES = \
  base58.cpp \
  bech32.cpp \
  chainparams.cpp \
  coins.cpp \
  compressor.cpp \
  core_read.cpp \
  core_write.cpp \
  key.cpp \
  key_io.cpp \
  keystore.cpp \
  netaddress.cpp \
  netbase.cpp \
  policy/feerate.cpp \
  psbt.cpp \
  protocol.cpp \
  scheduler.cpp \
  script/descriptor.cpp \
  script/ismine.cpp \
  script/sign.cpp \
  script/standard.cpp \
  versionbitsinfo.cpp \
  warnings.cpp \
  qtum/qtumutils.cpp \
  cpp-ethereum/aleth/buildinfo.c \
  cpp-ethereum/aleth/buildinfo.h \
  cpp-ethereum/evmc/lib/instructions/instruction_metrics.c \
  cpp-ethereum/evmc/lib/instructions/instruction_names.c \
  cpp-ethereum/libdevcore/Address.cpp \
  cpp-ethereum/libdevcore/Address.h \
  cpp-ethereum/libdevcore/Base64.cpp \
  cpp-ethereum/libdevcore/Base64.h \
  cpp-ethereum/libdevcore/Common.cpp \
  cpp-ethereum/libdevcore/Common.h \
  cpp-ethereum/libdevcore/CommonData.cpp \
  cpp-ethereum/libdevcore/CommonData.h \
  cpp-ethereum/libdevcore/CommonIO.cpp \
  cpp-ethereum/libdevcore/CommonIO.h \
  cpp-ethereum/libdevcore/CommonJS.cpp \
  cpp-ethereum/libdevcore/CommonJS.h \
  cpp-ethereum/libdevcore/FileSystem.cpp \
  cpp-ethereum/libdevcore/FileSystem.h \
  cpp-ethereum/libdevcore/FixedHash.cpp \
  cpp-ethereum/libdevcore/FixedHash.h \
  cpp-ethereum/libdevcore/Guards.cpp \
  cpp-ethereum/libdevcore/Guards.h \
  cpp-ethereum/libdevcrypto/Hash.cpp \
  cpp-ethereum/libdevcrypto/Hash.h \
  cpp-ethereum/libdevcore/Log.cpp \
  cpp-ethereum/libdevcore/Log.h \
  cpp-ethereum/libdevcore/LevelDB.cpp \
  cpp-ethereum/libdevcore/LevelDB.h \
  cpp-ethereum/libdevcore/MemoryDB.cpp \
  cpp-ethereum/libdevcore/MemoryDB.h \
  cpp-ethereum/libdevcore/OverlayDB.cpp \
  cpp-ethereum/libdevcore/OverlayDB.h \
  cpp-ethereum/libdevcore/StateCacheDB.cpp \
  cpp-ethereum/libdevcore/StateCacheDB.h \
  cpp-ethereum/libdevcore/RLP.cpp \
  cpp-ethereum/libdevcore/RLP.h \
  cpp-ethereum/libdevcore/SHA3.cpp \
  cpp-ethereum/libdevcore/SHA3.h \
  cpp-ethereum/libdevcore/TransientDirectory.cpp \
  cpp-ethereum/libdevcore/TransientDirectory.h \
  cpp-ethereum/libdevcore/TrieCommon.cpp \
  cpp-ethereum/libdevcore/TrieCommon.h \
  cpp-ethereum/libdevcore/Worker.cpp \
  cpp-ethereum/libdevcore/Worker.h \
  cpp-ethereum/libdevcore/DBFactory.h \
  cpp-ethereum/libdevcore/DBFactory.cpp \
  cpp-ethereum/libdevcore/JsonUtils.h \
  cpp-ethereum/libdevcore/JsonUtils.cpp \
  cpp-ethereum/libdevcore/dbfwd.h \
  cpp-ethereum/libevm/EVMC.cpp \
  cpp-ethereum/libevm/EVMC.h \
  cpp-ethereum/libevm/ExtVMFace.cpp \
  cpp-ethereum/libevm/ExtVMFace.h \
  cpp-ethereum/libaleth-interpreter/interpreter.h \
  cpp-ethereum/libaleth-interpreter/VMConfig.h \
  cpp-ethereum/libaleth-interpreter/VM.cpp \
  cpp-ethereum/libaleth-interpreter/VM.h \
  cpp-ethereum/libaleth-interpreter/VMOpt.cpp \
  cpp-ethereum/libaleth-interpreter/VMCalls.cpp \
  cpp-ethereum/libevm/VMFactory.cpp \
  cpp-ethereum/libevm/VMFactory.h \
  cpp-ethereum/libevm/Instruction.cpp \
  cpp-ethereum/libevm/Instruction.h \
  cpp-ethereum/libevm/LegacyVM.h \
  cpp-ethereum/libevm/LegacyVM.cpp \
  cpp-ethereum/libevm/LegacyVMCalls.cpp \
  cpp-ethereum/libevm/LegacyVMConfig.h \
  cpp-ethereum/libevm/LegacyVMOpt.cpp \
  cpp-ethereum/libethcore/EVMSchedule.h \
  cpp-ethereum/libethereum/ImportPerformanceLogger.cpp \
  cpp-ethereum/libethereum/ImportPerformanceLogger.h \
  cpp-ethereum/libethereum/Account.cpp \
  cpp-ethereum/libethereum/GasPricer.cpp \
  cpp-ethereum/libethereum/State.cpp \
  cpp-ethereum/libethcore/ABI.cpp \
  cpp-ethereum/libethcore/ChainOperationParams.cpp \
  cpp-ethereum/libethcore/Common.cpp \
  cpp-ethereum/libethcore/Precompiled.cpp \
  cpp-ethereum/libethcore/TransactionBase.cpp \
  cpp-ethereum/libdevcrypto/Common.cpp \
  cpp-ethereum/libdevcrypto/CryptoPP.cpp \
  cpp-ethereum/libdevcrypto/AES.cpp \
  cpp-ethereum/libdevcrypto/LibSnark.cpp \
  cpp-ethereum/libethashseal/GenesisInfo.cpp \
  cpp-ethereum/libethereum/ChainParams.cpp \
  cpp-ethereum/libethereum/Transaction.cpp \
  cpp-ethereum/libethereum/Executive.cpp \
  cpp-ethereum/libethereum/ExtVM.cpp \
  cpp-ethereum/libethereum/Block.cpp \
  cpp-ethereum/libethereum/BlockChain.cpp \
  cpp-ethereum/libethereum/BlockDetails.cpp \
  cpp-ethereum/libethereum/TransactionQueue.cpp \
  cpp-ethereum/libethereum/BlockQueue.cpp \
  cpp-ethereum/libethcore/BlockHeader.cpp \
  cpp-ethereum/libdevcore/RLP.cpp \
  cpp-ethereum/libethereum/TransactionReceipt.cpp \
  cpp-ethereum/libethcore/SealEngine.cpp \
  cpp-ethereum/libdevcore/TrieHash.cpp \
  cpp-ethereum/libethereum/GenesisInfo.cpp \
  cpp-ethereum/libethashseal/Ethash.cpp \
  cpp-ethereum/libethashseal/EthashCPUMiner.cpp \
  cpp-ethereum/libethashseal/EthashProofOfWork.cpp \
  cpp-ethereum/libethereum/ValidationSchemes.h \
  cpp-ethereum/libethereum/ValidationSchemes.cpp \
  cpp-ethereum/libethereum/Account.h \
  cpp-ethereum/libethereum/GasPricer.h \
  cpp-ethereum/libethereum/SecureTrieDB.h \
  cpp-ethereum/libethereum/State.h \
  cpp-ethereum/libethcore/ABI.h \
  cpp-ethereum/libethcore/ChainOperationParams.h \
  cpp-ethereum/libethcore/Common.h \
  cpp-ethereum/libethcore/Precompiled.h \
  cpp-ethereum/libethcore/LogEntry.h \
  cpp-ethereum/libethcore/LogEntry.cpp \
  cpp-ethereum/libdevcore/TrieDB.h \
  cpp-ethereum/libdevcrypto/Common.h \
  cpp-ethereum/libdevcrypto/CryptoPP.h \
  cpp-ethereum/libdevcrypto/AES.h \
  cpp-ethereum/libdevcrypto/LibSnark.h \
  cpp-ethereum/libethashseal/GenesisInfo.h \
  cpp-ethereum/libethereum/ChainParams.h \
  cpp-ethereum/libethcore/Exceptions.h \
  cpp-ethereum/libethcore/TransactionBase.h \
  cpp-ethereum/libethereum/Transaction.h \
  cpp-ethereum/libethereum/Executive.h \
  cpp-ethereum/libethereum/ExtVM.h \
  cpp-ethereum/libethereum/Block.h \
  cpp-ethereum/libethereum/BlockChain.h \
  cpp-ethereum/libethereum/BlockDetails.h \
  cpp-ethereum/libethereum/TransactionQueue.h \
  cpp-ethereum/libethereum/BlockQueue.h \
  cpp-ethereum/libethcore/BlockHeader.h \
  cpp-ethereum/libdevcore/RLP.h \
  cpp-ethereum/libethereum/TransactionReceipt.h \
  cpp-ethereum/libethcore/SealEngine.h \
  cpp-ethereum/libdevcore/TrieHash.h \
  cpp-ethereum/libethereum/GenesisInfo.h \
  cpp-ethereum/libethashseal/Ethash.h \
  cpp-ethereum/libethashseal/EthashCPUMiner.h \
  cpp-ethereum/libethashseal/EthashProofOfWork.h \
  cpp-ethereum/libdevcore/vector_ref.h \
  cpp-ethereum/libdevcore/Exceptions.h \
  cpp-ethereum/libdevcore/db.h \
  cpp-ethereum/libdevcore/concurrent_queue.h \
  cpp-ethereum/libdevcore/Terminal.h \
  cpp-ethereum/libethereum/CodeSizeCache.h \
  cpp-ethereum/libethereum/VerifiedBlock.h \
  cpp-ethereum/libdevcore/Assertions.h \
  cpp-ethereum/libethereum/GenericMiner.h \
  cpp-ethereum/libevm/VMFace.h \
  cpp-ethereum/libethereum/GenericFarm.h \
  cpp-ethereum/libethereum/LastBlockHashesFace.h \
  cpp-ethereum/libethereum/Interface.h \
  cpp-ethereum/libethereum/Client.h \
  cpp-ethereum/libethcore/CommonJS.h \
  cpp-ethereum/libdevcrypto/Exceptions.h \
  cpp-ethereum/libethashseal/genesis/mainNetwork.cpp \
  cpp-ethereum/libethashseal/genesis/qtumMainNetwork.cpp \
  cpp-ethereum/libethashseal/genesis/ropsten.cpp \
  cpp-ethereum/libethashseal/genesis/test/qtumTestNetwork.cpp \
  cpp-ethereum/libethashseal/genesis/test/transitionnetTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/mainNetworkTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/mainNetworkNoProofTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/homesteadToEIP150At5Test.cpp \
  cpp-ethereum/libethashseal/genesis/test/homesteadToDaoAt5Test.cpp \
  cpp-ethereum/libethashseal/genesis/test/homesteadTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/frontierToHomesteadAt5Test.cpp \
  cpp-ethereum/libethashseal/genesis/test/frontierTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/frontierNoProofTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/experimentalTransitionTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/EIP158ToByzantiumAt5Test.cpp \
  cpp-ethereum/libethashseal/genesis/test/eip158Test.cpp \
  cpp-ethereum/libethashseal/genesis/test/eip150Test.cpp \
  cpp-ethereum/libethashseal/genesis/test/constantinopleTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/constantinopleNoProofTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/byzantiumTransitionTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/ByzantiumToConstantinopleFixAt5Test.cpp \
  cpp-ethereum/libethashseal/genesis/test/byzantiumTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/byzantiumNoProofTest.cpp \
  cpp-ethereum/utils/json_spirit/JsonSpiritHeaders.h \
  cpp-ethereum/utils/json_spirit/json_spirit.h \
  cpp-ethereum/utils/json_spirit/json_spirit_error_position.h \
  cpp-ethereum/utils/json_spirit/json_spirit_reader.h \
  cpp-ethereum/utils/json_spirit/json_spirit_reader_template.h \
  cpp-ethereum/utils/json_spirit/json_spirit_stream_reader.h \
  cpp-ethereum/utils/json_spirit/json_spirit_utils.h \
  cpp-ethereum/utils/json_spirit/json_spirit_value.h \
  cpp-ethereum/utils/json_spirit/json_spirit_writer.h \
  cpp-ethereum/utils/json_spirit/json_spirit_writer_template.h \
  cpp-ethereum/evmc/include/evmc/evmc.h \
  cpp-ethereum/evmc/include/evmc/evmc.hpp \
  cpp-ethereum/evmc/include/evmc/helpers.h \
  cpp-ethereum/evmc/include/evmc/helpers.hpp \
  cpp-ethereum/evmc/include/evmc/instructions.h \
  cpp-ethereum/evmc/include/evmc/loader.h \
  cpp-ethereum/evmc/include/evmc/utils.h \
  cpp-ethereum/ethash/include/ethash/ethash.h \
  cpp-ethereum/ethash/include/ethash/ethash.hpp \
  cpp-ethereum/ethash/include/ethash/hash_types.h \
  cpp-ethereum/ethash/include/ethash/hash_types.hpp \
  cpp-ethereum/ethash/include/ethash/keccak.h \
  cpp-ethereum/ethash/include/ethash/keccak.hpp \
  cpp-ethereum/ethash/include/ethash/progpow.hpp \
  cpp-ethereum/ethash/lib/ethash/support/attributes.h \
  cpp-ethereum/ethash/lib/ethash/bit_manipulation.h \
  cpp-ethereum/ethash/lib/ethash/builtins.h \
  cpp-ethereum/ethash/lib/ethash/endianness.hpp \
  cpp-ethereum/ethash/lib/ethash/ethash.cpp \
  cpp-ethereum/ethash/lib/ethash/ethash-internal.hpp \
  cpp-ethereum/ethash/lib/ethash/keccak.c \
  cpp-ethereum/ethash/lib/ethash/keccakf800.c \
  cpp-ethereum/ethash/lib/ethash/keccakf1600.c \
  cpp-ethereum/ethash/lib/ethash/kiss99.hpp \
  cpp-ethereum/ethash/lib/ethash/managed.cpp \
  cpp-ethereum/ethash/lib/ethash/primes.c \
  cpp-ethereum/ethash/lib/ethash/primes.h \
  cpp-ethereum/ethash/lib/ethash/progpow.cpp \
  libff/libff/algebra/curves/public_params.hpp \
  libff/libff/algebra/curves/curve_utils.hpp \
  cpp-ethereum/libethereum/BlockChainImporter.h \
  cpp-ethereum/libethereum/EthereumCapability.h \
  cpp-ethereum/libethereum/ImportPerformanceLogger.h \
  cpp-ethereum/libethereum/SnapshotImporter.h \
  cpp-ethereum/libethereum/SnapshotStorage.h \
  cpp-ethereum/libethereum/StateImporter.h \
  cpp-ethereum/libethereum/ValidationSchemes.h \
  cpp-ethereum/libethereum/WarpCapability.h \
  cpp-ethereum/libethereum/ClientBase.h \
  cpp-ethereum/libethereum/ClientTest.h \
  cpp-ethereum/libethereum/LogFilter.h \
  cpp-ethereum/libethereum/CommonNet.h \
  cpp-ethereum/libp2p/All.h \
  cpp-ethereum/libp2p/Capability.h \
  cpp-ethereum/libp2p/CapabilityHost.h \
  cpp-ethereum/libp2p/Common.h \
  cpp-ethereum/libp2p/Host.h \
  cpp-ethereum/libp2p/Network.h \
  cpp-ethereum/libp2p/NodeTable.h \
  cpp-ethereum/libp2p/Peer.h \
  cpp-ethereum/libp2p/RLPXFrameCoder.h \
  cpp-ethereum/libp2p/RLPXPacket.h \
  cpp-ethereum/libp2p/RLPXSocket.h \
  cpp-ethereum/libp2p/RLPxHandshake.h \
  cpp-ethereum/libp2p/Session.h \
  cpp-ethereum/libp2p/UDP.h \
  cpp-ethereum/libp2p/UPnP.h \
  cpp-ethereum/libethashseal/genesis/test/constantinopleFixTest.cpp \
  $(BITCOIN_CORE_H)

#only added json_spirit so that `make dist` works properly

# util: shared between all executables.
# This library *must* be included to make sure that the glibc
# backward-compatibility objects and their sanity checks are linked.
libbitcoin_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libbitcoin_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_util_a_SOURCES = \
  support/lockedpool.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  compat/glibc_sanity.cpp \
  compat/glibcxx_sanity.cpp \
  compat/strnlen.cpp \
  fs.cpp \
  logging.cpp \
  random.cpp \
  rpc/protocol.cpp \
  support/cleanse.cpp \
  sync.cpp \
  threadinterrupt.cpp \
  util/bip32.cpp \
  util/bytevectorhash.cpp \
  util/system.cpp \
  util/moneystr.cpp \
  util/strencodings.cpp \
  util/time.cpp \
  $(BITCOIN_CORE_H)

if GLIBC_BACK_COMPAT
libbitcoin_util_a_SOURCES += compat/glibc_compat.cpp
AM_LDFLAGS += $(COMPAT_LDFLAGS)
endif

# cli: shared between bitcoin-cli and bitcoin-qt
libbitcoin_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libbitcoin_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_cli_a_SOURCES = \
  rpc/client.cpp \
  $(BITCOIN_CORE_H)

nodist_libbitcoin_util_a_SOURCES = $(srcdir)/obj/build.h
#

# bitcoind binary #
qtumd_SOURCES = bitcoind.cpp
qtumd_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
qtumd_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
qtumd_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
qtumd_SOURCES += bitcoind-res.rc
endif

qtumd_LDADD = \
  $(LIBBITCOIN_SERVER) \
  $(LIBBITCOIN_WALLET) \
  $(LIBBITCOIN_SERVER) \
  $(LIBBITCOIN_COMMON) \
  $(LIBUNIVALUE) \
  $(LIBBITCOIN_UTIL) \
  $(LIBBITCOIN_ZMQ) \
  $(LIBBITCOIN_CONSENSUS) \
  $(LIBBITCOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBCRYPTOPP) \
  $(LIBFF) \
  $(LIBSECP256K1)

qtumd_LDADD += $(BOOST_LIBS) $(BDB_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(MINIUPNPC_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ZMQ_LIBS) $(GMP_LIBS) $(GMPXX_LIBS)

# bitcoin-cli binary #
qtum_cli_SOURCES = bitcoin-cli.cpp
qtum_cli_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(EVENT_CFLAGS)
qtum_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
qtum_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
qtum_cli_SOURCES += bitcoin-cli-res.rc
endif

qtum_cli_LDADD = \
  $(LIBBITCOIN_CLI) \
  $(LIBUNIVALUE) \
  $(LIBBITCOIN_UTIL) \
  $(LIBBITCOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBCRYPTOPP) \
  $(LIBSECP256K1)

qtum_cli_LDADD += $(BOOST_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(EVENT_LIBS)
#

# bitcoin-tx binary #
qtum_tx_SOURCES = bitcoin-tx.cpp consensus/consensus.cpp
qtum_tx_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
qtum_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
qtum_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
qtum_tx_SOURCES += bitcoin-tx-res.rc
endif

qtum_tx_LDADD = \
  $(LIBUNIVALUE) \
  $(LIBBITCOIN_COMMON) \
  $(LIBBITCOIN_UTIL) \
  $(LIBBITCOIN_CONSENSUS) \
  $(LIBBITCOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBCRYPTOPP) \
  $(LIBSECP256K1)

qtum_tx_LDADD += $(BOOST_LIBS) $(CRYPTO_LIBS)
#

# bitcoin-wallet binary #
qtum_wallet_SOURCES = bitcoin-wallet.cpp
qtum_wallet_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
qtum_wallet_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
qtum_wallet_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
qtum_wallet_SOURCES += bitcoin-wallet-res.rc
endif

qtum_wallet_LDADD = \
  $(LIBBITCOIN_WALLET_TOOL) \
  $(LIBBITCOIN_WALLET) \
  $(LIBBITCOIN_SERVER) \
  $(LIBBITCOIN_COMMON) \
  $(LIBUNIVALUE) \
  $(LIBBITCOIN_CONSENSUS) \
  $(LIBBITCOIN_UTIL) \
  $(LIBBITCOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBCRYPTOPP) \
  $(LIBFF) \
  $(LIBSECP256K1)

qtum_wallet_LDADD += $(BOOST_LIBS) $(BDB_LIBS) $(CRYPTO_LIBS) $(MINIUPNPC_LIBS) $(GMP_LIBS) $(GMPXX_LIBS)
#

# bitcoinconsensus library #
if BUILD_BITCOIN_LIBS
include_HEADERS = script/bitcoinconsensus.h
libbitcoinconsensus_la_SOURCES = $(crypto_libbitcoin_crypto_base_a_SOURCES) $(libbitcoin_consensus_a_SOURCES)

if GLIBC_BACK_COMPAT
  libbitcoinconsensus_la_SOURCES += compat/glibc_compat.cpp
endif

libbitcoinconsensus_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS)
libbitcoinconsensus_la_LIBADD = $(LIBSECP256K1)
libbitcoinconsensus_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include $(BITCOIN_INCLUDES) -DBUILD_BITCOIN_INTERNAL
libbitcoinconsensus_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)

endif
#

CTAES_DIST =  crypto/ctaes/bench.c
CTAES_DIST += crypto/ctaes/ctaes.c
CTAES_DIST += crypto/ctaes/ctaes.h
CTAES_DIST += crypto/ctaes/README.md
CTAES_DIST += crypto/ctaes/test.c

CLEANFILES = $(EXTRA_LIBRARIES)

CLEANFILES += *.gcda *.gcno
CLEANFILES += compat/*.gcda compat/*.gcno
CLEANFILES += consensus/*.gcda consensus/*.gcno
CLEANFILES += crypto/*.gcda crypto/*.gcno
CLEANFILES += policy/*.gcda policy/*.gcno
CLEANFILES += primitives/*.gcda primitives/*.gcno
CLEANFILES += script/*.gcda script/*.gcno
CLEANFILES += support/*.gcda support/*.gcno
CLEANFILES += univalue/*.gcda univalue/*.gcno
CLEANFILES += wallet/*.gcda wallet/*.gcno
CLEANFILES += wallet/test/*.gcda wallet/test/*.gcno
CLEANFILES += zmq/*.gcda zmq/*.gcno
CLEANFILES += obj/build.h

EXTRA_DIST = $(CTAES_DIST)


config/bitcoin-config.h: config/stamp-h1
	@$(MAKE) -C $(top_builddir) $(subdir)/$(@)
config/stamp-h1: $(top_srcdir)/$(subdir)/config/bitcoin-config.h.in $(top_builddir)/config.status
	$(AM_V_at)$(MAKE) -C $(top_builddir) $(subdir)/$(@)
$(top_srcdir)/$(subdir)/config/bitcoin-config.h.in:  $(am__configure_deps)
	$(AM_V_at)$(MAKE) -C $(top_srcdir) $(subdir)/config/bitcoin-config.h.in

clean-local:
	-$(MAKE) -C secp256k1 clean
	-$(MAKE) -C univalue clean
	-rm -f leveldb/*/*.gcda leveldb/*/*.gcno leveldb/helpers/memenv/*.gcda leveldb/helpers/memenv/*.gcno
	-rm -f cryptopp/*.o libff/*.o
	-rm -f config.h
	-rm -rf test/__pycache__

.rc.o:
	@test -f $(WINDRES)
	## FIXME: How to get the appropriate modulename_CPPFLAGS in here?
	$(AM_V_GEN) $(WINDRES) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) -DWINDRES_PREPROC -i $< -o $@

check-symbols: $(bin_PROGRAMS)
if GLIBC_BACK_COMPAT
	@echo "Checking glibc back compat..."
	$(AM_V_at) READELF=$(READELF) CPPFILT=$(CPPFILT) $(top_srcdir)/contrib/devtools/symbol-check.py < $(bin_PROGRAMS)
endif

check-security: $(bin_PROGRAMS)
if HARDEN
	@echo "Checking binary security..."
	$(AM_V_at) READELF=$(READELF) OBJDUMP=$(OBJDUMP) $(top_srcdir)/contrib/devtools/security-check.py < $(bin_PROGRAMS)
endif

if ENABLE_BIP70
%.pb.cc %.pb.h: %.proto
	@test -f $(PROTOC)
	$(AM_V_GEN) $(PROTOC) --cpp_out=$(@D) --proto_path=$(<D) $<
endif

if EMBEDDED_LEVELDB
include Makefile.leveldb.include
endif

include Makefile.cryptopp.include

include Makefile.libff.include

if ENABLE_TESTS
include Makefile.test.include
endif

if ENABLE_BENCH
include Makefile.bench.include
endif

if ENABLE_QT
include Makefile.qt.include
endif

if ENABLE_QT_TESTS
include Makefile.qttest.include
endif
