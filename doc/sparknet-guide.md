# Qtum Sparknet Usage and Information

Qtum is a decentralized blockchain project built on Bitcoin's UTXO model, but with support for Ethereum Virtual Machine based smart contracts. It achieves this through the revolutionary Account Abstraction Layer. For more general information about Qtum as well as links to join our community, go to https://qtum.org

Welcome to Qtum Sparknet, the first public testnet for the Qtum blockchain. Sparknet is designed primarily for developers, and as such documentation at this point will be technical and suited more for developers. The mainnet is expected to be released in September and will be suited for the general public. Testnet tokens do not hold any value and should not be traded for any monetary instruments. The testnet can be reset or forked at anytime as deemed necessary for development. Sparknet does not include support for Mutualized Proof Of Stake, or for the Decentralized Governance Protocol. Both of these features are implemented, and their code is available on alternative branches (check the pull requests), but have not been tested and proven stable enough to include in this testnet. They will be implemented in the 2nd public testnet for Qtum. 

# Using Smart Contracts with Qtum

The smart contract interface in Qtum still requires some technical knowledge. The GUI is not completed yet, so all smart contract interation must happen either using `qtum-cli` at the command line, or in the debug window of `qtum-qt`. 

To demonstrate how to deploy and interact with a simple we will use this contract:

    pragma solidity ^0.4.0;

    contract QtumTest {
       uint storedNumber;
       function QtumTest() {
           storedNumber=1;
       }
       function setNumber(uint number) public{
           storedNumber = number;
       }
       function logNumber() constant public{
            log1("storedNumber", uintToBytes(storedNumber));
       }
       function returnNumber() constant public returns (uint){
           return storedNumber;
       }
       function deposit() public payable{
       }
       function withdraw() public{
           if(!msg.sender.send(this.balance)){
               throw;
           }
       }
       //utility function
       function uintToBytes(uint v) constant returns (bytes32 ret) {
           if (v == 0) {
               ret = '0';
           }
           else {
               while (v > 0) {
                   ret = bytes32(uint(ret) / (2 ** 8));
                   ret |= bytes32(((v % 10) + 48) * 2 ** (8 * 31));
                   v /= 10;
               }
           }
           return ret;
       }
    }

It compiles to the following EVM bytecode 

    6060604052341561000c57fe5b5b60016000819055505b5b6102bd806100266000396000f30060606040523615610076576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680633450bd6a146100785780633ccfd60b1461009e5780633fb5c1cb146100b057806394e8767d146100d05780639f2c436f1461010c578063d0e30db01461011e575bfe5b341561008057fe5b610088610128565b6040518082815260200191505060405180910390f35b34156100a657fe5b6100ae610133565b005b34156100b857fe5b6100ce6004808035906020019091905050610190565b005b34156100d857fe5b6100ee600480803590602001909190505061019b565b60405180826000191660001916815260200191505060405180910390f35b341561011457fe5b61011c610246565b005b61012661028e565b005b600060005490505b90565b3373ffffffffffffffffffffffffffffffffffffffff166108fc3073ffffffffffffffffffffffffffffffffffffffff16319081150290604051809050600060405180830381858888f19350505050151561018d57610000565b5b565b806000819055505b50565b600060008214156101ce577f3000000000000000000000000000000000000000000000000000000000000000905061023d565b5b600082111561023c5761010081600190048115156101e957fe5b0460010290507f01000000000000000000000000000000000000000000000000000000000000006030600a8481151561021e57fe5b06010260010281179050600a8281151561023457fe5b0491506101cf565b5b8090505b919050565b61025160005461019b565b6000191660405180807f73746f7265644e756d6265720000000000000000000000000000000000000000815250600c01905060405180910390a15b565b5b5600a165627a7a72305820326efcd34df5fdba07e7a1afe7ffd4b42873ef749ae9a5915db46fd20b9c251c0029

And finally, has the following JSON interface file:

    [{"constant":true,"inputs":[],"name":"returnNumber","outputs":[{"name":"","type":"uint256"}],"payable":false,"type":"function"},{"constant":false,"inputs":[],"name":"withdraw","outputs":[],"payable":false,"type":"function"},{"constant":false,"inputs":[{"name":"number","type":"uint256"}],"name":"setNumber","outputs":[],"payable":false,"type":"function"},{"constant":true,"inputs":[{"name":"v","type":"uint256"}],"name":"uintToBytes","outputs":[{"name":"ret","type":"bytes32"}],"payable":false,"type":"function"},{"constant":true,"inputs":[],"name":"logNumber","outputs":[],"payable":false,"type":"function"},{"constant":false,"inputs":[],"name":"deposit","outputs":[],"payable":true,"type":"function"},{"inputs":[],"payable":false,"type":"constructor"}]

This info can easily be retrieved for any contract by using [Browser Solidity](https://ethereum.github.io/browser-solidity/), inputing your contract's source code, and then on the right hand side clicking "contract details" 

(note, if using the debug window in the Qtum Qt application, don't include `./qtum-cli` in the commands)

First, we need to deploy the contract:

    ./qtum-cli createcontract 6060604052341561000c57fe5b5b60016000819055505b5b6102bd806100266000396000f30060606040523615610076576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680633450bd6a146100785780633ccfd60b1461009e5780633fb5c1cb146100b057806394e8767d146100d05780639f2c436f1461010c578063d0e30db01461011e575bfe5b341561008057fe5b610088610128565b6040518082815260200191505060405180910390f35b34156100a657fe5b6100ae610133565b005b34156100b857fe5b6100ce6004808035906020019091905050610190565b005b34156100d857fe5b6100ee600480803590602001909190505061019b565b60405180826000191660001916815260200191505060405180910390f35b341561011457fe5b61011c610246565b005b61012661028e565b005b600060005490505b90565b3373ffffffffffffffffffffffffffffffffffffffff166108fc3073ffffffffffffffffffffffffffffffffffffffff16319081150290604051809050600060405180830381858888f19350505050151561018d57610000565b5b565b806000819055505b50565b600060008214156101ce577f3000000000000000000000000000000000000000000000000000000000000000905061023d565b5b600082111561023c5761010081600190048115156101e957fe5b0460010290507f01000000000000000000000000000000000000000000000000000000000000006030600a8481151561021e57fe5b06010260010281179050600a8281151561023457fe5b0491506101cf565b5b8090505b919050565b61025160005461019b565b6000191660405180807f73746f7265644e756d6265720000000000000000000000000000000000000000815250600c01905060405180910390a15b565b5b5600a165627a7a72305820326efcd34df5fdba07e7a1afe7ffd4b42873ef749ae9a5915db46fd20b9c251c0029 300000

Note that the last number is the gas limit for this transaction. The default value is not large enough for this contract, so we increase it to 300,000 gas.

This should result in something like so:

    {
      "txid": "72b0e0576d289c1e4e6c777431e4845f77d0884d3b3cff0387a5f4a1a3a874ea",
      "sender": "qZbjaE8N18ZU1m7851G7QGhvxKL74SRBTt",
      "hash160": "aff3e34ab836edb8d214a993d9da105915e4a6e9",
      "address": "5bde092dbecb84ea1a229b4c5b25dfc9cdc674d9"
    }


Now, you should store the `address` in a variable so it's easy to track:

    export CONTRACT=5bde092dbecb84ea1a229b4c5b25dfc9cdc674d9

Now wait for your contract to be included in a block. You should be able to confirm it made it into a block by using:

    ./qtum-cli getaccountinfo $CONTRACT

If you get a message saying "Address does not exist", then either your transaction has not yet been included in a block (you can confirm this with `getrawtransaction` and your txid), or you did not provide enough gas for the contract to be executed and persisted into the blockchain. If the contract was successfully executed and persisted in the blockchain, you should see something like this:

    {
      "address": "5bde092dbecb84ea1a229b4c5b25dfc9cdc674d9",
      "balance": 0,
      "storage": {
        "290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e563": {
          "0000000000000000000000000000000000000000000000000000000000000000": "0000000000000000000000000000000000000000000000000000000000000001"
        }
      },
      "code": "..."
    }


In order to interact with the contract, you must create raw ABI data from the interface JSON file. The easiest tool to assist in this is ethabi, https://github.com/paritytech/ethabi

Make sure the JSON file is saved somewhere, we will call it interface.json. 

In order to get the `storedNumber` variable we use the `returnNumber()` function. We can construct the ABI values by using ethabi:

    ethabi encode function ~/interface.json returnNumber

The result of this is:

    3450bd6a

Now, because we are not changing state, we use `callcontract`:

    ./qtum-cli callcontract $CONTRACT 3450bd6a

This results in a lot of data that can be useful in different contexts (including gas estimates), but we are only concerned about the `output` field, which is the value of `storedNumber`

    {
      "address": "5bde092dbecb84ea1a229b4c5b25dfc9cdc674d9",
      "executionResult": {
        "gasUsed": 21664,
        "excepted": "None",
        "newAddress": "5bde092dbecb84ea1a229b4c5b25dfc9cdc674d9",
        "output": "0000000000000000000000000000000000000000000000000000000000000001",
        "codeDeposit": 0,
        "gasRefunded": 0,
        "depositSize": 0,
        "gasForDeposit": 0
      },
      "transactionReceipt": {
        "stateRoot": "ffbeb0377d43c6ed443a2840259ff5ead5158016ab54d55ef21b7b11aa71947f",
        "gasUsed": 21664,
        "bloom": "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        "log": [
        ]
      }
    }

To change the storedNumber we can do an on-chain execution using `sendtocontract`. First, we need the ABI data:

    ethabi encode function ~/interface.json setNumber -p 123456 --lenient
    3fb5c1cb000000000000000000000000000000000000000000000000000000000001e240

Note we use --lenient so that we do not need to provide a full 256bit value as a parameter. Now, we can execute the contract directly:

    ./qtum-cli sendtocontract $CONTRACT 3fb5c1cb000000000000000000000000000000000000000000000000000000000001e240

Afterwards, we can call `returnNumber()` again and check the `output` field:

    "output": "000000000000000000000000000000000000000000000000000000000001e240",

This is 123456 encoded as hex. 

You can also use the `logNumber()` function in order to generate logs. If your node was started with `-record-log-opcodes`, then the file `vmExecLogs.json` will contain any log operations that occur on the blockchain. This is what is used for events on the Ethereum blockchain, and eventually it is our intention to bring similar functionality to Qtum.

You can also deposit and withdraw coins from this test contract using the `deposit()` and `withdraw()` functions.

The ABI value for `deposit` is d0e30db0 and the ABI value for `withdraw` is 3ccfd60b

This will send 10 tokens to the contract:

    ./qtum-cli sendtocontract $CONTRACT d0e30db0 10

And then, to withdraw them it's also very simple:

    ./qtum-cli sendtocontract $CONTRACT 3ccfd60b

If you want to control the exact address that the contract sends coins to, you can also explicitly specify the sender. Note that you must be capable of sending coins from that address (you can't use someone elses address). One of my wallet addresses is qZbjaE8N18ZU1m7851G7QGhvxKL74SRBTt, so I'll use that:

    ./qtum-cli sendtocontract $CONTRACT 3ccfd60b 0 190000 0.0000001 qZbjaE8N18ZU1m7851G7QGhvxKL74SRBTt

Note that if you get the error "Sender address does not have any unspent outputs", then you should send some coins to that address (they must be spent in order to prove that you own that address). This can be accomplished with any amount of coins:

    ./qtum-cli sendtoaddress qZbjaE8N18ZU1m7851G7QGhvxKL74SRBTt 0.001

There is no need to wait for this transaction to confirm, it can be followed immediately by the sendtocontract command:

    ./qtum-cli sendtocontract $CONTRACT 3ccfd60b 0 190000 0.0000001 qZbjaE8N18ZU1m7851G7QGhvxKL74SRBTt

When creating this contract transaction, nothing will immediately happen, when the transaction is put into a block though a new transaction will appear in a block which will send any coins owned by the contract to the pubkeyhash address qZbjaE8N18ZU1m7851G7QGhvxKL74SRBTt

## FAQ

* Q: "I used `createcontract`, but can't call my contract and it's not in listcontract" A: You probably did not provide enough gas for the contract's constructor to be executed and it's code persisted in the blockchain. The vm.log file should confirm this by saying how much gas was needed
* Q: "I sent a large amount of gas but I never got a refund" A: Refunds are generated from the coinstake transaction, so you must wait 500 blocks for the gas refund to mature before it can be spent again
* Q: "I used -reindex and now my node is taking forever to resync" A: Currently when doing a reindex, all contracts are reprocessed, so in a chain with many contract executions this can add up to a significant amount of time. This will be made faster in the future, as well as the initial syncing speed of nodes
* Q: "I think I found a bug in Qtum" A: Please report any bugs at https://github.com/qtumproject/qtum/issues



# New Qtum RPC Commands

Qtum supports all of the RPC commands supported by Bitcoin Core, but also includes the following commands unique to Qtum:

* `createcontract` - This will create and deploy a new smart contract to the Qtum blockchain. This requires gas.
* `callcontract` - This will interact with an already deployed smart contract on the Qtum blockchain, with all computation taking place off-chain and no persistence to the blockchain. This does not require gas
* `sendtocontract` - This will interact with an already deployed smart contract on the Qtum blockchain. All computation takes place on-chain and any state changes will be persisted to the blockchain. This allows tokens to be sent to a smart contract. This requires gas.
* `getaccountinfo` - This will show some low level information about a contract, including the contract's bytecode, stored data, and balance on the blockchain.
* `listcontracts` - This will output a list of currently deployed contract addresses with their respective balance. This RPC call may change or be removed in the future.
* `reservebalance` - This will reserve a set amount of coins so that they do not participate in staking. If you reserve as many or more coins than are in your wallet, then you will not participate at all in staking and block creation for the network.
* `getstakinginfo` - This will show some info about the current node's staking status, including network difficulty and expected time (in seconds) until staking a new block.
* `gethexaddress` - This will convert a standard Base58 pubkeyhash address to a hex address for use in smart contracts
* `fromhexaddress` - this will convert a hex address used in smart contracts to a standard Base58 pubkeyhash address


# New Qtum Command Line Arguments

Qtum supports all of the usual command line arguments that Bitcoin Core supports. In addition it adds the following new command line arguments:

* `-record-log-opcodes` - This will create a new log file in the Qtum data directory (usually ~/.qtum) named vmExecLogs.json, where any EVM LOG opcode is logged along with topics and data that the contract requested be logged. 

# Untested features

Some features included in Bitcoin Core have not been tested in it's porting to Qtum. This includes:

* Pruning

# EVM Smart Contract Changes and Limitations

Because of Qtum's underlying technical differences, there are a few operations that can have different results or limitations when executed in Qtum than when compared to Ethereum. 

These include the following, though there may be others introduced in the future:  

* The gas schedule for Qtum is different from Ethereum. Certain operations are more or less expensive. As such, gas cost estimators designed for Ethereum will not give accurate results for Qtum. We will develop our own gas estimating tools as well as fully documenting these differences at a later date. 
* `block.coinbase` or the `COINBASE` opcode currently is not supported and will only return 0. When MPoS is released in the 2nd testnet this should be functioning as expected
* `block.number` will return the previous block height before this block containing the contract's execution
* `block.difficulty` will return the previous block's difficulty
* `block.timestamp will return the previous block's timestamp
* `block.blockhash(n)` will return 0 when n is the current block height (`block.number+1`), similar to Ethereum
* `sender` will return 0 when the coins spent (`vin[0].prevout`) are from a non-standard transaction. It will return the pubkeyhash 160bit address when spent from a pubkey or pubkeyhash transaction
* Coins can be sent to either contracts or pubkeyhash addresses. When coins are sent to a non-existent contract address, the coins will automatically be sent to a pubkeyhash address instead.
* Only 1000 vouts can be generated from a single contract execution. Sending coins to the same contract multiple times results in a single vout being created, so the limitation is effectively that coins can only be sent to up to 1000 unique contract or pubkeyhash addresses, including balance changes between contracts. If this limit is exceeded, an Out Of Gas exception is generated and all state changes are reverted. 
* Contract executions can not happen within coinbase or coinstake transactions

Additional documents for the overall design and expected results of various operations is available at the ITD repository here: https://github.com/qtumproject/qtum-itds


