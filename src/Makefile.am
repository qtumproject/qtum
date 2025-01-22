# Copyright (c) 2013-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Pattern rule to print variables, e.g. make print-top_srcdir
print-%: FORCE
	@echo '$*'='$($*)'

DIST_SUBDIRS = secp256k1 blst

AM_LDFLAGS = $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS) $(SANITIZER_LDFLAGS) $(CORE_LDFLAGS)
AM_CXXFLAGS = $(CORE_CXXFLAGS) $(DEBUG_CXXFLAGS) $(HARDENED_CXXFLAGS) $(WARN_CXXFLAGS) $(NOWARN_CXXFLAGS) $(ERROR_CXXFLAGS) $(SANITIZER_CXXFLAGS) -DETH_FATDB=1
AM_OBJCXXFLAGS = $(AM_CXXFLAGS)
AM_CPPFLAGS = $(DEBUG_CPPFLAGS) $(HARDENED_CPPFLAGS) $(CORE_CPPFLAGS) -DQTUM_BUILD
AM_LIBTOOLFLAGS = --preserve-dup-deps
PTHREAD_FLAGS = $(PTHREAD_CFLAGS) $(PTHREAD_LIBS)
EXTRA_LIBRARIES =

lib_LTLIBRARIES =
noinst_LTLIBRARIES =

bin_PROGRAMS =
noinst_PROGRAMS =
check_PROGRAMS =
TESTS =
BENCHMARKS =

BITCOIN_INCLUDES=-I$(builddir) -I$(srcdir)/$(MINISKETCH_INCLUDE_DIR_INT) -I$(srcdir)/secp256k1/include -I$(srcdir)/$(UNIVALUE_INCLUDE_DIR_INT) $(LEVELDB_CPPFLAGS) $(SSL_CFLAGS) $(BOOST_CPPFLAGS)

BITCOIN_INCLUDES += -I$(srcdir)/libff/libff
BITCOIN_INCLUDES += -I$(srcdir)/libff
BITCOIN_INCLUDES += -I$(srcdir)/blst/bindings
BITCOIN_INCLUDES += -I$(srcdir)/evmone/evmc/include
BITCOIN_INCLUDES += -I$(srcdir)/evmone/include
BITCOIN_INCLUDES += -I$(srcdir)/evmone/lib
BITCOIN_INCLUDES += -I$(srcdir)/eth_client
BITCOIN_INCLUDES += -I$(srcdir)/eth_client/utils
BITCOIN_INCLUDES += -I$(srcdir)/eth_client/utils/ethash/include
BITCOIN_INCLUDES += -I$(srcdir)/eth_client/utils/libscrypt

LIBBITCOIN_NODE=libbitcoin_node.a
LIBBITCOIN_COMMON=libbitcoin_common.a
LIBBITCOIN_CONSENSUS=libbitcoin_consensus.a
LIBBITCOIN_CLI=libbitcoin_cli.a
LIBBITCOIN_UTIL=libqtum_util.a
LIBBITCOIN_CRYPTO_BASE=crypto/libbitcoin_crypto_base.la
LIBBITCOINQT=qt/libbitcoinqt.a
LIBSECP256K1=secp256k1/libsecp256k1.la
LIBCRYPTOPP=cryptopp/libcryptopp.a
LIBFF=libff/libff.a
LIBBLST=blst/libblst.a

if ENABLE_ZMQ
LIBBITCOIN_ZMQ=libbitcoin_zmq.a
endif
if BUILD_BITCOIN_KERNEL_LIB
LIBBITCOINKERNEL=libbitcoinkernel.la
endif
if ENABLE_WALLET
LIBBITCOIN_WALLET=libbitcoin_wallet.a
LIBBITCOIN_WALLET_TOOL=libbitcoin_wallet_tool.a
endif

LIBBITCOIN_CRYPTO = $(LIBBITCOIN_CRYPTO_BASE)
if ENABLE_SSE41
LIBBITCOIN_CRYPTO_SSE41 = crypto/libbitcoin_crypto_sse41.la
LIBBITCOIN_CRYPTO += $(LIBBITCOIN_CRYPTO_SSE41)
if ENABLE_X86_SHANI
LIBBITCOIN_CRYPTO_X86_SHANI = crypto/libbitcoin_crypto_x86_shani.la
LIBBITCOIN_CRYPTO += $(LIBBITCOIN_CRYPTO_X86_SHANI)
endif
endif
if ENABLE_AVX2
LIBBITCOIN_CRYPTO_AVX2 = crypto/libbitcoin_crypto_avx2.la
LIBBITCOIN_CRYPTO += $(LIBBITCOIN_CRYPTO_AVX2)
endif
if ENABLE_ARM_SHANI
LIBBITCOIN_CRYPTO_ARM_SHANI = crypto/libbitcoin_crypto_arm_shani.la
LIBBITCOIN_CRYPTO += $(LIBBITCOIN_CRYPTO_ARM_SHANI)
endif
noinst_LTLIBRARIES += $(LIBBITCOIN_CRYPTO)

$(LIBSECP256K1): $(wildcard secp256k1/src/*.h) $(wildcard secp256k1/src/*.c) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

$(LIBBLST):
	CC='$(CC)' AR='$(AR)' $(MAKE) -C $(@D)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES += \
  $(LIBBITCOIN_UTIL) \
  $(LIBBITCOIN_COMMON) \
  $(LIBBITCOIN_CONSENSUS) \
  $(LIBBITCOIN_NODE) \
  $(LIBBITCOIN_CLI) \
  $(LIBBITCOIN_IPC) \
  $(LIBBITCOIN_WALLET) \
  $(LIBBITCOIN_WALLET_TOOL) \
  $(LIBBITCOIN_ZMQ)

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

if BUILD_BITCOIN_CHAINSTATE
  bin_PROGRAMS += qtum-chainstate
endif

.PHONY: FORCE check-symbols check-security
# bitcoin core #
BITCOIN_CORE_H = \
  addresstype.h \
  addrdb.h \
  addrman.h \
  addrman_impl.h \
  attributes.h \
  banman.h \
  base58.h \
  bech32.h \
  bip324.h \
  blockencodings.h \
  blockfilter.h \
  chain.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  checkqueue.h \
  clientversion.h \
  cluster_linearize.h \
  coins.h \
  common/args.h \
  common/bloom.h \
  common/init.h \
  common/run_command.h \
  common/types.h \
  common/url.h \
  compat/assumptions.h \
  compat/byteswap.h \
  compat/compat.h \
  compat/cpuid.h \
  compat/endian.h \
  common/messages.h \
  common/settings.h \
  common/signmessage.h \
  common/system.h \
  common/threadpriority.h \
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
  headerssync.h \
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
  interfaces/mining.h \
  interfaces/node.h \
  interfaces/wallet.h \
  kernel/blockmanager_opts.h \
  kernel/chain.h \
  kernel/chainparams.h \
  kernel/chainstatemanager_opts.h \
  kernel/checks.h \
  kernel/coinstats.h \
  kernel/context.h \
  kernel/cs_main.h \
  kernel/disconnected_transactions.h \
  kernel/mempool_entry.h \
  kernel/mempool_limits.h \
  kernel/mempool_options.h \
  kernel/mempool_removal_reason.h \
  kernel/messagestartchars.h \
  kernel/notifications_interface.h \
  kernel/warning.h \
  key.h \
  key_io.h \
  logging.h \
  logging/timer.h \
  mapport.h \
  memusage.h \
  merkleblock.h \
  net.h \
  net_permissions.h \
  net_processing.h \
  net_types.h \
  netaddress.h \
  netbase.h \
  netgroup.h \
  netmessagemaker.h \
  node/abort.h \
  node/blockmanager_args.h \
  node/blockstorage.h \
  node/caches.h \
  node/chainstate.h \
  node/chainstatemanager_args.h \
  node/coin.h \
  node/coins_view_args.h \
  node/connection_types.h \
  node/context.h \
  node/database_args.h \
  node/eviction.h \
  node/interface_ui.h \
  node/kernel_notifications.h \
  node/mempool_args.h \
  node/mempool_persist.h \
  node/mempool_persist_args.h \
  node/miner.h \
  node/mini_miner.h \
  node/minisketchwrapper.h \
  node/peerman_args.h \
  node/protocol_version.h \
  node/psbt.h \
  node/timeoffsets.h \
  node/transaction.h \
  node/txreconciliation.h \
  node/types.h \
  node/utxo_snapshot.h \
  node/warnings.h \
  noui.h \
  outputtype.h \
  policy/feerate.h \
  policy/fees.h \
  policy/fees_args.h \
  policy/packages.h \
  policy/policy.h \
  policy/rbf.h \
  policy/settings.h \
  policy/truc_policy.h \
  pow.h \
  pos.h \
  protocol.h \
  psbt.h \
  random.h \
  randomenv.h \
  rest.h \
  rpc/blockchain.h \
  rpc/client.h \
  rpc/mempool.h \
  rpc/mining.h \
  rpc/protocol.h \
  rpc/rawtransaction_util.h \
  rpc/register.h \
  rpc/request.h \
  rpc/server.h \
  rpc/server_util.h \
  rpc/util.h \
  rpc/contract_util.h \
  scheduler.h \
  script/descriptor.h \
  script/keyorigin.h \
  script/miniscript.h \
  script/parsing.h \
  script/sigcache.h \
  script/sign.h \
  script/signingprovider.h \
  script/solver.h \
  signet.h \
  streams.h \
  support/allocators/pool.h \
  support/allocators/secure.h \
  support/allocators/zeroafterfree.h \
  support/cleanse.h \
  support/events.h \
  support/lockedpool.h \
  sync.h \
  threadsafety.h \
  torcontrol.h \
  txdb.h \
  txmempool.h \
  txorphanage.h \
  txrequest.h \
  undo.h \
  util/any.h \
  util/asmap.h \
  util/batchpriority.h \
  util/bip32.h \
  util/bitdeque.h \
  util/bitset.h \
  util/bytevectorhash.h \
  util/chaintype.h \
  util/check.h \
  util/epochguard.h \
  util/exception.h \
  util/fastrange.h \
  util/feefrac.h \
  util/fs.h \
  util/fs_helpers.h \
  util/golombrice.h \
  util/hash_type.h \
  util/hasher.h \
  util/insert.h \
  util/macros.h \
  util/moneystr.h \
  util/overflow.h \
  util/overloaded.h \
  util/rbf.h \
  util/readwritefile.h \
  util/result.h \
  util/serfloat.h \
  util/signalinterrupt.h \
  util/sock.h \
  util/strencodings.h \
  util/string.h \
  util/subprocess.h \
  util/syserror.h \
  util/task_runner.h \
  util/thread.h \
  util/threadinterrupt.h \
  util/threadnames.h \
  util/time.h \
  util/tokenpipe.h \
  util/trace.h \
  util/transaction_identifier.h \
  util/translation.h \
  util/types.h \
  util/ui_change_type.h \
  util/vecdeque.h \
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
  wallet/load.h \
  wallet/migrate.h \
  wallet/receive.h \
  wallet/rpc/util.h \
  wallet/rpc/wallet.h \
  wallet/rpc/addresses.h \
  wallet/rpc/contract.h \
  wallet/rpc/mining.h \
  wallet/salvage.h \
  wallet/scriptpubkeyman.h \
  wallet/spend.h \
  wallet/sqlite.h \
  wallet/transaction.h \
  wallet/types.h \
  wallet/wallet.h \
  wallet/walletdb.h \
  wallet/wallettool.h \
  wallet/walletutil.h \
  wallet/stake.h \
  walletinitinterface.h \
  zmq/zmqabstractnotifier.h \
  zmq/zmqnotificationinterface.h \
  zmq/zmqpublishnotifier.h \
  zmq/zmqrpc.h \
  zmq/zmqutil.h \
  qtum/posutils.h \
  qtum/qtumstate.h \
  qtum/qtumtransaction.h \
  qtum/qtumDGP.h \
  qtum/storageresults.h \
  qtum/qtumutils.h \
  qtum/qtumdelegation.h \
  qtum/qtumtoken.h \
  qtum/qtumledger.h \
  qtum/delegationutils.h


obj/build.h: FORCE
	@$(MKDIR_P) $(builddir)/obj
	$(AM_V_GEN) $(top_srcdir)/share/genbuild.sh "$(abs_top_builddir)/src/obj/build.h" \
	  "$(abs_top_srcdir)"
libqtum_util_a-clientversion.$(OBJEXT): obj/build.h

# node #
libbitcoin_node_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(LEVELDB_CPPFLAGS) $(MINIUPNPC_CPPFLAGS) $(NATPMP_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libbitcoin_node_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_node_a_SOURCES = \
  addrdb.cpp \
  addrman.cpp \
  banman.cpp \
  bip324.cpp \
  blockencodings.cpp \
  blockfilter.cpp \
  chain.cpp \
  consensus/tx_verify.cpp \
  dbwrapper.cpp \
  deploymentstatus.cpp \
  flatfile.cpp \
  headerssync.cpp \
  httprpc.cpp \
  httpserver.cpp \
  i2p.cpp \
  index/base.cpp \
  index/blockfilterindex.cpp \
  index/coinstatsindex.cpp \
  index/txindex.cpp \
  init.cpp \
  kernel/chain.cpp \
  kernel/checks.cpp \
  kernel/coinstats.cpp \
  kernel/context.cpp \
  kernel/cs_main.cpp \
  kernel/disconnected_transactions.cpp \
  kernel/mempool_removal_reason.cpp \
  mapport.cpp \
  net.cpp \
  net_processing.cpp \
  netgroup.cpp \
  node/abort.cpp \
  node/blockmanager_args.cpp \
  node/blockstorage.cpp \
  node/caches.cpp \
  node/chainstate.cpp \
  node/chainstatemanager_args.cpp \
  node/coin.cpp \
  node/coins_view_args.cpp \
  node/connection_types.cpp \
  node/context.cpp \
  node/database_args.cpp \
  node/eviction.cpp \
  node/interface_ui.cpp \
  node/interfaces.cpp \
  node/kernel_notifications.cpp \
  node/mempool_args.cpp \
  node/mempool_persist.cpp \
  node/mempool_persist_args.cpp \
  node/miner.cpp \
  node/mini_miner.cpp \
  node/minisketchwrapper.cpp \
  node/peerman_args.cpp \
  node/psbt.cpp \
  node/timeoffsets.cpp \
  node/transaction.cpp \
  node/txreconciliation.cpp \
  node/utxo_snapshot.cpp \
  node/warnings.cpp \
  noui.cpp \
  policy/fees.cpp \
  policy/fees_args.cpp \
  policy/packages.cpp \
  policy/rbf.cpp \
  policy/settings.cpp \
  policy/truc_policy.cpp \
  pow.cpp \
  rest.cpp \
  rpc/blockchain.cpp \
  rpc/fees.cpp \
  rpc/mempool.cpp \
  rpc/mining.cpp \
  rpc/net.cpp \
  rpc/node.cpp \
  rpc/output_script.cpp \
  rpc/rawtransaction.cpp \
  rpc/server.cpp \
  rpc/server_util.cpp \
  rpc/signmessage.cpp \
  rpc/txoutproof.cpp \
  script/sigcache.cpp \
  signet.cpp \
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
libbitcoin_node_a_SOURCES += wallet/init.cpp \
  wallet/stake.cpp \
  wallet/rpc/contract.cpp \
  wallet/rpc/mining.cpp
libbitcoin_node_a_CPPFLAGS += $(BDB_CPPFLAGS)
endif
if !ENABLE_WALLET
libbitcoin_node_a_SOURCES += dummywallet.cpp
endif
#

# zmq #
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
#

# wallet #
libbitcoin_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(BDB_CPPFLAGS) $(SQLITE_CFLAGS)
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
  wallet/migrate.cpp \
  wallet/receive.cpp \
  wallet/rpc/addresses.cpp \
  wallet/rpc/backup.cpp \
  wallet/rpc/coins.cpp \
  wallet/rpc/encrypt.cpp \
  wallet/rpc/spend.cpp \
  wallet/rpc/signmessage.cpp \
  wallet/rpc/transactions.cpp \
  wallet/rpc/util.cpp \
  wallet/rpc/wallet.cpp \
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
#

# wallet tool #
libbitcoin_wallet_tool_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libbitcoin_wallet_tool_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_wallet_tool_a_SOURCES = \
  wallet/wallettool.cpp \
  $(BITCOIN_CORE_H)
#

# crypto #

# crypto_base contains the unspecialized (unoptimized) versions of our
# crypto functions. Functions that require custom compiler flags and/or
# runtime opt-in are omitted.
crypto_libbitcoin_crypto_base_la_CPPFLAGS = $(AM_CPPFLAGS)

# Specify -static in both CXXFLAGS and LDFLAGS so libtool will only build a
# static version of this library. We don't need a dynamic version, and a dynamic
# version can't be used on windows anyway because the library doesn't currently
# export DLL symbols.
crypto_libbitcoin_crypto_base_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -static
crypto_libbitcoin_crypto_base_la_LDFLAGS = $(AM_LDFLAGS) -static

crypto_libbitcoin_crypto_base_la_SOURCES = \
  crypto/aes.cpp \
  crypto/aes.h \
  crypto/chacha20.h \
  crypto/chacha20.cpp \
  crypto/chacha20poly1305.h \
  crypto/chacha20poly1305.cpp \
  crypto/common.h \
  crypto/hex_base.cpp \
  crypto/hex_base.h \
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
  crypto/sha256_sse4.cpp \
  crypto/sha3.cpp \
  crypto/sha3.h \
  crypto/sha512.cpp \
  crypto/sha512.h \
  crypto/siphash.cpp \
  crypto/siphash.h \
  support/cleanse.cpp

# See explanation for -static in crypto_libbitcoin_crypto_base_la's LDFLAGS and
# CXXFLAGS above
crypto_libbitcoin_crypto_sse41_la_LDFLAGS = $(AM_LDFLAGS) -static
crypto_libbitcoin_crypto_sse41_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -static
crypto_libbitcoin_crypto_sse41_la_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libbitcoin_crypto_sse41_la_CXXFLAGS += $(SSE41_CXXFLAGS)
crypto_libbitcoin_crypto_sse41_la_CPPFLAGS += -DENABLE_SSE41
crypto_libbitcoin_crypto_sse41_la_SOURCES = crypto/sha256_sse41.cpp

# See explanation for -static in crypto_libbitcoin_crypto_base_la's LDFLAGS and
# CXXFLAGS above
crypto_libbitcoin_crypto_avx2_la_LDFLAGS = $(AM_LDFLAGS) -static
crypto_libbitcoin_crypto_avx2_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -static
crypto_libbitcoin_crypto_avx2_la_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libbitcoin_crypto_avx2_la_CXXFLAGS += $(AVX2_CXXFLAGS)
crypto_libbitcoin_crypto_avx2_la_CPPFLAGS += -DENABLE_AVX2
crypto_libbitcoin_crypto_avx2_la_SOURCES = crypto/sha256_avx2.cpp

# See explanation for -static in crypto_libbitcoin_crypto_base_la's LDFLAGS and
# CXXFLAGS above
crypto_libbitcoin_crypto_x86_shani_la_LDFLAGS = $(AM_LDFLAGS) -static
crypto_libbitcoin_crypto_x86_shani_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -static
crypto_libbitcoin_crypto_x86_shani_la_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libbitcoin_crypto_x86_shani_la_CXXFLAGS += $(X86_SHANI_CXXFLAGS)
crypto_libbitcoin_crypto_x86_shani_la_CPPFLAGS += -DENABLE_SSE41 -DENABLE_X86_SHANI
crypto_libbitcoin_crypto_x86_shani_la_SOURCES = crypto/sha256_x86_shani.cpp

# See explanation for -static in crypto_libbitcoin_crypto_base_la's LDFLAGS and
# CXXFLAGS above
crypto_libbitcoin_crypto_arm_shani_la_LDFLAGS = $(AM_LDFLAGS) -static
crypto_libbitcoin_crypto_arm_shani_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -static
crypto_libbitcoin_crypto_arm_shani_la_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libbitcoin_crypto_arm_shani_la_CXXFLAGS += $(ARM_SHANI_CXXFLAGS)
crypto_libbitcoin_crypto_arm_shani_la_CPPFLAGS += -DENABLE_ARM_SHANI
crypto_libbitcoin_crypto_arm_shani_la_SOURCES = crypto/sha256_arm_shani.cpp
#

# consensus #
libbitcoin_consensus_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libbitcoin_consensus_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_consensus_a_SOURCES = \
  arith_uint256.cpp \
  arith_uint256.h \
  consensus/consensus.cpp \
  consensus/amount.h \
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
  uint256.h
#

# common #
libbitcoin_common_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) -DQTUM_BUILD
libbitcoin_common_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_common_a_CFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -std=c99
libbitcoin_common_a_SOURCES = \
  addresstype.cpp \
  base58.cpp \
  bech32.cpp \
  chainparamsbase.cpp \
  chainparams.cpp \
  coins.cpp \
  common/args.cpp \
  common/bloom.cpp \
  common/config.cpp \
  common/init.cpp \
  common/interfaces.cpp \
  common/messages.cpp \
  common/run_command.cpp \
  common/settings.cpp \
  common/signmessage.cpp \
  common/system.cpp \
  common/url.cpp \
  compressor.cpp \
  core_read.cpp \
  core_write.cpp \
  deploymentinfo.cpp \
  external_signer.cpp \
  init/common.cpp \
  kernel/chainparams.cpp \
  key.cpp \
  key_io.cpp \
  merkleblock.cpp \
  net_types.cpp \
  netaddress.cpp \
  netbase.cpp \
  net_permissions.cpp \
  outputtype.cpp \
  policy/feerate.cpp \
  policy/policy.cpp \
  protocol.cpp \
  psbt.cpp \
  pos.cpp \
  rpc/external_signer.cpp \
  rpc/rawtransaction_util.cpp \
  rpc/request.cpp \
  rpc/util.cpp \
  rpc/contract_util.cpp \
  scheduler.cpp \
  script/descriptor.cpp \
  script/miniscript.cpp \
  script/parsing.cpp \
  script/sign.cpp \
  script/signingprovider.cpp \
  script/solver.cpp \
  qtum/qtumutils.cpp \
  qtum/qtumDGP.cpp \
  qtum/qtumtoken.cpp \
  qtum/qtumdelegation.cpp \
  qtum/delegationutils.cpp \
  util/contractabi.cpp \
  libff/libff/algebra/curves/public_params.hpp \
  libff/libff/algebra/curves/curve_utils.hpp \
  evmone/include/evmone/evmone.h \
  evmone/evmc/lib/instructions/instruction_metrics.c \
  evmone/evmc/lib/instructions/instruction_names.c \
  evmone/evmc/include/evmc/bytes.hpp \
  evmone/evmc/include/evmc/evmc.h \
  evmone/evmc/include/evmc/evmc.hpp \
  evmone/evmc/include/evmc/filter_iterator.hpp \
  evmone/evmc/include/evmc/helpers.h \
  evmone/evmc/include/evmc/hex.hpp \
  evmone/evmc/include/evmc/instructions.h \
  evmone/evmc/include/evmc/loader.h \
  evmone/evmc/include/evmc/mocked_host.hpp \
  evmone/evmc/include/evmc/tooling.hpp \
  evmone/evmc/include/evmc/utils.h \
  evmone/lib/evmone/advanced_analysis.cpp \
  evmone/lib/evmone/advanced_analysis.hpp \
  evmone/lib/evmone/advanced_execution.cpp \
  evmone/lib/evmone/advanced_execution.hpp \
  evmone/lib/evmone/advanced_instructions.cpp \
  evmone/lib/evmone/baseline_analysis.cpp \
  evmone/lib/evmone/baseline_execution.cpp \
  evmone/lib/evmone/baseline.hpp \
  evmone/lib/evmone/baseline_instruction_table.cpp \
  evmone/lib/evmone/baseline_instruction_table.hpp \
  evmone/lib/evmone/constants.hpp \
  evmone/lib/evmone/compat.hpp \
  evmone/lib/evmone/eof.cpp \
  evmone/lib/evmone/eof.hpp \
  evmone/lib/evmone/execution_state.hpp \
  evmone/lib/evmone/instructions_calls.cpp \
  evmone/lib/evmone/instructions.hpp \
  evmone/lib/evmone/instructions_opcodes.hpp \
  evmone/lib/evmone/instructions_storage.cpp \
  evmone/lib/evmone/instructions_traits.hpp \
  evmone/lib/evmone/instructions_xmacro.hpp \
  evmone/lib/evmone/tracing.cpp \
  evmone/lib/evmone/tracing.hpp \
  evmone/lib/evmone/vm.cpp \
  evmone/lib/evmone/vm.hpp \
  evmone/lib/evmone_precompiles/bls.cpp \
  evmone/lib/evmone_precompiles/bls.hpp \
  evmone/lib/evmone_precompiles/kzg.cpp \
  evmone/lib/evmone_precompiles/kzg.hpp \
  evmone/lib/evmone_precompiles/sha256.cpp \
  evmone/lib/evmone_precompiles/sha256.hpp \
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
  eth_client/libdevcrypto/LibKzg.cpp \
  eth_client/libdevcrypto/LibKzg.h \
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
  eth_client/utils/ethash/include/ethash/global_context.h \
  eth_client/utils/ethash/include/ethash/global_context.hpp \
  eth_client/utils/ethash/include/ethash/hash_types.h \
  eth_client/utils/ethash/include/ethash/hash_types.hpp \
  eth_client/utils/ethash/include/ethash/keccak.h \
  eth_client/utils/ethash/include/ethash/keccak.hpp \
  eth_client/utils/ethash/include/ethash/version.h \
  eth_client/utils/ethash/lib/ethash/endianness.hpp \
  eth_client/utils/ethash/lib/ethash/ethash.cpp \
  eth_client/utils/ethash/lib/ethash/ethash-internal.hpp \
  eth_client/utils/ethash/lib/ethash/primes.c \
  eth_client/utils/ethash/lib/ethash/primes.h \
  eth_client/utils/ethash/lib/global_context/global_context.cpp \
  eth_client/utils/ethash/lib/keccak/keccak.c \
  eth_client/utils/ethash/lib/support/attributes.h \
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
  eth_client/utils/libscrypt/sysendian.h \
  $(BITCOIN_CORE_H)
#

# util #
libqtum_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libqtum_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libqtum_util_a_SOURCES = \
  support/lockedpool.cpp \
  clientversion.cpp \
  logging.cpp \
  random.cpp \
  randomenv.cpp \
  streams.cpp \
  sync.cpp \
  util/asmap.cpp \
  util/batchpriority.cpp \
  util/bip32.cpp \
  util/bytevectorhash.cpp \
  util/chaintype.cpp \
  util/check.cpp \
  util/exception.cpp \
  util/feefrac.cpp \
  util/fs.cpp \
  util/fs_helpers.cpp \
  util/hasher.cpp \
  util/sock.cpp \
  util/syserror.cpp \
  util/moneystr.cpp \
  util/rbf.cpp \
  util/readwritefile.cpp \
  util/signalinterrupt.cpp \
  util/thread.cpp \
  util/threadinterrupt.cpp \
  util/threadnames.cpp \
  util/serfloat.cpp \
  util/strencodings.cpp \
  util/string.cpp \
  util/time.cpp \
  util/tokenpipe.cpp \
  util/tokenstr.cpp \
  $(BITCOIN_CORE_H)
#

# cli #
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
  $(LIBBITCOIN_WALLET) \
  $(LIBBITCOIN_COMMON) \
  $(LIBBITCOIN_UTIL) \
  $(LIBUNIVALUE) \
  $(LIBBITCOIN_ZMQ) \
  $(LIBBITCOIN_CONSENSUS) \
  $(LIBBITCOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBMEMENV) \
  $(LIBCRYPTOPP) \
  $(LIBFF) \
  $(LIBSECP256K1) \
  $(LIBBLST)

qtum_bin_ldadd += $(BOOST_LIBS) $(BDB_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(MINIUPNPC_LIBS) $(NATPMP_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ZMQ_LIBS) $(SQLITE_LIBS) $(GMP_LIBS) $(GMPXX_LIBS)

qtumd_SOURCES = $(qtum_daemon_sources) init/bitcoind.cpp
qtumd_CPPFLAGS = $(qtum_bin_cppflags)
qtumd_CXXFLAGS = $(qtum_bin_cxxflags)
qtumd_LDFLAGS = $(qtum_bin_ldflags)
qtumd_LDADD = $(LIBBITCOIN_NODE) $(qtum_bin_ldadd)

qtum_node_SOURCES = $(qtum_daemon_sources) init/bitcoin-node.cpp
qtum_node_CPPFLAGS = $(qtum_bin_cppflags)
qtum_node_CXXFLAGS = $(qtum_bin_cxxflags)
qtum_node_LDFLAGS = $(qtum_bin_ldflags)
qtum_node_LDADD = $(LIBBITCOIN_NODE) $(qtum_bin_ldadd) $(LIBBITCOIN_IPC) $(LIBMULTIPROCESS_LIBS)

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
  $(LIBBITCOIN_COMMON) \
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
qtum_wallet_SOURCES = bitcoin-wallet.cpp
qtum_wallet_SOURCES += init/bitcoin-wallet.cpp
qtum_wallet_CPPFLAGS = $(qtum_bin_cppflags)
qtum_wallet_CXXFLAGS = $(qtum_bin_cxxflags)
qtum_wallet_LDFLAGS = $(qtum_bin_ldflags)
qtum_wallet_LDADD = \
  $(LIBBITCOIN_WALLET_TOOL) \
  $(LIBBITCOIN_WALLET) \
  $(LIBBITCOIN_COMMON) \
  $(LIBBITCOIN_UTIL) \
  $(LIBUNIVALUE) \
  $(LIBBITCOIN_CONSENSUS) \
  $(LIBBITCOIN_CRYPTO) \
  $(LIBSECP256K1) \
  $(BDB_LIBS) \
  $(SQLITE_LIBS) \
  $(BOOST_LIBS) \
  $(CRYPTO_LIBS)

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

qtum_util_LDADD += $(BOOST_LIBS) $(CRYPTO_LIBS)
#

# qtum-chainstate binary #
qtum_chainstate_SOURCES = bitcoin-chainstate.cpp
qtum_chainstate_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
qtum_chainstate_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)

qtum_chainstate_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(PTHREAD_FLAGS) $(LIBTOOL_APP_LDFLAGS) -static
qtum_chainstate_LDADD = $(LIBBITCOINKERNEL)

# libtool is unable to calculate this indirect dependency, presumably because it's a subproject.
# libsecp256k1 only needs to be linked in when libbitcoinkernel is static.
qtum_chainstate_LDADD += $(LIBSECP256K1)
#

# bitcoinkernel library #
if BUILD_BITCOIN_KERNEL_LIB
lib_LTLIBRARIES += $(LIBBITCOINKERNEL)

libbitcoinkernel_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS) $(PTHREAD_FLAGS)
libbitcoinkernel_la_LIBADD = $(LIBBITCOIN_CRYPTO) $(LIBLEVELDB) $(LIBMEMENV) $(LIBSECP256K1)
libbitcoinkernel_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS)

# libbitcoinkernel requires default symbol visibility, explicitly specify that
# here so that things still work even when user configures with
#   --enable-reduce-exports
#
# Note this is a quick hack that will be removed as we incrementally define what
# to export from the library.
libbitcoinkernel_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -fvisibility=default

# TODO: libbitcoinkernel is a work in progress consensus engine library, as more
#       and more modules are decoupled from the consensus engine, this list will
#       shrink to only those which are absolutely necessary.
libbitcoinkernel_la_SOURCES = \
  kernel/bitcoinkernel.cpp \
  arith_uint256.cpp \
  chain.cpp \
  clientversion.cpp \
  coins.cpp \
  compressor.cpp \
  consensus/merkle.cpp \
  consensus/tx_check.cpp \
  consensus/tx_verify.cpp \
  core_read.cpp \
  dbwrapper.cpp \
  deploymentinfo.cpp \
  deploymentstatus.cpp \
  flatfile.cpp \
  hash.cpp \
  kernel/chain.cpp \
  kernel/checks.cpp \
  kernel/chainparams.cpp \
  kernel/coinstats.cpp \
  kernel/context.cpp \
  kernel/cs_main.cpp \
  kernel/disconnected_transactions.cpp \
  kernel/mempool_removal_reason.cpp \
  logging.cpp \
  node/blockstorage.cpp \
  node/chainstate.cpp \
  node/utxo_snapshot.cpp \
  policy/feerate.cpp \
  policy/packages.cpp \
  policy/policy.cpp \
  policy/rbf.cpp \
  policy/settings.cpp \
  policy/truc_policy.cpp \
  pow.cpp \
  primitives/block.cpp \
  primitives/transaction.cpp \
  pubkey.cpp \
  random.cpp \
  randomenv.cpp \
  script/interpreter.cpp \
  script/script.cpp \
  script/script_error.cpp \
  script/sigcache.cpp \
  script/solver.cpp \
  signet.cpp \
  streams.cpp \
  support/lockedpool.cpp \
  sync.cpp \
  txdb.cpp \
  txmempool.cpp \
  uint256.cpp \
  util/chaintype.cpp \
  util/check.cpp \
  util/feefrac.cpp \
  util/fs.cpp \
  util/fs_helpers.cpp \
  util/hasher.cpp \
  util/moneystr.cpp \
  util/rbf.cpp \
  util/serfloat.cpp \
  util/signalinterrupt.cpp \
  util/strencodings.cpp \
  util/string.cpp \
  util/syserror.cpp \
  util/threadnames.cpp \
  util/time.cpp \
  util/tokenpipe.cpp \
  validation.cpp \
  validationinterface.cpp \
  versionbits.cpp

# Required for obj/build.h to be generated first.
# More details: https://www.gnu.org/software/automake/manual/html_node/Built-Sources-Example.html
libbitcoinkernel_la-clientversion.l$(OBJEXT): obj/build.h
endif # BUILD_BITCOIN_KERNEL_LIB
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
	-rm -f leveldb/*/*.gcda leveldb/*/*.gcno leveldb/helpers/memenv/*.gcda leveldb/helpers/memenv/*.gcno
	-rm -f cryptopp/*.o libff/*.o
	-rm -f config.h
	-rm -rf test/__pycache__
	-$(MAKE) -C blst clean

.rc.o:
	@test -f $(WINDRES) || (echo "windres $(WINDRES) not found, but is required to compile windows resource files"; exit 1)
	## FIXME: How to get the appropriate modulename_CPPFLAGS in here?
	$(AM_V_GEN) $(WINDRES) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) -DWINDRES_PREPROC -i $< -o $@

check-symbols: $(bin_PROGRAMS)
	@echo "Running symbol and dynamic library checks..."
	$(AM_V_at) $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py $(bin_PROGRAMS)

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

# Explicitly list dependencies on generated headers as described in
# https://www.gnu.org/software/automake/manual/html_node/Built-Sources-Example.html#Recording-Dependencies-manually
ipc/capnp/libbitcoin_ipc_a-protocol.$(OBJEXT): $(libbitcoin_ipc_mpgen_input:=.h)

if BUILD_MULTIPROCESS
LIBBITCOIN_IPC=libbitcoin_ipc.a
libbitcoin_ipc_a_SOURCES = \
  ipc/capnp/common-types.h \
  ipc/capnp/context.h \
  ipc/capnp/init-types.h \
  ipc/capnp/protocol.cpp \
  ipc/capnp/protocol.h \
  ipc/context.h \
  ipc/exception.h \
  ipc/interfaces.cpp \
  ipc/process.cpp \
  ipc/process.h \
  ipc/protocol.h
libbitcoin_ipc_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(BOOST_CPPFLAGS)
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

%.raw.h: %.raw
	@$(MKDIR_P) $(@D)
	$(AM_V_GEN) { \
	 echo "static unsigned const char $(*F)_raw[] = {" && \
	 $(HEXDUMP) -v -e '8/1 "0x%02x, "' -e '"\n"' $< | $(SED) -e 's/0x  ,//g' && \
	 echo "};"; \
	} > "$@.new" && mv -f "$@.new" "$@"

include Makefile.minisketch.include

include Makefile.crc32c.include
include Makefile.leveldb.include

include Makefile.cryptopp.include
include Makefile.libff.include
include Makefile.test_util.include
include Makefile.test_fuzz.include

include Makefile.test.include

if ENABLE_BENCH
include Makefile.bench.include
endif

if ENABLE_QT
include Makefile.qt.include
endif

if ENABLE_QT_TESTS
include Makefile.qttest.include
endif

include Makefile.univalue.include
