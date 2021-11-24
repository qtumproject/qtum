HARDWARE WALLET
====================

## Tools for hardware device support

Use [Ledger Nano S Loader](https://github.com/qtumproject/qtum-ledger-loader/releases) to install the Ledger Nano S Wallet and Ledger Nano S Stake application.

Use [HWI](https://github.com/qtumproject/HWI) for command line interaction with the Hardware Wallet.

## Graphical interface for hardware device

`qtum-qt` provides an interface for interacting with hardware wallet devices.

Set the HWI tool path using the the menu `Settings -> Option -> Main -> HWI Tool Path` and restart `qtum-qt`, the tool is needed for hardware wallet interaction.

Use the menu `File -> Create Wallet... -> Use a hardware device` for creating hardware wallet. The hardware wallet needs to be connected and the wallet application started.

Use hardware wallets to send/receive coins.

Ledger Nano S has support for smart contracts using the wallet application that can be installed with [Ledger Nano S Loader](https://github.com/qtumproject/qtum-ledger-loader/releases), it also supports delegation to a staker for offline staking.

## Graphical interface for hardware device staking

Ledger Nano S has support for staking using the staking application that can be installed with [Ledger Nano S Loader](https://github.com/qtumproject/qtum-ledger-loader/releases).

Using the menu `Settings -> Option -> Main -> Select Ledger device for staking` to select ledger for staking that the `qtum-qt` will automatically connect when started.

The staking will be active until the application is closed and will be automatically started when `qtum-qt` is started and the staking wallet is loaded.

## Command line interface for hardware device staking

`qtumd -hwitoolpath=<HWI Tool Path> -stakerledgerid=<Ledger device for staking> -wallet <Hardware wallet>`

`<HWI Tool Path>` is the location where the HWI is installed. In GUI, the value in menu `Settings -> Option -> Main -> HWI Tool Path`.

`<Ledger device for staking>` is the ledger fingerprint that will be used for staking. In GUI, the value in menu `Settings -> Option -> Main -> Select Ledger device for staking`. you can also get the fingerprint for the device by running `./hwi.py enumerate` from the command line in the HWI folder.

`<Hardware wallet>` is the name of the hardware device wallet that was created.

