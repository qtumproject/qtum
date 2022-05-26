# Copyright (c) 2013-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Pattern rule to print variables, e.g. make print-top_srcdir
print-%: FORCE
	@echo '$*'='$($*)'

DIST_SUBDIRS = secp256k1 univalue

AM_LDFLAGS = $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS) $(GPROF_LDFLAGS) $(SANITIZER_LDFLAGS)
AM_CXXFLAGS = $(DEBUG_CXXFLAGS) $(HARDENED_CXXFLAGS) $(WARN_CXXFLAGS) $(NOWARN_CXXFLAGS) $(ERROR_CXXFLAGS) $(GPROF_CXXFLAGS) $(SANITIZER_CXXFLAGS) -DETH_FATDB=1
AM_CPPFLAGS = $(DEBUG_CPPFLAGS) $(HARDENED_CPPFLAGS) -DQTUM_BUILD
AM_LIBTOOLFLAGS = --preserve-dup-deps
PTHREAD_FLAGS = $(PTHREAD_CFLAGS) $(PTHREAD_LIBS)
EXTRA_LIBRARIES =

if EMBEDDED_UNIVALUE
LIBUNIVALUE = univalue/libunivalue.la

$(LIBUNIVALUE): $(wildcard univalue/lib/*) $(wildcard univalue/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)
else
LIBUNIVALUE = $(UNIVALUE_LIBS)
endif

BITCOIN_INCLUDES=-I$(builddir) -I$(srcdir)/secp256k1/include -I$(srcdir)/eth_client/utils $(BDB_CPPFLAGS) $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS) $(SSL_CFLAGS)

BITCOIN_INCLUDES += -I$(srcdir)/libff/libff
BITCOIN_INCLUDES += -I$(srcdir)/libff
BITCOIN_INCLUDES += $(UNIVALUE_CFLAGS)

BITCOIN_INCLUDES += -I$(srcdir)/evmone/evmc/include
BITCOIN_INCLUDES += -I$(srcdir)/evmone/include
BITCOIN_INCLUDES += -I$(srcdir)/eth_client
BITCOIN_INCLUDES += -I$(srcdir)/eth_client/utils/ethash/include
BITCOIN_INCLUDES += -I$(srcdir)/eth_client/utils/libscrypt

LIBBITCOIN_SERVER=libbitcoin_server.a
LIBBITCOIN_COMMON=libbitcoin_common.a
LIBBITCOIN_CONSENSUS=libbitcoin_consensus.a
LIBBITCOIN_CLI=libbitcoin_cli.a
LIBBITCOIN_UTIL=libqtum_util.a
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
  $(LIBBITCOIN_IPC) \
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

if BUILD_BITCOIN_NODE
  bin_PROGRAMS += qtum-node
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

if BUILD_BITCOIN_UTIL
  bin_PROGRAMS += qtum-util
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
  blockencodings.h \
  blockfilter.h \
  bloom.h \
  chain.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  checkqueue.h \
  clientversion.h \
  coins.h \
  compat.h \
  compat/assumptions.h \
  compat/byteswap.h \
  compat/cpuid.h \
  compat/endian.h \
  compat/sanity.h \
  compressor.h \
  consensus/consensus.h \
  consensus/tx_check.h \
  consensus/tx_verify.h \
  core_io.h \
  core_memusage.h \
  cuckoocache.h \
  dbwrapper.h \
  deploymentinfo.h \
  deploymentstatus.h \
  external_signer.h \
  flatfile.h \
  fs.h \
  httprpc.h \
  httpserver.h \
  i2p.h \
  index/base.h \
  index/blockfilterindex.h \
  index/coinstatsindex.h \
  index/disktxpos.h \
  index/txindex.h \
  indirectmap.h \
  init.h \
  init/common.h \
  interfaces/chain.h \
  interfaces/echo.h \
  interfaces/handler.h \
  interfaces/init.h \
  interfaces/ipc.h \
  interfaces/node.h \
  interfaces/wallet.h \
  key.h \
  key_io.h \
  logging.h \
  logging/timer.h \
  mapport.h \
  memusage.h \
  merkleblock.h \
  miner.h \
  net.h \
  net_permissions.h \
  net_processing.h \
  net_types.h \
  netaddress.h \
  netbase.h \
  netmessagemaker.h \
  node/blockstorage.h \
  node/coin.h \
  node/coinstats.h \
  node/context.h \
  node/psbt.h \
  node/transaction.h \
  node/ui_interface.h \
  node/utxo_snapshot.h \
  noui.h \
  outputtype.h \
  policy/feerate.h \
  policy/fees.h \
  policy/packages.h \
  policy/policy.h \
  policy/rbf.h \
  policy/settings.h \
  pow.h \
  pos.h \
  protocol.h \
  psbt.h \
  random.h \
  randomenv.h \
  reverse_iterator.h \
  rpc/blockchain.h \
  rpc/client.h \
  rpc/mining.h \
  rpc/net.h \
  rpc/protocol.h \
  rpc/rawtransaction_util.h \
  rpc/register.h \
  rpc/request.h \
  rpc/server.h \
  rpc/util.h \
  rpc/contract_util.h \
  scheduler.h \
  script/descriptor.h \
  script/keyorigin.h \
  script/sigcache.h \
  script/sign.h \
  script/signingprovider.h \
  script/standard.h \
  shutdown.h \
  signet.h \
  streams.h \
  support/allocators/secure.h \
  support/allocators/zeroafterfree.h \
  support/cleanse.h \
  support/events.h \
  support/lockedpool.h \
  sync.h \
  threadinterrupt.h \
  threadsafety.h \
  timedata.h \
  torcontrol.h \
  txdb.h \
  txmempool.h \
  txorphanage.h \
  txrequest.h \
  undo.h \
  util/asmap.h \
  util/bip32.h \
  util/bytevectorhash.h \
  util/check.h \
  util/epochguard.h \
  util/error.h \
  util/fees.h \
  util/getuniquepath.h \
  util/golombrice.h \
  util/hash_type.h \
  util/hasher.h \
  util/macros.h \
  util/message.h \
  util/moneystr.h \
  util/rbf.h \
  util/readwritefile.h \
  util/serfloat.h \
  util/settings.h \
  util/sock.h \
  util/spanparsing.h \
  util/string.h \
  util/system.h \
  util/thread.h \
  util/threadnames.h \
  util/time.h \
  util/tokenpipe.h \
  util/trace.h \
  util/translation.h \
  util/ui_change_type.h \
  util/url.h \
  util/vector.h \
  util/convert.h \
  util/signstr.h \
  util/contractabi.h \
  util/tokenstr.h \
  validation.h \
  validationinterface.h \
  versionbits.h \
  wallet/bdb.h \
  wallet/coincontrol.h \
  wallet/coinselection.h \
  wallet/context.h \
  wallet/crypter.h \
  wallet/db.h \
  wallet/dump.h \
  wallet/external_signer_scriptpubkeyman.h \
  wallet/feebumper.h \
  wallet/fees.h \
  wallet/ismine.h \
  wallet/load.h \
  wallet/receive.h \
  wallet/rpcwallet.h \
  wallet/salvage.h \
  wallet/scriptpubkeyman.h \
  wallet/spend.h \
  wallet/sqlite.h \
  wallet/transaction.h \
  wallet/wallet.h \
  wallet/walletdb.h \
  wallet/wallettool.h \
  wallet/walletutil.h \
  walletinitinterface.h \
  warnings.h \
  zmq/zmqabstractnotifier.h \
  zmq/zmqnotificationinterface.h \
  zmq/zmqpublishnotifier.h \
  zmq/zmqrpc.h \
  zmq/zmqutil.h \
  qtum/qtumstate.h \
  qtum/qtumtransaction.h \
  qtum/qtumDGP.h \
  qtum/storageresults.h \
  qtum/qtumutils.h \
  qtum/qtumdelegation.h \
  qtum/qtumtoken.h \
  qtum/qtumledger.h

obj/build.h: FORCE
	@$(MKDIR_P) $(builddir)/obj
	@$(top_srcdir)/share/genbuild.sh "$(abs_top_builddir)/src/obj/build.h" \
	  "$(abs_top_srcdir)"
libqtum_util_a-clientversion.$(OBJEXT): obj/build.h

ipc/capnp/libbitcoin_ipc_a-ipc.$(OBJEXT): $(libbitcoin_ipc_mpgen_input:=.h)

# server: shared between bitcoind and bitcoin-qt
# Contains code accessing mempool and chain state that is meant to be separated
# from wallet and gui code (see node/README.md). Shared code should go in
# libbitcoin_common or libqtum_util libraries, instead.
libbitcoin_server_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(MINIUPNPC_CPPFLAGS) $(NATPMP_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libbitcoin_server_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_server_a_SOURCES = \
  addrdb.cpp \
  addrman.cpp \
  banman.cpp \
  blockencodings.cpp \
  blockfilter.cpp \
  chain.cpp \
  consensus/tx_verify.cpp \
  dbwrapper.cpp \
  deploymentstatus.cpp \
  flatfile.cpp \
  httprpc.cpp \
  httpserver.cpp \
  i2p.cpp \
  index/base.cpp \
  index/blockfilterindex.cpp \
  index/coinstatsindex.cpp \
  index/txindex.cpp \
  init.cpp \
  mapport.cpp \
  miner.cpp \
  net.cpp \
  net_processing.cpp \
  node/blockstorage.cpp \
  node/coin.cpp \
  node/coinstats.cpp \
  node/context.cpp \
  node/interfaces.cpp \
  node/psbt.cpp \
  node/transaction.cpp \
  node/ui_interface.cpp \
  noui.cpp \
  policy/fees.cpp \
  policy/packages.cpp \
  policy/rbf.cpp \
  policy/settings.cpp \
  pow.cpp \
  rest.cpp \
  rpc/blockchain.cpp \
  rpc/mining.cpp \
  rpc/misc.cpp \
  rpc/net.cpp \
  rpc/rawtransaction.cpp \
  rpc/server.cpp \
  script/sigcache.cpp \
  shutdown.cpp \
  signet.cpp \
  timedata.cpp \
  torcontrol.cpp \
  txdb.cpp \
  txmempool.cpp \
  txorphanage.cpp \
  txrequest.cpp \
  validation.cpp \
  validationinterface.cpp \
  versionbits.cpp \
  qtum/qtumstate.cpp \
  qtum/storageresults.cpp \
  qtum/qtumledger.cpp \
  $(BITCOIN_CORE_H)

if ENABLE_WALLET
libbitcoin_server_a_SOURCES += wallet/init.cpp
endif
if !ENABLE_WALLET
libbitcoin_server_a_SOURCES += dummywallet.cpp
endif

if ENABLE_ZMQ
libbitcoin_zmq_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(ZMQ_CFLAGS)
libbitcoin_zmq_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_zmq_a_SOURCES = \
  zmq/zmqabstractnotifier.cpp \
  zmq/zmqnotificationinterface.cpp \
  zmq/zmqpublishnotifier.cpp \
  zmq/zmqrpc.cpp \
  zmq/zmqutil.cpp
endif


# wallet: shared between bitcoind and bitcoin-qt, but only linked
# when wallet enabled
libbitcoin_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(SQLITE_CFLAGS)
libbitcoin_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_wallet_a_SOURCES = \
  wallet/coincontrol.cpp \
  wallet/context.cpp \
  wallet/crypter.cpp \
  wallet/db.cpp \
  wallet/dump.cpp \
  wallet/external_signer_scriptpubkeyman.cpp \
  wallet/feebumper.cpp \
  wallet/fees.cpp \
  wallet/interfaces.cpp \
  wallet/load.cpp \
  wallet/receive.cpp \
  wallet/rpcdump.cpp \
  wallet/rpcwallet.cpp \
  wallet/scriptpubkeyman.cpp \
  wallet/spend.cpp \
  wallet/transaction.cpp \
  wallet/wallet.cpp \
  wallet/walletdb.cpp \
  wallet/walletutil.cpp \
  wallet/coinselection.cpp \
  $(BITCOIN_CORE_H)

if USE_SQLITE
libbitcoin_wallet_a_SOURCES += wallet/sqlite.cpp
endif
if USE_BDB
libbitcoin_wallet_a_SOURCES += wallet/bdb.cpp wallet/salvage.cpp
endif

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
  crypto/chacha_poly_aead.h \
  crypto/chacha_poly_aead.cpp \
  crypto/chacha20.h \
  crypto/chacha20.cpp \
  crypto/common.h \
  crypto/hkdf_sha256_32.cpp \
  crypto/hkdf_sha256_32.h \
  crypto/hmac_sha256.cpp \
  crypto/hmac_sha256.h \
  crypto/hmac_sha512.cpp \
  crypto/hmac_sha512.h \
  crypto/poly1305.h \
  crypto/poly1305.cpp \
  crypto/muhash.h \
  crypto/muhash.cpp \
  crypto/ripemd160.cpp \
  crypto/ripemd160.h \
  crypto/sha1.cpp \
  crypto/sha1.h \
  crypto/sha256.cpp \
  crypto/sha256.h \
  crypto/sha3.cpp \
  crypto/sha3.h \
  crypto/sha512.cpp \
  crypto/sha512.h \
  crypto/siphash.cpp \
  crypto/siphash.h \
  eth_client/utils/libscrypt/b64.c \
  eth_client/utils/libscrypt/b64.h \
  eth_client/utils/libscrypt/crypto-mcf.c \
  eth_client/utils/libscrypt/crypto-scrypt-saltgen.c \
  eth_client/utils/libscrypt/crypto_scrypt-check.c \
  eth_client/utils/libscrypt/crypto_scrypt-hash.c \
  eth_client/utils/libscrypt/crypto_scrypt-hexconvert.c \
  eth_client/utils/libscrypt/crypto_scrypt-hexconvert.h \
  eth_client/utils/libscrypt/crypto_scrypt-nosse.c \
  eth_client/utils/libscrypt/libscrypt.h \
  eth_client/utils/libscrypt/sha256.c \
  eth_client/utils/libscrypt/sha256.h \
  eth_client/utils/libscrypt/slowequals.c \
  eth_client/utils/libscrypt/slowequals.h \
  eth_client/utils/libscrypt/sysendian.h

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
  consensus/consensus.cpp \
  consensus/merkle.cpp \
  consensus/merkle.h \
  consensus/params.h \
  consensus/tx_check.cpp \
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
  bloom.cpp \
  chainparams.cpp \
  coins.cpp \
  compressor.cpp \
  core_read.cpp \
  core_write.cpp \
  deploymentinfo.cpp \
  external_signer.cpp \
  init/common.cpp \
  key.cpp \
  key_io.cpp \
  merkleblock.cpp \
  netaddress.cpp \
  netbase.cpp \
  net_permissions.cpp \
  outputtype.cpp \
  policy/feerate.cpp \
  policy/policy.cpp \
  protocol.cpp \
  psbt.cpp \
  pos.cpp \
  rpc/rawtransaction_util.cpp \
  rpc/external_signer.cpp \
  rpc/util.cpp \
  rpc/contract_util.cpp \
  scheduler.cpp \
  script/descriptor.cpp \
  script/sign.cpp \
  script/signingprovider.cpp \
  script/standard.cpp \
  warnings.cpp \
  qtum/qtumutils.cpp \
  qtum/qtumDGP.cpp \
  qtum/qtumtoken.cpp \
  qtum/qtumdelegation.cpp \
  util/contractabi.cpp \
  libff/libff/algebra/curves/public_params.hpp \
  libff/libff/algebra/curves/curve_utils.hpp \
  evmone/include/evmone/evmone.h \
  evmone/evmc/lib/instructions/instruction_metrics.c \
  evmone/evmc/lib/instructions/instruction_names.c \
  evmone/evmc/include/evmc/evmc.h \
  evmone/evmc/include/evmc/evmc.hpp \
  evmone/evmc/include/evmc/helpers.h \
  evmone/evmc/include/evmc/instructions.h \
  evmone/evmc/include/evmc/loader.h \
  evmone/evmc/include/evmc/utils.h \
  evmone/lib/evmone/analysis.cpp \
  evmone/lib/evmone/analysis.hpp \
  evmone/lib/evmone/baseline.cpp \
  evmone/lib/evmone/baseline.hpp \
  evmone/lib/evmone/baseline_instruction_table.cpp \
  evmone/lib/evmone/baseline_instruction_table.hpp \
  evmone/lib/evmone/execution.cpp \
  evmone/lib/evmone/execution.hpp \
  evmone/lib/evmone/execution_state.hpp \
  evmone/lib/evmone/instructions_calls.cpp \
  evmone/lib/evmone/instructions.cpp \
  evmone/lib/evmone/instructions.hpp \
  evmone/lib/evmone/instruction_traits.hpp \
  evmone/lib/evmone/limits.hpp \
  evmone/lib/evmone/opcodes_helpers.h \
  evmone/lib/evmone/tracing.cpp \
  evmone/lib/evmone/tracing.hpp \
  evmone/lib/evmone/vm.cpp \
  evmone/lib/evmone/vm.hpp \
  eth_client/libdevcore/Address.cpp \
  eth_client/libdevcore/Address.h \
  eth_client/libdevcore/Assertions.h \
  eth_client/libdevcore/Common.cpp \
  eth_client/libdevcore/Common.h \
  eth_client/libdevcore/CommonData.cpp \
  eth_client/libdevcore/CommonData.h \
  eth_client/libdevcore/CommonIO.cpp \
  eth_client/libdevcore/CommonIO.h \
  eth_client/libdevcore/DBFactory.cpp \
  eth_client/libdevcore/DBFactory.h \
  eth_client/libdevcore/Exceptions.h \
  eth_client/libdevcore/FileSystem.cpp \
  eth_client/libdevcore/FileSystem.h \
  eth_client/libdevcore/FixedHash.cpp \
  eth_client/libdevcore/FixedHash.h \
  eth_client/libdevcore/Guards.h \
  eth_client/libdevcore/JsonUtils.cpp \
  eth_client/libdevcore/JsonUtils.h \
  eth_client/libdevcore/LevelDB.cpp \
  eth_client/libdevcore/LevelDB.h \
  eth_client/libdevcore/Log.cpp \
  eth_client/libdevcore/Log.h \
  eth_client/libdevcore/OverlayDB.cpp \
  eth_client/libdevcore/OverlayDB.h \
  eth_client/libdevcore/RLP.cpp \
  eth_client/libdevcore/RLP.h \
  eth_client/libdevcore/SHA3.cpp \
  eth_client/libdevcore/SHA3.h \
  eth_client/libdevcore/StateCacheDB.cpp \
  eth_client/libdevcore/StateCacheDB.h \
  eth_client/libdevcore/TrieCommon.cpp \
  eth_client/libdevcore/TrieCommon.h \
  eth_client/libdevcore/TrieDB.h \
  eth_client/libdevcore/TrieHash.cpp \
  eth_client/libdevcore/TrieHash.h \
  eth_client/libdevcore/UndefMacros.h \
  eth_client/libdevcore/db.h \
  eth_client/libdevcore/dbfwd.h \
  eth_client/libdevcore/vector_ref.h \
  eth_client/libdevcrypto/Blake2.cpp \
  eth_client/libdevcrypto/Blake2.h \
  eth_client/libdevcrypto/Common.cpp \
  eth_client/libdevcrypto/Common.h \
  eth_client/libdevcrypto/CryptoPP.cpp \
  eth_client/libdevcrypto/CryptoPP.h \
  eth_client/libdevcrypto/Exceptions.h \
  eth_client/libdevcrypto/Hash.cpp \
  eth_client/libdevcrypto/Hash.h \
  eth_client/libdevcrypto/LibSnark.cpp \
  eth_client/libdevcrypto/LibSnark.h \
  eth_client/libethashseal/GenesisInfo.cpp \
  eth_client/libethashseal/GenesisInfo.h \
  eth_client/libethashseal/genesis/qtumNetwork.cpp \
  eth_client/libethcore/ABI.h \
  eth_client/libethcore/BlockHeader.cpp \
  eth_client/libethcore/BlockHeader.h \
  eth_client/libethcore/ChainOperationParams.cpp \
  eth_client/libethcore/ChainOperationParams.h \
  eth_client/libethcore/Common.cpp \
  eth_client/libethcore/Common.h \
  eth_client/libethcore/EVMSchedule.cpp \
  eth_client/libethcore/EVMSchedule.h \
  eth_client/libethcore/Exceptions.h \
  eth_client/libethcore/LogEntry.cpp \
  eth_client/libethcore/LogEntry.h \
  eth_client/libethcore/Precompiled.cpp \
  eth_client/libethcore/Precompiled.h \
  eth_client/libethcore/SealEngine.cpp \
  eth_client/libethcore/SealEngine.h \
  eth_client/libethcore/TransactionBase.cpp \
  eth_client/libethcore/TransactionBase.h \
  eth_client/libethereum/Account.cpp \
  eth_client/libethereum/Account.h \
  eth_client/libethereum/ChainParams.cpp \
  eth_client/libethereum/ChainParams.h \
  eth_client/libethereum/CodeSizeCache.h \
  eth_client/libethereum/DatabasePaths.cpp \
  eth_client/libethereum/DatabasePaths.h \
  eth_client/libethereum/Executive.cpp \
  eth_client/libethereum/Executive.h \
  eth_client/libethereum/ExtVM.cpp \
  eth_client/libethereum/ExtVM.h \
  eth_client/libethereum/LastBlockHashesFace.h \
  eth_client/libethereum/SecureTrieDB.h \
  eth_client/libethereum/State.cpp \
  eth_client/libethereum/State.h \
  eth_client/libethereum/Transaction.cpp \
  eth_client/libethereum/Transaction.h \
  eth_client/libethereum/TransactionReceipt.cpp \
  eth_client/libethereum/TransactionReceipt.h \
  eth_client/libethereum/ValidationSchemes.cpp \
  eth_client/libethereum/ValidationSchemes.h \
  eth_client/libevm/EVMC.cpp \
  eth_client/libevm/EVMC.h \
  eth_client/libevm/ExtVMFace.cpp \
  eth_client/libevm/ExtVMFace.h \
  eth_client/libevm/VMFace.h \
  eth_client/libevm/VMFactory.cpp \
  eth_client/libevm/VMFactory.h \
  eth_client/utils/ethash/include/ethash/ethash.h \
  eth_client/utils/ethash/include/ethash/ethash.hpp \
  eth_client/utils/ethash/include/ethash/hash_types.h \
  eth_client/utils/ethash/include/ethash/hash_types.hpp \
  eth_client/utils/ethash/include/ethash/keccak.h \
  eth_client/utils/ethash/include/ethash/keccak.hpp \
  eth_client/utils/ethash/include/ethash/progpow.hpp \
  eth_client/utils/ethash/lib/ethash/bit_manipulation.h \
  eth_client/utils/ethash/lib/ethash/builtins.h \
  eth_client/utils/ethash/lib/ethash/endianness.hpp \
  eth_client/utils/ethash/lib/ethash/ethash-internal.hpp \
  eth_client/utils/ethash/lib/ethash/ethash.cpp \
  eth_client/utils/ethash/lib/ethash/keccak.c \
  eth_client/utils/ethash/lib/ethash/keccakf1600.c \
  eth_client/utils/ethash/lib/ethash/keccakf800.c \
  eth_client/utils/ethash/lib/ethash/kiss99.hpp \
  eth_client/utils/ethash/lib/ethash/managed.cpp \
  eth_client/utils/ethash/lib/ethash/primes.c \
  eth_client/utils/ethash/lib/ethash/primes.h \
  eth_client/utils/ethash/lib/ethash/progpow.cpp \
  eth_client/utils/ethash/lib/ethash/support/attributes.h \
  eth_client/utils/intx/int128.hpp \
  eth_client/utils/intx/intx.hpp \
  eth_client/utils/json_spirit/JsonSpiritHeaders.h \
  eth_client/utils/json_spirit/json_spirit.h \
  eth_client/utils/json_spirit/json_spirit.vcproj \
  eth_client/utils/json_spirit/json_spirit_error_position.h \
  eth_client/utils/json_spirit/json_spirit_reader.cpp \
  eth_client/utils/json_spirit/json_spirit_reader.h \
  eth_client/utils/json_spirit/json_spirit_reader_template.h \
  eth_client/utils/json_spirit/json_spirit_stream_reader.h \
  eth_client/utils/json_spirit/json_spirit_utils.h \
  eth_client/utils/json_spirit/json_spirit_value.cpp \
  eth_client/utils/json_spirit/json_spirit_value.h \
  eth_client/utils/json_spirit/json_spirit_writer.cpp \
  eth_client/utils/json_spirit/json_spirit_writer.h \
  eth_client/utils/json_spirit/json_spirit_writer_template.h \
  $(BITCOIN_CORE_H)

#only added json_spirit so that `make dist` works properly

# util: shared between all executables.
# This library *must* be included to make sure that the glibc
# backward-compatibility objects and their sanity checks are linked.
libqtum_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libqtum_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libqtum_util_a_SOURCES = \
  support/lockedpool.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  compat/glibcxx_sanity.cpp \
  compat/strnlen.cpp \
  fs.cpp \
  interfaces/echo.cpp \
  interfaces/handler.cpp \
  interfaces/init.cpp \
  logging.cpp \
  random.cpp \
  randomenv.cpp \
  rpc/request.cpp \
  support/cleanse.cpp \
  sync.cpp \
  threadinterrupt.cpp \
  util/asmap.cpp \
  util/bip32.cpp \
  util/bytevectorhash.cpp \
  util/error.cpp \
  util/fees.cpp \
  util/getuniquepath.cpp \
  util/hasher.cpp \
  util/sock.cpp \
  util/system.cpp \
  util/message.cpp \
  util/moneystr.cpp \
  util/rbf.cpp \
  util/readwritefile.cpp \
  util/settings.cpp \
  util/thread.cpp \
  util/threadnames.cpp \
  util/serfloat.cpp \
  util/spanparsing.cpp \
  util/strencodings.cpp \
  util/string.cpp \
  util/time.cpp \
  util/tokenpipe.cpp \
  util/tokenstr.cpp \
  $(BITCOIN_CORE_H)

if USE_LIBEVENT
libqtum_util_a_SOURCES += util/url.cpp
endif

if GLIBC_BACK_COMPAT
libqtum_util_a_SOURCES += compat/glibc_compat.cpp
AM_LDFLAGS += $(COMPAT_LDFLAGS)
endif

# cli: shared between bitcoin-cli and bitcoin-qt
libbitcoin_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libbitcoin_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_cli_a_SOURCES = \
  compat/stdin.h \
  compat/stdin.cpp \
  rpc/client.cpp \
  $(BITCOIN_CORE_H)

nodist_libqtum_util_a_SOURCES = $(srcdir)/obj/build.h
#

# bitcoind & bitcoin-node binaries #
qtum_daemon_sources = bitcoind.cpp
qtum_bin_cppflags = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
qtum_bin_cxxflags = $(AM_CXXFLAGS) $(PIE_FLAGS)
qtum_bin_ldflags = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
qtum_daemon_sources += bitcoind-res.rc
endif

qtum_bin_ldadd = \
  $(LIBBITCOIN_SERVER) \
  $(LIBBITCOIN_WALLET) \
  $(LIBBITCOIN_SERVER) \
  $(LIBBITCOIN_COMMON) \
  $(LIBBITCOIN_UTIL) \
  $(LIBUNIVALUE) \
  $(LIBBITCOIN_ZMQ) \
  $(LIBBITCOIN_CONSENSUS) \
  $(LIBBITCOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBCRYPTOPP) \
  $(LIBFF) \
  $(LIBSECP256K1)

qtum_bin_ldadd += $(BOOST_LIBS) $(BDB_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(MINIUPNPC_LIBS) $(NATPMP_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ZMQ_LIBS) $(SQLITE_LIBS) $(GMP_LIBS) $(GMPXX_LIBS)

qtumd_SOURCES = $(qtum_daemon_sources) init/bitcoind.cpp
qtumd_CPPFLAGS = $(qtum_bin_cppflags)
qtumd_CXXFLAGS = $(qtum_bin_cxxflags)
qtumd_LDFLAGS = $(qtum_bin_ldflags)
qtumd_LDADD = $(LIBBITCOIN_SERVER) $(qtum_bin_ldadd)

qtum_node_SOURCES = $(qtum_daemon_sources) init/bitcoin-node.cpp
qtum_node_CPPFLAGS = $(qtum_bin_cppflags)
qtum_node_CXXFLAGS = $(qtum_bin_cxxflags)
qtum_node_LDFLAGS = $(qtum_bin_ldflags)
qtum_node_LDADD = $(LIBBITCOIN_SERVER) $(qtum_bin_ldadd) $(LIBBITCOIN_IPC) $(LIBMULTIPROCESS_LIBS)

# bitcoin-cli binary #
qtum_cli_SOURCES = bitcoin-cli.cpp
qtum_cli_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(EVENT_CFLAGS)
qtum_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
qtum_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

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
qtum_tx_SOURCES = bitcoin-tx.cpp
qtum_tx_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
qtum_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
qtum_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

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
qtum_wallet_SOURCES = bitcoin-wallet.cpp validation.cpp rpc/server.cpp
qtum_wallet_CPPFLAGS = $(qtum_bin_cppflags)
qtum_wallet_CXXFLAGS = $(qtum_bin_cxxflags)
qtum_wallet_LDFLAGS = $(qtum_bin_ldflags)
qtum_wallet_LDADD = $(LIBBITCOIN_WALLET_TOOL) $(qtum_bin_ldadd)

if TARGET_WINDOWS
qtum_wallet_SOURCES += bitcoin-wallet-res.rc
endif
#

# bitcoin-util binary #
qtum_util_SOURCES = bitcoin-util.cpp
qtum_util_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
qtum_util_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
qtum_util_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
qtum_util_SOURCES += bitcoin-util-res.rc
endif

qtum_util_LDADD = \
  $(LIBBITCOIN_COMMON) \
  $(LIBBITCOIN_UTIL) \
  $(LIBUNIVALUE) \
  $(LIBBITCOIN_CONSENSUS) \
  $(LIBBITCOIN_CRYPTO) \
  $(LIBSECP256K1)

qtum_util_LDADD += $(BOOST_LIBS)
#

# bitcoinconsensus library #
if BUILD_BITCOIN_LIBS
include_HEADERS = script/bitcoinconsensus.h
libbitcoinconsensus_la_SOURCES = support/cleanse.cpp $(crypto_libbitcoin_crypto_base_a_SOURCES) $(libbitcoin_consensus_a_SOURCES)

if GLIBC_BACK_COMPAT
  libbitcoinconsensus_la_SOURCES += compat/glibc_compat.cpp
endif

libbitcoinconsensus_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS)
libbitcoinconsensus_la_LIBADD = $(LIBSECP256K1)
libbitcoinconsensus_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include -DBUILD_BITCOIN_INTERNAL
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
CLEANFILES += crc32c/src/*.gcda crc32c/src/*.gcno
CLEANFILES += crypto/*.gcda crypto/*.gcno
CLEANFILES += index/*.gcda index/*.gcno
CLEANFILES += interfaces/*.gcda interfaces/*.gcno
CLEANFILES += node/*.gcda node/*.gcno
CLEANFILES += policy/*.gcda policy/*.gcno
CLEANFILES += primitives/*.gcda primitives/*.gcno
CLEANFILES += rpc/*.gcda rpc/*.gcno
CLEANFILES += script/*.gcda script/*.gcno
CLEANFILES += support/*.gcda support/*.gcno
CLEANFILES += univalue/*.gcda univalue/*.gcno
CLEANFILES += util/*.gcda util/*.gcno
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
if TARGET_DARWIN
	@echo "Checking macOS dynamic libraries..."
	$(AM_V_at) $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py $(bin_PROGRAMS)
endif

if TARGET_WINDOWS
	@echo "Checking Windows dynamic libraries..."
	$(AM_V_at) $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py $(bin_PROGRAMS)
endif

if TARGET_LINUX
	@echo "Checking glibc back compat..."
	$(AM_V_at) CPPFILT='$(CPPFILT)' $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py $(bin_PROGRAMS)
endif

check-security: $(bin_PROGRAMS)
if HARDEN
	@echo "Checking binary security..."
	$(AM_V_at) $(PYTHON) $(top_srcdir)/contrib/devtools/security-check.py $(bin_PROGRAMS)
endif

libbitcoin_ipc_mpgen_input = \
  ipc/capnp/echo.capnp \
  ipc/capnp/init.capnp
EXTRA_DIST += $(libbitcoin_ipc_mpgen_input)
%.capnp:

if BUILD_MULTIPROCESS
LIBBITCOIN_IPC=libbitcoin_ipc.a
libbitcoin_ipc_a_SOURCES = \
  ipc/capnp/init-types.h \
  ipc/capnp/protocol.cpp \
  ipc/capnp/protocol.h \
  ipc/exception.h \
  ipc/interfaces.cpp \
  ipc/process.cpp \
  ipc/process.h \
  ipc/protocol.h
libbitcoin_ipc_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libbitcoin_ipc_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) $(LIBMULTIPROCESS_CFLAGS)

include $(MPGEN_PREFIX)/include/mpgen.mk
libbitcoin_ipc_mpgen_output = \
  $(libbitcoin_ipc_mpgen_input:=.c++) \
  $(libbitcoin_ipc_mpgen_input:=.h) \
  $(libbitcoin_ipc_mpgen_input:=.proxy-client.c++) \
  $(libbitcoin_ipc_mpgen_input:=.proxy-server.c++) \
  $(libbitcoin_ipc_mpgen_input:=.proxy-types.c++) \
  $(libbitcoin_ipc_mpgen_input:=.proxy-types.h) \
  $(libbitcoin_ipc_mpgen_input:=.proxy.h)
nodist_libbitcoin_ipc_a_SOURCES = $(libbitcoin_ipc_mpgen_output)
CLEANFILES += $(libbitcoin_ipc_mpgen_output)
endif

if EMBEDDED_LEVELDB
include Makefile.crc32c.include
include Makefile.leveldb.include
endif

include Makefile.cryptopp.include
include Makefile.libff.include
include Makefile.test_util.include
include Makefile.test_fuzz.include

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
