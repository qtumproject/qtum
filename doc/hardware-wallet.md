HARDWARE WALLET
====================

## Tools for hardware device

Using the [NanoS Loader](https://github.com/qtumproject/qtum-ledger-loader/releases) can be installed the NanoS Wallet and NanoS Stake application.

Using the [HWI](https://github.com/qtumproject/HWI) for command line interaction with the Hardware Wallet, for installation instructions check the [HWI](https://github.com/qtumproject/HWI) repository.

## Graphical interface for hardware device

`qtum-qt` provide the hardware interface for working with Hardware Device like Ledger.

Set the HWI tool path using the the menu `Settings -> Option -> Main -> HWI Tool Path` and restart the `qtum-qt`, the tool is needed for hardware interaction.

Using the menu `File -> Create Wallet... -> Use a hardware device` for creating hardware wallet. The ledger need to be connected and the wallet application started.

Using the hardware wallet to send/receive coins. For sending coins the ledger need to be connected and the transaction confirmed on it.

Ledger NanoS has support for smart contracts using the wallet application that can be installed with [NanoS Loader](https://github.com/qtumproject/qtum-ledger-loader/releases), the coins can be delegated to a staker for offline staking.

## Graphical interface for hardware device staking

Ledger NanoS has support for staking using the staking application that can be installed with [NanoS Loader](https://github.com/qtumproject/qtum-ledger-loader/releases).
Start `qtum-qt`, open the hardware wallet, click the staking button from the navigation bar and select the ledger from the list.
The staking will be active until the application is closed and will not be automatically started when `qtum-qt` is started.

Using the menu `Settings -> Option -> Main -> Select Ledger device for staking` to select ledger for staking that the `qtum-qt` will automatically connect when started.

## Command line interface for hardware device staking

`qtumd -hwitoolpath=<HWI Tool Path> -stakerledgerid=<Ledger device for staking> -wallet <Hardware wallet>`

`<HWI Tool Path>` is the location where the HWI is installed. In GUI, the value in menu `Settings -> Option -> Main -> HWI Tool Path`.

`<Ledger device for staking>` is the ledger fingerprint that will be used for staking. In GUI, the value in menu `Settings -> Option -> Main -> Select Ledger device for staking`.

`<Hardware wallet>` is the name of the hardware wallet.

