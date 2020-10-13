0.20.1 Release Notes
====================

Bitcoin Core version 0.20.1 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-0.20.1/>

This minor release includes various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on Mac)
or `bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of Bitcoin Core are generally supported.

Compatibility
==============

Bitcoin Core is supported and extensively tested on operating systems
using the Linux kernel, macOS 10.12+, and Windows 7 and newer.  Bitcoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them.  It is not recommended to use Bitcoin Core on
unsupported systems.

From Bitcoin Core 0.20.0 onwards, macOS versions earlier than 10.12 are no
longer supported. Additionally, Bitcoin Core does not yet change appearance
when macOS "dark mode" is activated.

Known Bugs
==========

The process for generating the source code release ("tarball") has changed in an
effort to make it more complete, however, there are a few regressions in
this release:

- The generated `configure` script is currently missing, and you will need to
  install autotools and run `./autogen.sh` before you can run
  `./configure`. This is the same as when checking out from git.

- Instead of running `make` simply, you should instead run
  `BITCOIN_GENBUILD_NO_GIT=1 make`.

Notable changes
===============

Changes regarding misbehaving peers
-----------------------------------

Peers that misbehave (e.g. send us invalid blocks) are now referred to as
discouraged nodes in log output, as they're not (and weren't) strictly banned:
incoming connections are still allowed from them, but they're preferred for
eviction.

Furthermore, a few additional changes are introduced to how discouraged
addresses are treated:

- Discouraging an address does not time out automatically after 24 hours
  (or the `-bantime` setting). Depending on traffic from other peers,
  discouragement may time out at an indeterminate time.

- Discouragement is not persisted over restarts.

- There is no method to list discouraged addresses. They are not returned by
  the `listbanned` RPC. That RPC also no longer reports the `ban_reason`
  field, as `"manually added"` is the only remaining option.

- Discouragement cannot be removed with the `setban remove` RPC command.
  If you need to remove a discouragement, you can remove all discouragements by
  stop-starting your node.

Notification changes
--------------------

`-walletnotify` notifications are now sent for wallet transactions that are
removed from the mempool because they conflict with a new block. These
notifications were sent previously before the v0.19 release, but had been
broken since that release (bug
[#18325](https://github.com/bitcoin/bitcoin/issues/18325)).

PSBT changes
------------

PSBTs will contain both the non-witness utxo and the witness utxo for segwit
inputs in order to restore compatibility with wallet software that are now
requiring the full previous transaction for segwit inputs. The witness utxo
is still provided to maintain compatibility with software which relied on its
existence to determine whether an input was segwit.

- The `walletprocesspsbt` and `walletcreatefundedpsbt` RPCs now include
  BIP32 derivation paths by default for public keys if we know them.
  This can be disabled by setting the `bip32derivs` parameter to
  `false`.  (#17264)

- The `bumpfee` RPC's parameter `totalFee`, which was deprecated in
  0.19, has been removed.  (#18312)

- The `bumpfee` RPC will return a PSBT when used with wallets that have
  private keys disabled.  (#16373)

- The `getpeerinfo` RPC now includes a `mapped_as` field to indicate the
  mapped Autonomous System used for diversifying peer selection. See the
  `-asmap` configuration option described below in _New Settings_.  (#16702)

- The `createmultisig` and `addmultisigaddress` RPCs now return an
  output script descriptor for the newly created address.  (#18032)

Build System
------------

- OpenSSL is no longer used by Bitcoin Core.  (#17265)

- BIP70 support has been fully removed from Bitcoin Core. The
  `--enable-bip70` option remains, but it will throw an error during configure.
  (#17165)

- glibc 2.17 or greater is now required to run the release binaries. This
  retains compatibility with RHEL 7, CentOS 7, Debian 8 and Ubuntu 14.04 LTS. (#17538)

- The source code archives that are provided with gitian builds no longer contain
  any autotools artifacts. Therefore, to build from such source, a user
  should run the `./autogen.sh` script from the root of the unpacked archive.
  This implies that `autotools` and other required packages are installed on the
  user's system. (#18331)

New settings
------------

- New `rpcwhitelist` and `rpcwhitelistdefault` configuration parameters
  allow giving certain RPC users permissions to only some RPC calls.
  (#12763)

- A new `-asmap` configuration option has been added to diversify the
  node's network connections by mapping IP addresses Autonomous System
  Numbers (ASNs) and then limiting the number of connections made to any
  single ASN.  See [issue #16599](https://github.com/bitcoin/bitcoin/issues/16599),
  [PR #16702](https://github.com/bitcoin/bitcoin/pull/16702), and the
  `bitcoind help` for more information.  This option is experimental and
  subject to removal or breaking changes in future releases, so the
  legacy /16 prefix mapping of IP addresses remains the default.  (#16702)

Updated settings
----------------

- All custom settings configured when Bitcoin Core starts are now
  written to the `debug.log` file to assist troubleshooting.  (#16115)

- Importing blocks upon startup via the `bootstrap.dat` file no longer
  occurs by default. The file must now be specified with
  `-loadblock=<file>`.  (#17044)

- The `-debug=db` logging category has been renamed to
  `-debug=walletdb` to distinguish it from `coindb`.  The `-debug=db`
  option has been deprecated and will be removed in the next major
  release.  (#17410)

- The `-walletnotify` configuration parameter will now replace any `%w`
  in its argument with the name of the wallet generating the
  notification.  This is not supported on Windows. (#13339)

Removed settings
----------------

- The `-whitelistforcerelay` configuration parameter has been removed after
  it was discovered that it was rendered ineffective in version 0.13 and
  hasn't actually been supported for almost four years.  (#17985)

GUI changes
-----------

- The "Start Bitcoin Core on system login" option has been removed on macOS.
  (#17567)

- In the Peers window, the details for a peer now displays a `Mapped AS`
  field to indicate the mapped Autonomous System used for diversifying
  peer selection. See the `-asmap` configuration option in _New
  Settings_, above.  (#18402)

- A "known bug" [announced](https://bitcoincore.org/en/releases/0.18.0/#wallet-gui)
  in the release notes of version 0.18 has been fixed.  The issue
  affected anyone who simultaneously used multiple Bitcoin Core wallets
  and the GUI coin control feature. (#18894)

- For watch-only wallets, creating a new transaction in the Send screen
  or fee bumping an existing transaction in the Transactions screen will
  automatically copy a Partially-Signed Bitcoin Transaction (PSBT) to
  the system clipboard.  This can then be pasted into an external
  program such as [HWI](https://github.com/bitcoin-core/HWI) for
  signing.  Future versions of Bitcoin Core should support a GUI option
  for finalizing and broadcasting PSBTs, but for now the debug console
  may be used with the `finalizepsbt` and `sendrawtransaction` RPCs.
  (#16944, #17492)

Wallet
------

- The wallet now by default uses bech32 addresses when using RPC, and
  creates native segwit change outputs.  (#16884)

- The way that output trust was computed has been fixed, which affects
  confirmed/unconfirmed balance status and coin selection.  (#16766)

- The `gettransaction`, `listtransactions` and `listsinceblock` RPC
  responses now also include the height of the block that contains the
  wallet transaction, if any.  (#17437)

- The `getaddressinfo` RPC has had its `label` field deprecated
  (re-enable for this release using the configuration parameter
  `-deprecatedrpc=label`).  The `labels` field is altered from returning
  JSON objects to returning a JSON array of label names (re-enable
  previous behavior for this release using the configuration parameter
  `-deprecatedrpc=labelspurpose`).  Backwards compatibility using the
  deprecated configuration parameters is expected to be dropped in the
  0.21 release.  (#17585, #17578)

Documentation changes
---------------------

- Bitcoin Core's automatically-generated source code documentation is
  now available at https://doxygen.bitcoincore.org.  (#17596)

Low-level changes
=================

Utilities
---------

- The `bitcoin-cli` utility used with the `-getinfo` parameter now
  returns a `headers` field with the number of downloaded block headers
  on the best headers chain (similar to the `blocks` field that is also
  returned) and a `verificationprogress` field that estimates how much
  of the best block chain has been synced by the local node.  The
  information returned no longer includes the `protocolversion`,
  `walletversion`, and `keypoololdest` fields.  (#17302, #17650)

- The `bitcoin-cli` utility now accepts a `-stdinwalletpassphrase`
  parameter that can be used when calling the `walletpassphrase` and
  `walletpassphrasechange` RPCs to read the passphrase from standard
  input without echoing it to the terminal, improving security against
  anyone who can look at your screen.  The existing `-stdinrpcpass`
  parameter is also updated to not echo the passphrase. (#13716)

Command line
------------

- Command line options prefixed with main/test/regtest network names like
  `-main.port=8333` `-test.server=1` previously were allowed but ignored. Now
  they trigger "Invalid parameter" errors on startup. (#17482)

New RPCs
--------

- The `dumptxoutset` RPC outputs a serialized snapshot of the current
  UTXO set.  A script is provided in the `contrib/devtools` directory
  for generating a snapshot of the UTXO set at a particular block
  height.  (#16899)

- The `generatetodescriptor` RPC allows testers using regtest mode to
  generate blocks that pay an arbitrary output script descriptor.
  (#16943)

Updated RPCs
------------

- The `verifychain` RPC default values are now static instead of
  depending on the command line options or configuration file
  (`-checklevel`, and `-checkblocks`). Users can pass in the RPC
  arguments explicitly when they don't want to rely on the default
  values. (#18541)

- The `getblockchaininfo` RPC's `verificationprogress` field will no
  longer report values higher than 1.  Previously it would occasionally
  report the chain was more than 100% verified.  (#17328)

Tests
-----

- It is now an error to use an unqualified `walletdir=path` setting in
  the config file if running on testnet or regtest networks. The setting
  now needs to be qualified as `chain.walletdir=path` or placed in the
  appropriate `[chain]` section. (#17447)

- `-fallbackfee` was 0 (disabled) by default for the main chain, but
  0.0002 by default for the test chains. Now it is 0 by default for all
  chains. Testnet and regtest users will have to add
  `fallbackfee=0.0002` to their configuration if they weren't setting it
  and they want it to keep working like before. (#16524)

Build system
------------

- Support is provided for building with the Android Native Development
  Kit (NDK).  (#16110)

Credits
=======

Thanks to everyone who directly contributed to this release:

(todo)
As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoin/bitcoin/).
