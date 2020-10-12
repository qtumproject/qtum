// Copyright (c) 2018-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_WALLET_H
#define BITCOIN_INTERFACES_WALLET_H

#include <amount.h>                    // For CAmount
#include <pubkey.h>                    // For CKeyID and CScriptID (definitions needed in CTxDestination instantiation)
#include <script/standard.h>           // For CTxDestination
#include <support/allocators/secure.h> // For SecureString
#include <ui_interface.h>              // For ChangeType
#include <util/message.h>

#include <functional>
#include <map>
#include <memory>
#include <psbt.h>
#include <stdint.h>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

class CCoinControl;
class CFeeRate;
class CKey;
class CWallet;
enum isminetype : unsigned int;
enum class FeeReason;
typedef uint8_t isminefilter;

enum class OutputType;
struct CRecipient;

namespace interfaces {

class Handler;
struct WalletAddress;
struct WalletBalances;
struct WalletTx;
struct WalletTxOut;
struct WalletTxStatus;
struct TokenInfo;
struct TokenTx;
struct ContractBookData;
struct DelegationInfo;
struct DelegationDetails;
struct SuperStakerInfo;
struct DelegationStakerInfo;


using WalletOrderForm = std::vector<std::pair<std::string, std::string>>;
using WalletValueMap = std::map<std::string, std::string>;

//! Interface for accessing a wallet.
class Wallet
{
public:
    virtual ~Wallet() {}

    //! Encrypt wallet.
    virtual bool encryptWallet(const SecureString& wallet_passphrase) = 0;

    //! Return whether wallet is encrypted.
    virtual bool isCrypted() = 0;

    //! Lock wallet.
    virtual bool lock() = 0;

    //! Unlock wallet.
    virtual bool unlock(const SecureString& wallet_passphrase) = 0;

    //! Return whether wallet is locked.
    virtual bool isLocked() = 0;

    //! Change wallet passphrase.
    virtual bool changeWalletPassphrase(const SecureString& old_wallet_passphrase,
        const SecureString& new_wallet_passphrase) = 0;

    //! Abort a rescan.
    virtual void abortRescan() = 0;

    //! Back up wallet.
    virtual bool backupWallet(const std::string& filename) = 0;

    //! Get wallet name.
    virtual std::string getWalletName() = 0;

    // Get a new address.
    virtual bool getNewDestination(const OutputType type, const std::string label, CTxDestination& dest) = 0;

    //! Get public key.
    virtual bool getPubKey(const CScript& script, const CKeyID& address, CPubKey& pub_key) = 0;

    //! Sign message
    virtual SigningResult signMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) = 0;

    //! Return whether wallet has private key.
    virtual bool isSpendable(const CTxDestination& dest) = 0;

    //! Return whether wallet has watch only keys.
    virtual bool haveWatchOnly() = 0;

    //! Add or update address.
    virtual bool setAddressBook(const CTxDestination& dest, const std::string& name, const std::string& purpose) = 0;

    // Remove address.
    virtual bool delAddressBook(const CTxDestination& dest) = 0;

    //! Look up address in wallet, return whether exists.
    virtual bool getAddress(const CTxDestination& dest,
        std::string* name,
        isminetype* is_mine,
        std::string* purpose) = 0;

    //! Get wallet address list.
    virtual std::vector<WalletAddress> getAddresses() = 0;

    //! Add dest data.
    virtual bool addDestData(const CTxDestination& dest, const std::string& key, const std::string& value) = 0;

    //! Erase dest data.
    virtual bool eraseDestData(const CTxDestination& dest, const std::string& key) = 0;

    //! Get dest values with prefix.
    virtual std::vector<std::string> getDestValues(const std::string& prefix) = 0;

    //! Lock coin.
    virtual void lockCoin(const COutPoint& output) = 0;

    //! Unlock coin.
    virtual void unlockCoin(const COutPoint& output) = 0;

    //! Return whether coin is locked.
    virtual bool isLockedCoin(const COutPoint& output) = 0;

    //! List locked coins.
    virtual void listLockedCoins(std::vector<COutPoint>& outputs) = 0;

    //! Create transaction.
    virtual CTransactionRef createTransaction(const std::vector<CRecipient>& recipients,
        const CCoinControl& coin_control,
        bool sign,
        int& change_pos,
        CAmount& fee,
        std::string& fail_reason) = 0;

    //! Commit transaction.
    virtual void commitTransaction(CTransactionRef tx,
        WalletValueMap value_map,
        WalletOrderForm order_form) = 0;

    //! Return whether transaction can be abandoned.
    virtual bool transactionCanBeAbandoned(const uint256& txid) = 0;

    //! Abandon transaction.
    virtual bool abandonTransaction(const uint256& txid) = 0;

    //! Return whether transaction can be bumped.
    virtual bool transactionCanBeBumped(const uint256& txid) = 0;

    //! Create bump transaction.
    virtual bool createBumpTransaction(const uint256& txid,
        const CCoinControl& coin_control,
        std::vector<std::string>& errors,
        CAmount& old_fee,
        CAmount& new_fee,
        CMutableTransaction& mtx) = 0;

    //! Sign bump transaction.
    virtual bool signBumpTransaction(CMutableTransaction& mtx) = 0;

    //! Commit bump transaction.
    virtual bool commitBumpTransaction(const uint256& txid,
        CMutableTransaction&& mtx,
        std::vector<std::string>& errors,
        uint256& bumped_txid) = 0;

    //! Get a transaction.
    virtual CTransactionRef getTx(const uint256& txid) = 0;

    //! Get transaction information.
    virtual WalletTx getWalletTx(const uint256& txid) = 0;

    //! Get list of all wallet transactions.
    virtual std::vector<WalletTx> getWalletTxs() = 0;

    //! Try to get updated status for a particular transaction, if possible without blocking.
    virtual bool tryGetTxStatus(const uint256& txid,
        WalletTxStatus& tx_status,
        int& num_blocks,
        int64_t& block_time) = 0;

    //! Get transaction details.
    virtual WalletTx getWalletTxDetails(const uint256& txid,
        WalletTxStatus& tx_status,
        WalletOrderForm& order_form,
        bool& in_mempool,
        int& num_blocks) = 0;

    //! Fill PSBT.
    virtual TransactionError fillPSBT(int sighash_type,
        bool sign,
        bool bip32derivs,
        PartiallySignedTransaction& psbtx,
        bool& complete) = 0;

    //! Get balances.
    virtual WalletBalances getBalances() = 0;

    //! Get balances if possible without waiting for chain and wallet locks.
    virtual bool tryGetBalances(WalletBalances& balances,
        int& num_blocks,
        bool force,
        int cached_num_blocks) = 0;

    //! Get balance.
    virtual CAmount getBalance() = 0;

    //! Get available balance.
    virtual CAmount getAvailableBalance(const CCoinControl& coin_control) = 0;

    //! Return whether transaction input belongs to wallet.
    virtual isminetype txinIsMine(const CTxIn& txin) = 0;

    //! Return whether transaction output belongs to wallet.
    virtual isminetype txoutIsMine(const CTxOut& txout) = 0;

    //! Return debit amount if transaction input belongs to wallet.
    virtual CAmount getDebit(const CTxIn& txin, isminefilter filter) = 0;

    //! Return credit amount if transaction input belongs to wallet.
    virtual CAmount getCredit(const CTxOut& txout, isminefilter filter) = 0;

    //! Check if address have unspent coins
    virtual bool isUnspentAddress(const std::string& address) = 0;

    //! Check if address is mine
    virtual bool isMineAddress(const std::string &strAddress) = 0;

    //! Try get available coins addresses
    virtual bool tryGetAvailableAddresses(std::vector<std::string> &spendableAddresses, std::vector<std::string> &allAddresses, bool &includeZeroValue) = 0;

    //! Return AvailableCoins + LockedCoins grouped by wallet address.
    //! (put change in one group with wallet address)
    using CoinsList = std::map<CTxDestination, std::vector<std::tuple<COutPoint, WalletTxOut>>>;
    virtual CoinsList listCoins() = 0;

    //! Return wallet transaction output information.
    virtual std::vector<WalletTxOut> getCoins(const std::vector<COutPoint>& outputs) = 0;

    //! Get required fee.
    virtual CAmount getRequiredFee(unsigned int tx_bytes) = 0;

    //! Get minimum fee.
    virtual CAmount getMinimumFee(unsigned int tx_bytes,
        const CCoinControl& coin_control,
        int* returned_target,
        FeeReason* reason) = 0;

    //! Get tx confirm target.
    virtual unsigned int getConfirmTarget() = 0;

    // Return whether HD enabled.
    virtual bool hdEnabled() = 0;

    // Return whether the wallet is blank.
    virtual bool canGetAddresses() = 0;

    // Return whether private keys enabled.
    virtual bool privateKeysDisabled() = 0;

    // Get default address type.
    virtual OutputType getDefaultAddressType() = 0;

    // Get default change type.
    virtual OutputType getDefaultChangeType() = 0;

    //! Get max tx fee.
    virtual CAmount getDefaultMaxTxFee() = 0;

    // Remove wallet.
    virtual void remove() = 0;

    //! Add wallet token entry.
    virtual bool addTokenEntry(const TokenInfo &token) = 0;

    //! Add wallet token transaction entry.
    virtual bool addTokenTxEntry(const TokenTx& tokenTx, bool fFlushOnClose=true) = 0;

    //! Check if exist wallet token entry.
    virtual bool existTokenEntry(const TokenInfo &token) = 0;

    //! Remove wallet token entry.
    virtual bool removeTokenEntry(const std::string &sHash) = 0;

    //! Get invalid wallet tokens
    virtual std::vector<TokenInfo> getInvalidTokens() = 0;

    //! Get token transaction information.
    virtual TokenTx getTokenTx(const uint256& txid) = 0;

    //! Get list of all wallet token transactions.
    virtual std::vector<TokenTx> getTokenTxs() = 0;

    //! Get token information.
    virtual TokenInfo getToken(const uint256& id) = 0;

    //! Get list of all tokens.
    virtual std::vector<TokenInfo> getTokens() = 0;

    //! Try to get updated status for a particular token transaction, if possible without blocking.
    virtual bool tryGetTokenTxStatus(const uint256& txid, int& block_number, bool& in_mempool, int& num_blocks) = 0;

    //! Get updated status for a particular token transaction.
    virtual bool getTokenTxStatus(const uint256& txid, int& block_number, bool& in_mempool, int& num_blocks) = 0;

    //! Get token transaction details
    virtual bool getTokenTxDetails(const TokenTx &wtx, uint256& credit, uint256& debit, std::string& tokenSymbol, uint8_t& decimals) = 0;

    //! Clean token transaction entries in the wallet
    virtual bool cleanTokenTxEntries() = 0;

    //! Check if token transaction is mine
    virtual bool isTokenTxMine(const TokenTx &wtx) = 0;

    //! Get contract book data.
    virtual ContractBookData getContractBook(const std::string& address) = 0;

    //! Get list of all contract book data.
    virtual std::vector<ContractBookData> getContractBooks() = 0;

    //! Check if exist contract book.
    virtual bool existContractBook(const std::string& address) = 0;

    //! Delete contract book data.
    virtual bool delContractBook(const std::string& address) = 0;

    //! Set contract book data.
    virtual bool setContractBook(const std::string& address, const std::string& name, const std::string& abi) = 0;

    //! Restore wallet delegations.
    virtual uint32_t restoreDelegations() = 0;

    //! Add wallet delegation entry.
    virtual bool addDelegationEntry(const DelegationInfo &delegation) = 0;

    //! Check if exist wallet delegation entry.
    virtual bool existDelegationEntry(const DelegationInfo &delegation) = 0;

    //! Get delegation information.
    virtual DelegationInfo getDelegation(const uint256& id) = 0;

    //! Get delegation information from contract.
    virtual DelegationInfo getDelegationContract(const std::string &sHash, bool& validated, bool& contractRet) = 0;

    //! Get delegation details for address.
    virtual DelegationDetails getDelegationDetails(const std::string &sAddress) = 0;

    //! Get list of all delegations.
    virtual std::vector<DelegationInfo> getDelegations() = 0;

    //! Remove wallet delegation entry.
    virtual bool removeDelegationEntry(const std::string &sHash) = 0;

    //! Set delegation entry removed.
    virtual bool setDelegationRemoved(const std::string &sHash, const std::string &sTxid) = 0;

    //! Restore wallet super stakers.
    virtual uint32_t restoreSuperStakers() = 0;

    //! Exist super staker.
    virtual bool existSuperStaker(const std::string &sAddress) = 0;

    //! Get super staker information.
    virtual SuperStakerInfo getSuperStaker(const uint256& id) = 0;

    //! Get super staker recommended config.
    virtual SuperStakerInfo getSuperStakerRecommendedConfig() = 0;

    //! Get list of all super stakers.
    virtual std::vector<SuperStakerInfo> getSuperStakers() = 0;

    //! Add wallet super staker entry.
    virtual bool addSuperStakerEntry(const SuperStakerInfo &superStaker) = 0;

    //! Remove wallet super staker entry.
    virtual bool removeSuperStakerEntry(const std::string &sHash) = 0;

    //! Get the super staker weight
    virtual uint64_t getSuperStakerWeight(const uint256& id) = 0;

    //! Is super staker staking
    virtual bool isSuperStakerStaking(const uint256& id, CAmount& delegationsWeight) = 0;

    //! Try get the stake weight
    virtual bool tryGetStakeWeight(uint64_t& nWeight) = 0;

    //! Get the stake weight
    virtual uint64_t getStakeWeight() = 0;

    //! Get last coin stake search interval
    virtual int64_t getLastCoinStakeSearchInterval() = 0;

    //! Get wallet unlock for staking only
    virtual bool getWalletUnlockStakingOnly() = 0;

    //! Set wallet unlock for staking only
    virtual void setWalletUnlockStakingOnly(bool unlock) = 0;

    //! Set wallet enabled for staking
    virtual void setEnabledStaking(bool enabled) = 0;

    //! Get wallet enabled for staking
    virtual bool getEnabledStaking() = 0;

    //! Get wallet enabled for super staking
    virtual bool getEnabledSuperStaking() = 0;

    //! Get a delegation from super staker.
    virtual DelegationStakerInfo getDelegationStaker(const uint160& id) = 0;

    //! Get list of all delegations for super stakers.
    virtual std::vector<DelegationStakerInfo> getDelegationsStakers() = 0;

    //! Get staker address balance.
    virtual bool getStakerAddressBalance(const std::string& staker, CAmount& balance, CAmount& stake, CAmount& weight) = 0;

    //! Register handler for unload message.
    using UnloadFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleUnload(UnloadFn fn) = 0;

    //! Register handler for show progress messages.
    using ShowProgressFn = std::function<void(const std::string& title, int progress)>;
    virtual std::unique_ptr<Handler> handleShowProgress(ShowProgressFn fn) = 0;

    //! Register handler for status changed messages.
    using StatusChangedFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleStatusChanged(StatusChangedFn fn) = 0;

    //! Register handler for address book changed messages.
    using AddressBookChangedFn = std::function<void(const CTxDestination& address,
        const std::string& label,
        bool is_mine,
        const std::string& purpose,
        ChangeType status)>;
    virtual std::unique_ptr<Handler> handleAddressBookChanged(AddressBookChangedFn fn) = 0;

    //! Register handler for transaction changed messages.
    using TransactionChangedFn = std::function<void(const uint256& txid, ChangeType status)>;
    virtual std::unique_ptr<Handler> handleTransactionChanged(TransactionChangedFn fn) = 0;

    //! Register handler for token transaction changed messages.
    using TokenTransactionChangedFn = std::function<void(const uint256& id, ChangeType status)>;
    virtual std::unique_ptr<Handler> handleTokenTransactionChanged(TokenTransactionChangedFn fn) = 0;

    //! Register handler for token changed messages.
    using TokenChangedFn = std::function<void(const uint256& id, ChangeType status)>;
    virtual std::unique_ptr<Handler> handleTokenChanged(TokenChangedFn fn) = 0;

    //! Register handler for watchonly changed messages.
    using WatchOnlyChangedFn = std::function<void(bool have_watch_only)>;
    virtual std::unique_ptr<Handler> handleWatchOnlyChanged(WatchOnlyChangedFn fn) = 0;

    //! Register handler for keypool changed messages.
    using CanGetAddressesChangedFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleCanGetAddressesChanged(CanGetAddressesChangedFn fn) = 0;

    //! Register handler for contract book changed messages.
    using ContractBookChangedFn = std::function<void( const std::string& address,
        const std::string& label,
        const std::string& abi,
        ChangeType status)>;
    virtual std::unique_ptr<Handler> handleContractBookChanged(ContractBookChangedFn fn) = 0;

    //! Register handler for delegation changed messages.
    using DelegationChangedFn = std::function<void(const uint256& id, ChangeType status)>;
    virtual std::unique_ptr<Handler> handleDelegationChanged(DelegationChangedFn fn) = 0;

    //! Register handler for super staker changed messages.
    using SuperStakerChangedFn = std::function<void(const uint256& id, ChangeType status)>;
    virtual std::unique_ptr<Handler> handleSuperStakerChanged(SuperStakerChangedFn fn) = 0;

    //! Register handler for delegations staker changed messages.
    using DelegationsStakerChangedFn = std::function<void(const uint160& id, ChangeType status)>;
    virtual std::unique_ptr<Handler> handleDelegationsStakerChanged(DelegationsStakerChangedFn fn) = 0;
};

//! Information about one wallet address.
struct WalletAddress
{
    CTxDestination dest;
    isminetype is_mine;
    std::string name;
    std::string purpose;

    WalletAddress(CTxDestination dest, isminetype is_mine, std::string name, std::string purpose)
        : dest(std::move(dest)), is_mine(is_mine), name(std::move(name)), purpose(std::move(purpose))
    {
    }
};

//! Collection of wallet balances.
struct WalletBalances
{
    CAmount balance = 0;
    CAmount unconfirmed_balance = 0;
    CAmount immature_balance = 0;
    CAmount stake = 0;
    bool have_watch_only = false;
    CAmount watch_only_balance = 0;
    CAmount unconfirmed_watch_only_balance = 0;
    CAmount immature_watch_only_balance = 0;
    CAmount watch_only_stake = 0;

    bool balanceChanged(const WalletBalances& prev) const
    {
        return balance != prev.balance || unconfirmed_balance != prev.unconfirmed_balance ||
               immature_balance != prev.immature_balance || stake != prev.stake || watch_only_balance != prev.watch_only_balance ||
               unconfirmed_watch_only_balance != prev.unconfirmed_watch_only_balance ||
               immature_watch_only_balance != prev.immature_watch_only_balance || watch_only_stake != prev.watch_only_stake;
    }
};

//! Wallet transaction information.
struct WalletTx
{
    CTransactionRef tx;
    std::vector<isminetype> txin_is_mine;
    std::vector<isminetype> txout_is_mine;
    std::vector<CTxDestination> txout_address;
    std::vector<isminetype> txout_address_is_mine;
    CAmount credit;
    CAmount debit;
    CAmount change;
    int64_t time;
    std::map<std::string, std::string> value_map;
    bool is_coinbase;
    bool is_coinstake;
    bool is_in_main_chain;

    // Contract tx params
    bool has_create_or_call;
    CKeyID tx_sender_key;
    std::vector<CKeyID> txout_keys;
};

//! Updated transaction status.
struct WalletTxStatus
{
    int block_height;
    int blocks_to_maturity;
    int depth_in_main_chain;
    unsigned int time_received;
    uint32_t lock_time;
    bool is_final;
    bool is_trusted;
    bool is_abandoned;
    bool is_coinbase;
    bool is_coinstake;
    bool is_in_main_chain;
};

//! Wallet transaction output.
struct WalletTxOut
{
    CTxOut txout;
    int64_t time;
    int depth_in_main_chain = -1;
    bool is_spent = false;
};

// Wallet token information.
struct TokenInfo
{
    std::string contract_address;
    std::string token_name;
    std::string token_symbol;
    uint8_t decimals = 0;
    std::string sender_address;
    int64_t time = 0;
    uint256 block_hash;
    int64_t block_number = -1;
    uint256 hash;
};

// Wallet token transaction
struct TokenTx
{
    std::string contract_address;
    std::string sender_address;
    std::string receiver_address;
    uint256 value;
    uint256 tx_hash;
    int64_t time = 0;
    uint256 block_hash;
    int64_t block_number = -1;
    std::string label;
    uint256 hash;
};

// Wallet contract book data */
struct ContractBookData
{
    std::string address;
    std::string name;
    std::string abi;
};

// Wallet delegation information.
struct DelegationInfo
{
    std::string delegate_address;
    std::string staker_address;
    std::string staker_name;
    uint8_t fee = 0;
    int64_t time = 0;
    int64_t block_number = -1;
    uint256 hash;
    uint256 create_tx_hash;
    uint256 remove_tx_hash;
};

// Delegation details.
struct DelegationDetails
{
    // Wallet delegation details
    bool w_entry_exist = false;
    std::string w_delegate_address;
    std::string w_staker_address;
    std::string w_staker_name;
    uint8_t w_fee = 0;
    int64_t w_time = 0;
    int64_t w_block_number = -1;
    uint256 w_hash;
    uint256 w_create_tx_hash;
    uint256 w_remove_tx_hash;

    // Wallet create tx details
    bool w_create_exist = false;
    bool w_create_in_main_chain = false;
    bool w_create_in_mempool = false;
    bool w_create_abandoned = false;

    // Wallet remove tx details
    bool w_remove_exist = false;
    bool w_remove_in_main_chain = false;
    bool w_remove_in_mempool = false;
    bool w_remove_abandoned = false;

    // Delegation contract details
    std::string c_delegate_address;
    std::string c_staker_address;
    uint8_t c_fee = 0;
    int64_t c_block_number = -1;
    bool c_entry_exist = false;
    bool c_contract_return = false;

    // To delegation info
    DelegationInfo toInfo(bool fromWallet = true)
    {
        interfaces::DelegationInfo info;
        info.delegate_address = fromWallet ? w_delegate_address : c_delegate_address;
        info.staker_address = fromWallet ? w_staker_address : c_staker_address;
        info.fee =  fromWallet ? w_fee : c_fee;
        info.block_number = fromWallet ? w_block_number : c_block_number;
        info.hash = w_hash;
        info.time = w_time;
        info.create_tx_hash = w_create_tx_hash;
        info.remove_tx_hash = w_remove_tx_hash;
        info.staker_name = w_staker_name;
        return info;
    }
};

// Super staker address list
enum SuperStakerAddressList
{
    AcceptAll = 0,
    AllowList = 1,
    ExcludeList = 2
};

// Wallet super staker information.
struct SuperStakerInfo
{
    uint256 hash;
    std::string staker_address;
    std::string staker_name;
    int64_t time = 0;
    bool custom_config = false;
    uint8_t min_fee = 0;
    CAmount min_delegate_utxo = 0;
    std::vector<std::string> delegate_address_list;
    int delegate_address_type = 0;
};

// Wallet delegation staker information.
struct DelegationStakerInfo
{
    std::string delegate_address;
    std::string staker_address;
    std::string PoD;
    uint8_t fee = 0;
    int64_t time = 0;
    int64_t block_number = -1;
    CAmount weight = 0;
    uint160 hash;
};

//! Return implementation of Wallet interface. This function is defined in
//! dummywallet.cpp and throws if the wallet component is not compiled.
std::unique_ptr<Wallet> MakeWallet(const std::shared_ptr<CWallet>& wallet);

} // namespace interfaces

#endif // BITCOIN_INTERFACES_WALLET_H
