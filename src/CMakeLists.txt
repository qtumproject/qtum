# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include(AddWindowsResources)

configure_file(${PROJECT_SOURCE_DIR}/cmake/bitcoin-build-config.h.in bitcoin-build-config.h USE_SOURCE_PERMISSIONS @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libff/libff)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/evmone/evmc/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/evmone/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/evmone/lib)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/eth_client)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/eth_client/utils)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/eth_client/utils/ethash/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/eth_client/utils/libscrypt)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/leveldb/include)

#=============================
# Subprojects
#=============================
# Add blst library
add_subdirectory(blst)

# Subprojects include subdirectories that do or could have tests
# and/or benchmark binaries, such as all subtrees and univalue.
# These need to be included before CMAKE_*_OUTPUT_DIRECTORY variables
# are set, so output locations of subproject tests and libraries are
# not overridden.
include(../cmake/crc32c.cmake)
include(../cmake/leveldb.cmake)
include(../cmake/minisketch.cmake)
add_subdirectory(univalue)
if (ENABLE_IPC AND NOT WITH_EXTERNAL_LIBMULTIPROCESS)
  include(../cmake/libmultiprocess.cmake)
  add_libmultiprocess(ipc/libmultiprocess)
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DQTUM_BUILD -DETH_FATDB=1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQTUM_BUILD -DETH_FATDB=1")
include(../cmake/secp256k1.cmake)
add_secp256k1(secp256k1)

# Set top-level target output locations.
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
endif()

add_custom_target(generate_build_info
  BYPRODUCTS ${PROJECT_BINARY_DIR}/src/bitcoin-build-info.h
  COMMAND ${CMAKE_COMMAND} -DBUILD_INFO_HEADER_PATH=${PROJECT_BINARY_DIR}/src/bitcoin-build-info.h -DSOURCE_DIR=${PROJECT_SOURCE_DIR} -P ${PROJECT_SOURCE_DIR}/cmake/script/GenerateBuildInfo.cmake
  COMMENT "Generating bitcoin-build-info.h"
  VERBATIM
)
add_library(bitcoin_clientversion STATIC
  clientversion.cpp
)
target_link_libraries(bitcoin_clientversion
  PRIVATE
    core_interface
)
add_dependencies(bitcoin_clientversion generate_build_info)

add_subdirectory(crypto)
add_subdirectory(util)
if(ENABLE_IPC)
  add_subdirectory(ipc)
endif()

add_library(bitcoin_consensus STATIC EXCLUDE_FROM_ALL
  arith_uint256.cpp
  consensus/consensus.cpp
  consensus/merkle.cpp
  consensus/tx_check.cpp
  hash.cpp
  primitives/block.cpp
  primitives/transaction.cpp
  pubkey.cpp
  script/interpreter.cpp
  script/script.cpp
  script/script_error.cpp
  uint256.cpp
)
target_link_libraries(bitcoin_consensus
  PRIVATE
    core_interface
    bitcoin_crypto
    secp256k1
)

if(WITH_ZMQ)
  add_subdirectory(zmq)
endif()

# Add cryptopp library
add_subdirectory(cryptopp)

# Add libff library
cmake_policy(PUSH)
set(WITH_PROCPS OFF CACHE BOOL "" FORCE)
set(IS_LIBFF_PARENT OFF CACHE BOOL "" FORCE)
set(USE_ASM OFF CACHE BOOL "" FORCE)
set(CURVE ALT_BN128 CACHE STRING "" FORCE)
set(INSTALL_LIBFF OFF CACHE BOOL "" FORCE)
add_subdirectory(libff)
cmake_policy(POP)

# Add ethash library
add_library(ethash STATIC EXCLUDE_FROM_ALL
  eth_client/utils/ethash/lib/ethash/ethash.cpp
  eth_client/utils/ethash/lib/ethash/primes.c
  eth_client/utils/ethash/lib/global_context/global_context.cpp
  eth_client/utils/ethash/lib/keccak/keccak.c
)

# Add evmone library
add_library(evmone STATIC EXCLUDE_FROM_ALL
  evmone/evmc/lib/instructions/instruction_metrics.c
  evmone/evmc/lib/instructions/instruction_names.c
  evmone/lib/evmone/advanced_analysis.cpp
  evmone/lib/evmone/advanced_execution.cpp
  evmone/lib/evmone/advanced_instructions.cpp
  evmone/lib/evmone/baseline_analysis.cpp
  evmone/lib/evmone/baseline_execution.cpp
  evmone/lib/evmone/baseline_instruction_table.cpp
  evmone/lib/evmone/delegation.cpp
  evmone/lib/evmone/eof.cpp
  evmone/lib/evmone/instructions_calls.cpp
  evmone/lib/evmone/instructions_storage.cpp
  evmone/lib/evmone/tracing.cpp
  evmone/lib/evmone/vm.cpp
  evmone/lib/evmone_precompiles/blake2b.cpp
  evmone/lib/evmone_precompiles/bls.cpp
  evmone/lib/evmone_precompiles/bn254.cpp
  evmone/lib/evmone_precompiles/kzg.cpp
  evmone/lib/evmone_precompiles/ripemd160.cpp
  evmone/lib/evmone_precompiles/secp256k1.cpp
  evmone/lib/evmone_precompiles/sha256.cpp
  evmone/lib/evmone_precompiles/pairing/bn254/pairing.cpp
)
target_link_libraries(evmone PRIVATE blst ethash)
target_compile_definitions(evmone PRIVATE PROJECT_VERSION="\\\"0.16.0\\\"")

# Home for common functionality shared by different executables and libraries.
# Similar to `bitcoin_util` library, but higher-level.
add_library(bitcoin_common STATIC EXCLUDE_FROM_ALL
  addresstype.cpp
  base58.cpp
  bech32.cpp
  chain.cpp
  chainparams.cpp
  chainparamsbase.cpp
  coins.cpp
  common/args.cpp
  common/bloom.cpp
  common/config.cpp
  common/init.cpp
  common/interfaces.cpp
  common/messages.cpp
  common/netif.cpp
  common/pcp.cpp
  common/run_command.cpp
  common/settings.cpp
  common/signmessage.cpp
  common/system.cpp
  common/url.cpp
  compressor.cpp
  core_io.cpp
  deploymentinfo.cpp
  external_signer.cpp
  init/common.cpp
  kernel/chainparams.cpp
  key.cpp
  key_io.cpp
  merkleblock.cpp
  musig.cpp
  net_permissions.cpp
  net_types.cpp
  netaddress.cpp
  netbase.cpp
  outputtype.cpp
  policy/feerate.cpp
  policy/policy.cpp
  pow.cpp
  protocol.cpp
  psbt.cpp
  rpc/rawtransaction_util.cpp
  rpc/request.cpp
  rpc/util.cpp
  scheduler.cpp
  script/descriptor.cpp
  script/miniscript.cpp
  script/parsing.cpp
  script/sign.cpp
  script/signingprovider.cpp
  script/solver.cpp
  eth_client/libdevcore/Address.cpp
  eth_client/libdevcore/Common.cpp
  eth_client/libdevcore/CommonData.cpp
  eth_client/libdevcore/CommonIO.cpp
  eth_client/libdevcore/DBFactory.cpp
  eth_client/libdevcore/FileSystem.cpp
  eth_client/libdevcore/FixedHash.cpp
  eth_client/libdevcore/JsonUtils.cpp
  eth_client/libdevcore/LevelDB.cpp
  eth_client/libdevcore/Log.cpp
  eth_client/libdevcore/OverlayDB.cpp
  eth_client/libdevcore/RLP.cpp
  eth_client/libdevcore/SHA3.cpp
  eth_client/libdevcore/StateCacheDB.cpp
  eth_client/libdevcore/TrieCommon.cpp
  eth_client/libdevcore/TrieHash.cpp
  eth_client/libdevcrypto/Blake2.cpp
  eth_client/libdevcrypto/Common.cpp
  eth_client/libdevcrypto/CryptoPP.cpp
  eth_client/libdevcrypto/Hash.cpp
  eth_client/libdevcrypto/LibBls.cpp
  eth_client/libdevcrypto/LibSnark.cpp
  eth_client/libdevcrypto/LibKzg.cpp
  eth_client/libethashseal/GenesisInfo.cpp
  eth_client/libethashseal/genesis/qtumNetwork.cpp
  eth_client/libethcore/BlockHeader.cpp
  eth_client/libethcore/ChainOperationParams.cpp
  eth_client/libethcore/Common.cpp
  eth_client/libethcore/EVMSchedule.cpp
  eth_client/libethcore/LogEntry.cpp
  eth_client/libethcore/Precompiled.cpp
  eth_client/libethcore/SealEngine.cpp
  eth_client/libethcore/TransactionBase.cpp
  eth_client/libethereum/Account.cpp
  eth_client/libethereum/ChainParams.cpp
  eth_client/libethereum/DatabasePaths.cpp
  eth_client/libethereum/Executive.cpp
  eth_client/libethereum/ExtVM.cpp
  eth_client/libethereum/State.cpp
  eth_client/libethereum/Transaction.cpp
  eth_client/libethereum/TransactionReceipt.cpp
  eth_client/libethereum/ValidationSchemes.cpp
  eth_client/libevm/EVMC.cpp
  eth_client/libevm/ExtVMFace.cpp
  eth_client/libevm/VMFactory.cpp
  eth_client/utils/json_spirit/json_spirit_reader.cpp
  eth_client/utils/json_spirit/json_spirit_value.cpp
  eth_client/utils/json_spirit/json_spirit_writer.cpp
  eth_client/utils/libscrypt/b64.c
  eth_client/utils/libscrypt/crypto-mcf.c
  eth_client/utils/libscrypt/crypto-scrypt-saltgen.c
  eth_client/utils/libscrypt/crypto_scrypt-check.c
  eth_client/utils/libscrypt/crypto_scrypt-hash.c
  eth_client/utils/libscrypt/crypto_scrypt-hexconvert.c
  eth_client/utils/libscrypt/crypto_scrypt-nosse.c
  eth_client/utils/libscrypt/sha256.c
  eth_client/utils/libscrypt/slowequals.c
)

link_directories(${CMAKE_BINARY_DIR}/lib)
target_link_libraries(bitcoin_common
  PRIVATE
    core_interface
    bitcoin_consensus
    bitcoin_util
    univalue
    secp256k1
    Boost::headers
    $<TARGET_NAME_IF_EXISTS:USDT::headers>
    $<$<PLATFORM_ID:Windows>:ws2_32>
    leveldb
    ff
    cryptopp
    Boost::filesystem
    Boost::program_options
    Boost::thread
    $<TARGET_NAME_IF_EXISTS:Boost::process>
    evmone
    OpenSSL::SSL
    OpenSSL::Crypto
)

include(InstallBinaryComponent)

if(ENABLE_WALLET)
  add_subdirectory(wallet)

  if(BUILD_WALLET_TOOL)
    add_executable(qtum-wallet
      bitcoin-wallet.cpp
      init/bitcoin-wallet.cpp
      wallet/wallettool.cpp
    )
    add_windows_resources(qtum-wallet bitcoin-wallet-res.rc)
    add_windows_application_manifest(qtum-wallet)
    target_link_libraries(qtum-wallet
      core_interface
      bitcoin_wallet
      bitcoin_common
      bitcoin_util
      Boost::headers
    )
    install_binary_component(qtum-wallet HAS_MANPAGE)
  endif()
endif()


# P2P and RPC server functionality used by `bitcoind` and `bitcoin-qt` executables.
add_library(bitcoin_node STATIC EXCLUDE_FROM_ALL
  addrdb.cpp
  addrman.cpp
  banman.cpp
  bip324.cpp
  blockencodings.cpp
  blockfilter.cpp
  consensus/tx_verify.cpp
  dbwrapper.cpp
  deploymentstatus.cpp
  flatfile.cpp
  headerssync.cpp
  httprpc.cpp
  httpserver.cpp
  i2p.cpp
  index/base.cpp
  index/blockfilterindex.cpp
  index/coinstatsindex.cpp
  index/txindex.cpp
  index/txospenderindex.cpp
  init.cpp
  kernel/chain.cpp
  kernel/checks.cpp
  kernel/coinstats.cpp
  kernel/context.cpp
  kernel/cs_main.cpp
  kernel/disconnected_transactions.cpp
  kernel/mempool_removal_reason.cpp
  mapport.cpp
  net.cpp
  net_processing.cpp
  netgroup.cpp
  node/abort.cpp
  node/blockmanager_args.cpp
  node/blockstorage.cpp
  node/caches.cpp
  node/chainstate.cpp
  node/chainstatemanager_args.cpp
  node/coin.cpp
  node/coins_view_args.cpp
  node/connection_types.cpp
  node/context.cpp
  node/database_args.cpp
  node/eviction.cpp
  node/interface_ui.cpp
  node/interfaces.cpp
  node/kernel_notifications.cpp
  node/mempool_args.cpp
  node/mempool_persist.cpp
  node/mempool_persist_args.cpp
  node/miner.cpp
  node/mini_miner.cpp
  node/minisketchwrapper.cpp
  node/peerman_args.cpp
  node/psbt.cpp
  node/timeoffsets.cpp
  node/transaction.cpp
  node/txdownloadman_impl.cpp
  node/txorphanage.cpp
  node/txreconciliation.cpp
  node/utxo_snapshot.cpp
  node/warnings.cpp
  noui.cpp
  policy/ephemeral_policy.cpp
  policy/fees/block_policy_estimator.cpp
  policy/fees/block_policy_estimator_args.cpp
  policy/packages.cpp
  policy/rbf.cpp
  policy/settings.cpp
  policy/truc_policy.cpp
  private_broadcast.cpp
  rest.cpp
  rpc/blockchain.cpp
  rpc/external_signer.cpp
  rpc/fees.cpp
  rpc/mempool.cpp
  rpc/mining.cpp
  rpc/net.cpp
  rpc/node.cpp
  rpc/output_script.cpp
  rpc/rawtransaction.cpp
  rpc/server.cpp
  rpc/server_util.cpp
  rpc/signmessage.cpp
  rpc/txoutproof.cpp
  script/sigcache.cpp
  signet.cpp
  torcontrol.cpp
  txdb.cpp
  txgraph.cpp
  txmempool.cpp
  txrequest.cpp
  validation.cpp
  validationinterface.cpp
  versionbits.cpp
  $<$<TARGET_EXISTS:bitcoin_wallet>:wallet/init.cpp>
  $<$<NOT:$<TARGET_EXISTS:bitcoin_wallet>>:dummywallet.cpp>
)
target_link_libraries(bitcoin_node
  PRIVATE
    core_interface
    bitcoin_common
    bitcoin_util
    $<TARGET_NAME_IF_EXISTS:bitcoin_zmq>
    leveldb
    minisketch
    univalue
    Boost::headers
    $<TARGET_NAME_IF_EXISTS:libevent::core>
    $<TARGET_NAME_IF_EXISTS:libevent::extra>
    $<TARGET_NAME_IF_EXISTS:libevent::pthreads>
    $<TARGET_NAME_IF_EXISTS:USDT::headers>
)
if(WITH_EMBEDDED_ASMAP)
  target_compile_definitions(bitcoin_node PRIVATE ENABLE_EMBEDDED_ASMAP=1)
  include(TargetDataSources)
  target_raw_data_sources(bitcoin_node NAMESPACE node::data
    node/data/ip_asn.dat
  )
endif()

# Bitcoin wrapper executable that can call other executables.
if(BUILD_BITCOIN_BIN)
  add_executable(qtum bitcoin.cpp)
  add_windows_resources(qtum bitcoin-res.rc)
  add_windows_application_manifest(qtum)
  target_link_libraries(qtum core_interface bitcoin_common bitcoin_util)
  install_binary_component(qtum HAS_MANPAGE)
endif()

# Bitcoin Core bitcoind.
if(BUILD_DAEMON)
  add_executable(qtumd
    bitcoind.cpp
    init/bitcoind.cpp
  )
  add_windows_resources(qtumd bitcoind-res.rc)
  add_windows_application_manifest(qtumd)
  target_link_libraries(qtumd
    core_interface
    bitcoin_node
    $<TARGET_NAME_IF_EXISTS:bitcoin_wallet>
    Boost::headers
  )
  install_binary_component(qtumd HAS_MANPAGE)
endif()
if(ENABLE_IPC AND BUILD_DAEMON)
  add_executable(qtum-node
    bitcoind.cpp
    init/bitcoin-node.cpp
  )
  target_link_libraries(qtum-node
    core_interface
    bitcoin_node
    bitcoin_ipc
    $<TARGET_NAME_IF_EXISTS:bitcoin_wallet>
    Boost::headers
  )
  install_binary_component(qtum-node INTERNAL)
endif()


add_library(bitcoin_cli STATIC EXCLUDE_FROM_ALL
  compat/stdin.cpp
  rpc/client.cpp
)
target_link_libraries(bitcoin_cli
  PUBLIC
    core_interface
    univalue
)


# Bitcoin Core RPC client
if(BUILD_CLI)
  add_executable(qtum-cli bitcoin-cli.cpp)
  add_windows_resources(qtum-cli bitcoin-cli-res.rc)
  add_windows_application_manifest(qtum-cli)
  target_link_libraries(qtum-cli
    core_interface
    bitcoin_cli
    bitcoin_common
    bitcoin_util
    libevent::core
    libevent::extra
  )
  install_binary_component(qtum-cli HAS_MANPAGE)
endif()


if(BUILD_TX)
  add_executable(qtum-tx bitcoin-tx.cpp)
  add_windows_resources(qtum-tx bitcoin-tx-res.rc)
  add_windows_application_manifest(qtum-tx)
  target_link_libraries(qtum-tx
    core_interface
    bitcoin_common
    bitcoin_util
    univalue
    Boost::headers
  )
  install_binary_component(qtum-tx HAS_MANPAGE)
endif()


if(BUILD_UTIL)
  add_executable(qtum-util bitcoin-util.cpp)
  add_windows_resources(qtum-util bitcoin-util-res.rc)
  add_windows_application_manifest(qtum-util)
  target_link_libraries(qtum-util
    core_interface
    bitcoin_common
    bitcoin_util
    Boost::headers
  )
  install_binary_component(qtum-util HAS_MANPAGE)
endif()


if(BUILD_GUI)
  add_subdirectory(qt)
endif()


if(BUILD_KERNEL_LIB)
  add_subdirectory(kernel)
  if (BUILD_KERNEL_TEST)
    add_subdirectory(test/kernel)
  endif()
  if(BUILD_UTIL_CHAINSTATE)
    add_executable(qtum-chainstate
      bitcoin-chainstate.cpp
    )
    add_windows_application_manifest(bitcoin-chainstate)
    target_link_libraries(qtum-chainstate
      PRIVATE
        core_interface
        bitcoinkernel
    )
    install_binary_component(qtum-chainstate INTERNAL)
  endif()
endif()


add_subdirectory(test/util)
if(BUILD_BENCH)
  add_subdirectory(bench)
endif()

if(BUILD_TESTS)
  add_subdirectory(test)
endif()

if(BUILD_FUZZ_BINARY)
  add_subdirectory(test/fuzz)
endif()
