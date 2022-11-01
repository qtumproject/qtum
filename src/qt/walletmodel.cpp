// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/walletmodel.h>

#include <qt/addresstablemodel.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/paymentserver.h>
#include <qt/recentrequeststablemodel.h>
#include <qt/sendcoinsdialog.h>
#include <qt/transactiontablemodel.h>
#include <qt/tokenitemmodel.h>
#include <qt/tokentransactiontablemodel.h>
#include <qt/contracttablemodel.h>
#include <qt/delegationitemmodel.h>
#include <qt/superstakeritemmodel.h>
#include <qt/delegationstakeritemmodel.h>
#include <interfaces/handler.h>
#include <interfaces/node.h>
#include <key_io.h>
#include <node/ui_interface.h>
#include <psbt.h>
#include <util/system.h> // for GetBoolArg
#include <util/translation.h>
#include <wallet/coincontrol.h>
#include <wallet/wallet.h> // for CRecipient
#include <qt/qtumhwitool.h>

#include <stdint.h>
#include <functional>

#include <QDebug>
#include <QMessageBox>
#include <QSet>
#include <QTimer>
#include <QFile>

using wallet::CCoinControl;
using wallet::CRecipient;
using wallet::DEFAULT_DISABLE_WALLET;

static int pollSyncSkip = 30;

class WalletWorker : public QObject
{
    Q_OBJECT
public:
    WalletModel *walletModel;
    WalletWorker(WalletModel *_walletModel):
        walletModel(_walletModel){}

private Q_SLOTS:
    void updateModel()
    {
        if(walletModel && walletModel->node().shutdownRequested())
            return;

        // Update the model with results of task that take more time to be completed
        walletModel->checkHardwareWallet();
        walletModel->checkCoinAddressesChanged();
        walletModel->checkStakeWeightChanged();
        walletModel->checkHardwareDevice();
    }
};

#include <qt/walletmodel.moc>

WalletModel::WalletModel(std::unique_ptr<interfaces::Wallet> wallet, ClientModel& client_model, const PlatformStyle *platformStyle, QObject *parent) :
    QObject(parent),
    m_wallet(std::move(wallet)),
    m_client_model(&client_model),
    m_node(client_model.node()),
    optionsModel(client_model.getOptionsModel()),
    addressTableModel(nullptr),
    contractTableModel(nullptr),
    transactionTableModel(nullptr),
    recentRequestsTableModel(nullptr),
    tokenItemModel(nullptr),
    tokenTransactionTableModel(nullptr),
    delegationItemModel(nullptr),
    superStakerItemModel(nullptr),
    delegationStakerItemModel(nullptr),
    cachedEncryptionStatus(Unencrypted),
    timer(new QTimer(this)),
    nWeight(0),
    updateStakeWeight(true),
    updateCoinAddresses(true),
    worker(0)
{
    fHaveWatchOnly = m_wallet->haveWatchOnly();
    addressTableModel = new AddressTableModel(this);
    contractTableModel = new ContractTableModel(this);
    transactionTableModel = new TransactionTableModel(platformStyle, this);
    recentRequestsTableModel = new RecentRequestsTableModel(this);
    tokenItemModel = new TokenItemModel(this);
    tokenTransactionTableModel = new TokenTransactionTableModel(platformStyle, this);
    delegationItemModel = new DelegationItemModel(this);
    superStakerItemModel = new SuperStakerItemModel(this);
    delegationStakerItemModel = new DelegationStakerItemModel(this);

    worker = new WalletWorker(this);
    worker->moveToThread(&(t));
    t.start();

    connect(addressTableModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(checkCoinAddresses()));
    connect(addressTableModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(checkCoinAddresses()));
    connect(recentRequestsTableModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(checkCoinAddresses()));
    connect(recentRequestsTableModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(checkCoinAddresses()));
    subscribeToCoreSignals();
}

WalletModel::~WalletModel()
{
    unsubscribeFromCoreSignals();

    join();
}

void WalletModel::startPollBalance()
{
    // This timer will be fired repeatedly to update the balance
    // Since the QTimer::timeout is a private signal, it cannot be used
    // in the GUIUtil::ExceptionSafeConnect directly.
    connect(timer, &QTimer::timeout, this, &WalletModel::timerTimeout);
    GUIUtil::ExceptionSafeConnect(this, &WalletModel::timerTimeout, this, &WalletModel::pollBalanceChanged);
    connect(timer, SIGNAL(timeout()), worker, SLOT(updateModel()));
    timer->start(MODEL_UPDATE_DELAY);
}

void WalletModel::setClientModel(ClientModel* client_model)
{
    m_client_model = client_model;
    if (!m_client_model) timer->stop();
}

void WalletModel::updateStatus()
{
    EncryptionStatus newEncryptionStatus = getEncryptionStatus();

    if(cachedEncryptionStatus != newEncryptionStatus) {
        Q_EMIT encryptionStatusChanged();
    }
}

void WalletModel::pollBalanceChanged()
{
    // Get node synchronization information
    int numBlocks = -1;
    bool isSyncing = false;
    pollNum++;
    if(!m_node.tryGetSyncInfo(numBlocks, isSyncing) || (isSyncing && pollNum < pollSyncSkip))
        return;

    // Avoid recomputing wallet balances unless a TransactionChanged or
    // BlockTip notification was received.
    if (!fForceCheckBalanceChanged && m_cached_last_update_tip == getLastBlockProcessed()) return;

    // Try to get balances and return early if locks can't be acquired. This
    // avoids the GUI from getting stuck on periodical polls if the core is
    // holding the locks for a longer time - for example, during a wallet
    // rescan.
    interfaces::WalletBalances new_balances;
    uint256 block_hash;
    if (!m_wallet->tryGetBalances(new_balances, block_hash)) {
        return;
    }
    pollNum = 0;

    bool cachedBlockHashChanged = block_hash != m_cached_last_update_tip;
    if (fForceCheckBalanceChanged || cachedBlockHashChanged) {
        fForceCheckBalanceChanged = false;

        // Balance and number of transactions might have changed
        m_cached_last_update_tip = block_hash;

        bool balanceChanged = checkBalanceChanged(new_balances);
        if(m_client_model && transactionTableModel)
            transactionTableModel->updateConfirmations();

        if(m_client_model && tokenTransactionTableModel)
            tokenTransactionTableModel->updateConfirmations();

        if(cachedBlockHashChanged)
        {
            checkTokenBalanceChanged();
            checkDelegationChanged();
            checkSuperStakerChanged();
        }

        if(balanceChanged)
        {
            updateCoinAddresses = true;
        }

        // The stake weight is used for the staking icon status
        // Get the stake weight only when not syncing because it is time consuming
        if(!isSyncing && (balanceChanged || cachedBlockHashChanged))
        {
            updateStakeWeight = true;
        }
    }
}

void WalletModel::updateContractBook(const QString &address, const QString &label, const QString &abi, int status)
{
    if(contractTableModel)
        contractTableModel->updateEntry(address, label, abi, status);
}

bool WalletModel::checkBalanceChanged(const interfaces::WalletBalances& new_balances)
{
    if(new_balances.balanceChanged(m_cached_balances)) {
        m_cached_balances = new_balances;
        Q_EMIT balanceChanged(new_balances);
        return true;
    }
    return false;
}

void WalletModel::checkTokenBalanceChanged()
{
    if(m_client_model && tokenItemModel)
    {
        tokenItemModel->checkTokenBalanceChanged();
    }
}

void WalletModel::checkDelegationChanged()
{
    if(m_client_model && delegationItemModel)
    {
        delegationItemModel->checkDelegationChanged();
    }
}

void WalletModel::checkSuperStakerChanged()
{
    if(m_client_model && superStakerItemModel)
    {
        superStakerItemModel->checkSuperStakerChanged();
    }
}

void WalletModel::updateTransaction()
{
    // Balance and number of transactions might have changed
    fForceCheckBalanceChanged = true;
}

void WalletModel::updateAddressBook(const QString &address, const QString &label,
        bool isMine, const QString &purpose, int status)
{
    if(addressTableModel)
        addressTableModel->updateEntry(address, label, isMine, purpose, status);
}

void WalletModel::updateWatchOnlyFlag(bool fHaveWatchonly)
{
    fHaveWatchOnly = fHaveWatchonly;
    Q_EMIT notifyWatchonlyChanged(fHaveWatchonly);
}

bool WalletModel::validateAddress(const QString &address)
{
    return IsValidDestinationString(address.toStdString());
}

WalletModel::SendCoinsReturn WalletModel::prepareTransaction(WalletModelTransaction &transaction, const CCoinControl& coinControl)
{
    CAmount total = 0;
    bool fSubtractFeeFromAmount = false;
    QList<SendCoinsRecipient> recipients = transaction.getRecipients();
    std::vector<CRecipient> vecSend;

    if(recipients.empty())
    {
        return OK;
    }

    QSet<QString> setAddress; // Used to detect duplicates
    int nAddresses = 0;

    // Pre-check input data for validity
    for (const SendCoinsRecipient &rcp : recipients)
    {
        if (rcp.fSubtractFeeFromAmount)
            fSubtractFeeFromAmount = true;
        {   // User-entered bitcoin address / amount:
            if(!validateAddress(rcp.address))
            {
                return InvalidAddress;
            }
            if(rcp.amount <= 0)
            {
                return InvalidAmount;
            }
            setAddress.insert(rcp.address);
            ++nAddresses;

            CScript scriptPubKey = GetScriptForDestination(DecodeDestination(rcp.address.toStdString()));
            CRecipient recipient = {scriptPubKey, rcp.amount, rcp.fSubtractFeeFromAmount};
            vecSend.push_back(recipient);

            total += rcp.amount;
        }
    }
    if(setAddress.size() != nAddresses)
    {
        return DuplicateAddress;
    }

    CAmount nBalance = m_wallet->getAvailableBalance(coinControl);

    if(total > nBalance)
    {
        return AmountExceedsBalance;
    }

    {
        CAmount nFeeRequired = 0;
        int nChangePosRet = -1;
        bilingual_str error;

        auto& newTx = transaction.getWtx();
        newTx = m_wallet->createTransaction(vecSend, coinControl, !wallet().privateKeysDisabled() /* sign */, nChangePosRet, nFeeRequired, error);
        transaction.setTransactionFee(nFeeRequired);
        if (fSubtractFeeFromAmount && newTx)
            transaction.reassignAmounts(nChangePosRet);

        if(!newTx)
        {
            if(!fSubtractFeeFromAmount && (total + nFeeRequired) > nBalance)
            {
                return SendCoinsReturn(AmountWithFeeExceedsBalance);
            }
            Q_EMIT message(tr("Send Coins"), QString::fromStdString(error.translated),
                CClientUIInterface::MSG_ERROR);
            return TransactionCreationFailed;
        }

        // Reject absurdly high fee. (This can never happen because the
        // wallet never creates transactions with fee greater than
        // m_default_max_tx_fee. This merely a belt-and-suspenders check).
        if (nFeeRequired > m_wallet->getDefaultMaxTxFee()) {
            return AbsurdFee;
        }
    }

    return SendCoinsReturn(OK);
}

WalletModel::SendCoinsReturn WalletModel::sendCoins(WalletModelTransaction &transaction)
{
    QByteArray transaction_array; /* store serialized transaction */

    {
        std::vector<std::pair<std::string, std::string>> vOrderForm;
        for (const SendCoinsRecipient &rcp : transaction.getRecipients())
        {
            if (!rcp.message.isEmpty()) // Message from normal bitcoin:URI (bitcoin:123...?message=example)
                vOrderForm.emplace_back("Message", rcp.message.toStdString());
        }

        auto& newTx = transaction.getWtx();
        wallet().commitTransaction(newTx, {} /* mapValue */, std::move(vOrderForm));

        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << *newTx;
        transaction_array.append((const char*)ssTx.data(), ssTx.size());
    }

    // Add addresses / update labels that we've sent to the address book,
    // and emit coinsSent signal for each recipient
    for (const SendCoinsRecipient &rcp : transaction.getRecipients())
    {
        {
            std::string strAddress = rcp.address.toStdString();
            CTxDestination dest = DecodeDestination(strAddress);
            std::string strLabel = rcp.label.toStdString();
            {
                // Check if we have a new address or an updated label
                std::string name;
                if (!m_wallet->getAddress(
                     dest, &name, /* is_mine= */ nullptr, /* purpose= */ nullptr))
                {
                    m_wallet->setAddressBook(dest, strLabel, "send");
                }
                else if (name != strLabel)
                {
                    m_wallet->setAddressBook(dest, strLabel, ""); // "" means don't change purpose
                }
            }
        }
        Q_EMIT coinsSent(this, rcp, transaction_array);
    }

    checkBalanceChanged(m_wallet->getBalances()); // update balance immediately, otherwise there could be a short noticeable delay until pollBalanceChanged hits

    return SendCoinsReturn(OK);
}

OptionsModel *WalletModel::getOptionsModel()
{
    return optionsModel;
}

AddressTableModel *WalletModel::getAddressTableModel()
{
    return addressTableModel;
}

ContractTableModel *WalletModel::getContractTableModel()
{
    return contractTableModel;
}

TransactionTableModel *WalletModel::getTransactionTableModel()
{
    return transactionTableModel;
}

RecentRequestsTableModel *WalletModel::getRecentRequestsTableModel()
{
    return recentRequestsTableModel;
}

TokenItemModel *WalletModel::getTokenItemModel()
{
    return tokenItemModel;
}

TokenTransactionTableModel *WalletModel::getTokenTransactionTableModel()
{
    return tokenTransactionTableModel;
}

DelegationItemModel *WalletModel::getDelegationItemModel()
{
    return delegationItemModel;
}

SuperStakerItemModel *WalletModel::getSuperStakerItemModel()
{
    return superStakerItemModel;
}

DelegationStakerItemModel *WalletModel::getDelegationStakerItemModel()
{
    return delegationStakerItemModel;
}

WalletModel::EncryptionStatus WalletModel::getEncryptionStatus() const
{
    if(!m_wallet->isCrypted())
    {
        // A previous bug allowed for watchonly wallets to be encrypted (encryption keys set, but nothing is actually encrypted).
        // To avoid misrepresenting the encryption status of such wallets, we only return NoKeys for watchonly wallets that are unencrypted.
        if (m_wallet->privateKeysDisabled()) {
            return NoKeys;
        }
        return Unencrypted;
    }
    else if(m_wallet->isLocked())
    {
        return Locked;
    }
    else
    {
        return Unlocked;
    }
}

bool WalletModel::setWalletEncrypted(const SecureString& passphrase)
{
    return m_wallet->encryptWallet(passphrase);
}

bool WalletModel::setWalletLocked(bool locked, const SecureString &passPhrase)
{
    if(locked)
    {
        // Lock
        return m_wallet->lock();
    }
    else
    {
        // Unlock
        return m_wallet->unlock(passPhrase);
    }
}

bool WalletModel::changePassphrase(const SecureString &oldPass, const SecureString &newPass)
{
    m_wallet->lock(); // Make sure wallet is locked before attempting pass change
    return m_wallet->changeWalletPassphrase(oldPass, newPass);
}

bool WalletModel::restoreWallet(const QString &filename, const QString &param)
{
    if(QFile::exists(filename))
    {
        fs::path pathWalletBak = gArgs.GetDataDirNet() / strprintf("wallet.%d.bak", GetTime());
        std::string walletBak = fs::PathToString(pathWalletBak);
        if(m_wallet->backupWallet(walletBak))
        {
            restorePath = filename;
            restoreParam = param;
            return true;
        }
    }

    return false;
}

// Handlers for core signals
static void NotifyUnload(WalletModel* walletModel)
{
    qDebug() << "NotifyUnload";
    bool invoked = QMetaObject::invokeMethod(walletModel, "unload");
    assert(invoked);
}

static void NotifyKeyStoreStatusChanged(WalletModel *walletmodel)
{
    qDebug() << "NotifyKeyStoreStatusChanged";
    bool invoked = QMetaObject::invokeMethod(walletmodel, "updateStatus", Qt::QueuedConnection);
    assert(invoked);
}

static void NotifyAddressBookChanged(WalletModel *walletmodel,
        const CTxDestination &address, const std::string &label, bool isMine,
        const std::string &purpose, ChangeType status)
{
    QString strAddress = QString::fromStdString(EncodeDestination(address));
    QString strLabel = QString::fromStdString(label);
    QString strPurpose = QString::fromStdString(purpose);

    qDebug() << "NotifyAddressBookChanged: " + strAddress + " " + strLabel + " isMine=" + QString::number(isMine) + " purpose=" + strPurpose + " status=" + QString::number(status);
    bool invoked = QMetaObject::invokeMethod(walletmodel, "updateAddressBook", Qt::QueuedConnection,
                              Q_ARG(QString, strAddress),
                              Q_ARG(QString, strLabel),
                              Q_ARG(bool, isMine),
                              Q_ARG(QString, strPurpose),
                              Q_ARG(int, status));
    assert(invoked);
}

static void NotifyTransactionChanged(WalletModel *walletmodel, const uint256 &hash, ChangeType status)
{
    Q_UNUSED(hash);
    Q_UNUSED(status);
    bool invoked = QMetaObject::invokeMethod(walletmodel, "updateTransaction", Qt::QueuedConnection);
    assert(invoked);
}

static void ShowProgress(WalletModel *walletmodel, const std::string &title, int nProgress)
{
    // emits signal "showProgress"
    bool invoked = QMetaObject::invokeMethod(walletmodel, "showProgress", Qt::QueuedConnection,
                              Q_ARG(QString, QString::fromStdString(title)),
                              Q_ARG(int, nProgress));
    assert(invoked);
}

static void NotifyWatchonlyChanged(WalletModel *walletmodel, bool fHaveWatchonly)
{
    bool invoked = QMetaObject::invokeMethod(walletmodel, "updateWatchOnlyFlag", Qt::QueuedConnection,
                              Q_ARG(bool, fHaveWatchonly));
    assert(invoked);
}

static void NotifyCanGetAddressesChanged(WalletModel* walletmodel)
{
    bool invoked = QMetaObject::invokeMethod(walletmodel, "canGetAddressesChanged");
    assert(invoked);
}

static void NotifyContractBookChanged(WalletModel *walletmodel,
        const std::string &address, const std::string &label, const std::string &abi, ChangeType status)
{
    QString strAddress = QString::fromStdString(address);
    QString strLabel = QString::fromStdString(label);
    QString strAbi = QString::fromStdString(abi);

    qDebug() << "NotifyContractBookChanged: " + strAddress + " " + strLabel + " status=" + QString::number(status);
    bool invoked = QMetaObject::invokeMethod(walletmodel, "updateContractBook", Qt::QueuedConnection,
                              Q_ARG(QString, strAddress),
                              Q_ARG(QString, strLabel),
                              Q_ARG(QString, strAbi),
                              Q_ARG(int, status));
    assert(invoked);
}

void WalletModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    m_handler_unload = m_wallet->handleUnload(std::bind(&NotifyUnload, this));
    m_handler_status_changed = m_wallet->handleStatusChanged(std::bind(&NotifyKeyStoreStatusChanged, this));
    m_handler_address_book_changed = m_wallet->handleAddressBookChanged(std::bind(NotifyAddressBookChanged, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
    m_handler_transaction_changed = m_wallet->handleTransactionChanged(std::bind(NotifyTransactionChanged, this, std::placeholders::_1, std::placeholders::_2));
    m_handler_show_progress = m_wallet->handleShowProgress(std::bind(ShowProgress, this, std::placeholders::_1, std::placeholders::_2));
    m_handler_watch_only_changed = m_wallet->handleWatchOnlyChanged(std::bind(NotifyWatchonlyChanged, this, std::placeholders::_1));
    m_handler_can_get_addrs_changed = m_wallet->handleCanGetAddressesChanged(std::bind(NotifyCanGetAddressesChanged, this));
    m_handler_contract_book_changed = m_wallet->handleContractBookChanged(std::bind(NotifyContractBookChanged, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

void WalletModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    m_handler_unload->disconnect();
    m_handler_status_changed->disconnect();
    m_handler_address_book_changed->disconnect();
    m_handler_transaction_changed->disconnect();
    m_handler_show_progress->disconnect();
    m_handler_watch_only_changed->disconnect();
    m_handler_can_get_addrs_changed->disconnect();
    m_handler_contract_book_changed->disconnect();
}

// WalletModel::UnlockContext implementation
WalletModel::UnlockContext WalletModel::requestUnlock()
{
    bool was_locked = getEncryptionStatus() == Locked;

    if ((!was_locked) && getWalletUnlockStakingOnly())
    {
       setWalletLocked(true);
       was_locked = getEncryptionStatus() == Locked;
    }

    if(was_locked)
    {
        // Request UI to unlock wallet
        Q_EMIT requireUnlock();
    }
    // If wallet is still locked, unlock was failed or cancelled, mark context as invalid
    bool valid = getEncryptionStatus() != Locked;

    return UnlockContext(this, valid, was_locked && !getWalletUnlockStakingOnly());
}

WalletModel::UnlockContext::UnlockContext(WalletModel *_wallet, bool _valid, bool _relock):
        wallet(_wallet),
        valid(_valid),
        relock(_relock),
        stakingOnly(false)
{
    if(!relock)
    {
        stakingOnly = wallet->getWalletUnlockStakingOnly();
        wallet->setWalletUnlockStakingOnly(false);
    }
}

WalletModel::UnlockContext::~UnlockContext()
{
    if(valid && relock)
    {
        wallet->setWalletLocked(true);
    }
    if(!relock)
    {
        wallet->setWalletUnlockStakingOnly(stakingOnly);
        wallet->updateStatus();
    }
}

void WalletModel::UnlockContext::CopyFrom(UnlockContext&& rhs)
{
    // Transfer context; old object no longer relocks wallet
    *this = rhs;
    rhs.relock = false;
}

bool WalletModel::bumpFee(uint256 hash, uint256& new_hash)
{
    CCoinControl coin_control;
    coin_control.m_signal_bip125_rbf = true;
    std::vector<bilingual_str> errors;
    CAmount old_fee;
    CAmount new_fee;
    CMutableTransaction mtx;
    if (!m_wallet->createBumpTransaction(hash, coin_control, errors, old_fee, new_fee, mtx)) {
        QMessageBox::critical(nullptr, tr("Fee bump error"), tr("Increasing transaction fee failed") + "<br />(" +
            (errors.size() ? QString::fromStdString(errors[0].translated) : "") +")");
        return false;
    }

    // allow a user based fee verification
    /*: Asks a user if they would like to manually increase the fee of a transaction that has already been created. */
    QString questionString = tr("Do you want to increase the fee?");
    questionString.append("<br />");
    questionString.append("<table style=\"text-align: left;\">");
    questionString.append("<tr><td>");
    questionString.append(tr("Current fee:"));
    questionString.append("</td><td>");
    questionString.append(BitcoinUnits::formatHtmlWithUnit(getOptionsModel()->getDisplayUnit(), old_fee));
    questionString.append("</td></tr><tr><td>");
    questionString.append(tr("Increase:"));
    questionString.append("</td><td>");
    questionString.append(BitcoinUnits::formatHtmlWithUnit(getOptionsModel()->getDisplayUnit(), new_fee - old_fee));
    questionString.append("</td></tr><tr><td>");
    questionString.append(tr("New fee:"));
    questionString.append("</td><td>");
    questionString.append(BitcoinUnits::formatHtmlWithUnit(getOptionsModel()->getDisplayUnit(), new_fee));
    questionString.append("</td></tr></table>");

    // Display warning in the "Confirm fee bump" window if the "Coin Control Features" option is enabled
    if (getOptionsModel()->getCoinControlFeatures()) {
        questionString.append("<br><br>");
        questionString.append(tr("Warning: This may pay the additional fee by reducing change outputs or adding inputs, when necessary. It may add a new change output if one does not already exist. These changes may potentially leak privacy."));
    }

    auto confirmationDialog = new SendConfirmationDialog(tr("Confirm fee bump"), questionString, "", "", SEND_CONFIRM_DELAY, !m_wallet->privateKeysDisabled(), getOptionsModel()->getEnablePSBTControls(), nullptr);
    confirmationDialog->setAttribute(Qt::WA_DeleteOnClose);
    // TODO: Replace QDialog::exec() with safer QDialog::show().
    const auto retval = static_cast<QMessageBox::StandardButton>(confirmationDialog->exec());

    // cancel sign&broadcast if user doesn't want to bump the fee
    if (retval != QMessageBox::Yes && retval != QMessageBox::Save) {
        return false;
    }

    WalletModel::UnlockContext ctx(requestUnlock());
    if(!ctx.isValid())
    {
        return false;
    }

    // Short-circuit if we are returning a bumped transaction PSBT to clipboard
    if (retval == QMessageBox::Save) {
        PartiallySignedTransaction psbtx(mtx);
        bool complete = false;
        const TransactionError err = wallet().fillPSBT(SIGHASH_ALL, false /* sign */, true /* bip32derivs */, nullptr, psbtx, complete);
        if (err != TransactionError::OK || complete) {
            QMessageBox::critical(nullptr, tr("Fee bump error"), tr("Can't draft transaction."));
            return false;
        }
        // Serialize the PSBT
        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << psbtx;
        GUIUtil::setClipboard(EncodeBase64(ssTx.str()).c_str());
        Q_EMIT message(tr("PSBT copied"), "Copied to clipboard", CClientUIInterface::MSG_INFORMATION);
        return true;
    }

    assert(!m_wallet->privateKeysDisabled());

    // sign bumped transaction
    if (!m_wallet->signBumpTransaction(mtx)) {
        QMessageBox::critical(nullptr, tr("Fee bump error"), tr("Can't sign transaction."));
        return false;
    }
    // commit the bumped transaction
    if(!m_wallet->commitBumpTransaction(hash, std::move(mtx), errors, new_hash)) {
        QMessageBox::critical(nullptr, tr("Fee bump error"), tr("Could not commit transaction") + "<br />(" +
            QString::fromStdString(errors[0].translated)+")");
        return false;
    }
    return true;
}

bool WalletModel::displayAddress(std::string sAddress)
{
    CTxDestination dest = DecodeDestination(sAddress);
    bool res = false;
    try {
        res = m_wallet->displayAddress(dest);
    } catch (const std::runtime_error& e) {
        QMessageBox::critical(nullptr, tr("Can't display address"), e.what());
    }
    return res;
}

bool WalletModel::isWalletEnabled()
{
   return !gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET);
}

QString WalletModel::getWalletName() const
{
    return QString::fromStdString(m_wallet->getWalletName());
}

QString WalletModel::getDisplayName() const
{
    const QString name = getWalletName();
    return name.isEmpty() ? "["+tr("default wallet")+"]" : name;
}

bool WalletModel::isMultiwallet()
{
    return m_node.walletLoader().getWallets().size() > 1;
}

void WalletModel::refresh(bool pk_hash_only)
{
    addressTableModel = new AddressTableModel(this, pk_hash_only);
}

uint256 WalletModel::getLastBlockProcessed() const
{
    return m_client_model ? m_client_model->getBestBlockHash() : uint256{};
}

QString WalletModel::getRestorePath()
{
    return restorePath;
}

QString WalletModel::getRestoreParam()
{
    return restoreParam;
}

bool WalletModel::restore()
{
    return !restorePath.isEmpty();
}

uint64_t WalletModel::getStakeWeight()
{
    return nWeight;
}

bool WalletModel::getWalletUnlockStakingOnly()
{
    return m_wallet->getWalletUnlockStakingOnly();
}

void WalletModel::setWalletUnlockStakingOnly(bool unlock)
{
    m_wallet->setWalletUnlockStakingOnly(unlock);
}

void WalletModel::checkCoinAddressesChanged()
{
    // Get the list of coin addresses and emit it to the subscribers
    std::vector<std::string> spendableAddresses;
    std::vector<std::string> allAddresses;
    bool includeZeroValue = false;
    if(updateCoinAddresses && m_wallet->tryGetAvailableAddresses(spendableAddresses, allAddresses, includeZeroValue))
    {
        QStringList listSpendableAddresses;
        for(std::string address : spendableAddresses)
            listSpendableAddresses.append(QString::fromStdString(address));

        QStringList listAllAddresses;
        for(std::string address : allAddresses)
            listAllAddresses.append(QString::fromStdString(address));

        Q_EMIT availableAddressesChanged(listSpendableAddresses, listAllAddresses, includeZeroValue);

        updateCoinAddresses = false;
    }
}

void WalletModel::checkStakeWeightChanged()
{
    if(updateStakeWeight && m_wallet->tryGetStakeWeight(nWeight))
    {
        updateStakeWeight = false;
    }
}

void WalletModel::checkCoinAddresses()
{
    updateCoinAddresses = true;
}

QString WalletModel::getFingerprint(bool stake) const
{
    if(stake)
    {
        std::string ledgerId = wallet().getStakerLedgerId();
        return QString::fromStdString(ledgerId);
    }

    return fingerprint;
}

void WalletModel::setFingerprint(const QString &value, bool stake)
{
    if(stake)
    {
        wallet().setStakerLedgerId(value.toStdString());
    }
    else
    {
        fingerprint = value;
    }
}

void WalletModel::checkHardwareWallet()
{
    if(hardwareWalletInitRequired)
    {
        // Init variables
        QtumHwiTool hwiTool;
        hwiTool.setModel(this);
        QString errorMessage;
        bool error = false;

        if(hwiTool.isConnected(fingerprint, false))
        {
            // Setup key pool
            if(importPKH)
            {
                QStringList pkhdesc;
                bool OK = hwiTool.getKeyPoolPKH(fingerprint, pathPKH, pkhdesc);
                if(OK) OK &= hwiTool.importMulti(pkhdesc);

                if(!OK)
                {
                    error = true;
                    errorMessage = tr("Import PKH failed.\n") + hwiTool.errorMessage();
                }
            }

            if(importP2SH)
            {
                QStringList p2shdesc;
                bool OK = hwiTool.getKeyPoolP2SH(fingerprint, pathP2SH, p2shdesc);
                if(OK) OK &= hwiTool.importMulti(p2shdesc);

                if(!OK)
                {
                    error = true;
                    if(!errorMessage.isEmpty()) errorMessage += "\n\n";
                    errorMessage += tr("Import P2SH failed.\n") + hwiTool.errorMessage();
                }
            }

            if(importBech32)
            {
                QStringList bech32desc;
                bool OK = hwiTool.getKeyPoolBech32(fingerprint, pathBech32, bech32desc);
                if(OK) OK &= hwiTool.importMulti(bech32desc);

                if(!OK)
                {
                    error = true;
                    if(!errorMessage.isEmpty()) errorMessage += "\n\n";
                    errorMessage += tr("Import Bech32 failed.\n") + hwiTool.errorMessage();
                }
            }

            // Rescan the chain
            if(rescan && !error)
                hwiTool.rescanBlockchain();
        }
        else
        {
            error = true;
            errorMessage = tr("Ledger not connected.");
        }

        // Display error message if happen
        if(error)
        {
            if(errorMessage.isEmpty())
                errorMessage = tr("unknown error");
            Q_EMIT message(tr("Import addresses"), errorMessage,
                           CClientUIInterface::MSG_ERROR);
        }

        hardwareWalletInitRequired = false;
    }
}

void WalletModel::importAddressesData(bool _rescan, bool _importPKH, bool _importP2SH, bool _importBech32, QString _pathPKH, QString _pathP2SH, QString _pathBech32)
{
    rescan = _rescan;
    importPKH = _importPKH;
    importP2SH = _importP2SH;
    importBech32 = _importBech32;
    pathPKH = _pathPKH;
    pathP2SH = _pathP2SH;
    pathBech32 = _pathBech32;
    hardwareWalletInitRequired = true;
}

bool WalletModel::getSignPsbtWithHwiTool()
{
    if(!::Params().HasHardwareWalletSupport())
        return false;

    return wallet().privateKeysDisabled() && gArgs.GetBoolArg("-signpsbtwithhwitool", wallet::DEFAULT_SIGN_PSBT_WITH_HWI_TOOL);
}

bool WalletModel::createUnsigned()
{
    if(wallet().privateKeysDisabled())
    {
        if(!::Params().HasHardwareWalletSupport())
            return true;

        QString hwiToolPath = GUIUtil::getHwiToolPath();
        if(QFile::exists(hwiToolPath))
        {
            return !getSignPsbtWithHwiTool();
        }
        else
        {
            return true;
        }
    }

    return false;
}

bool WalletModel::hasLedgerProblem()
{
    return wallet().privateKeysDisabled() &&
            wallet().getEnabledStaking() &&
            !getFingerprint(true).isEmpty();
}

QList<HWDevice> WalletModel::getDevices()
{
    return devices;
}

void WalletModel::checkHardwareDevice()
{
    int64_t time = GetTimeMillis();
    if(time > (count_milliseconds(DEVICE_UPDATE_DELAY) + deviceTime))
    {
        QList<HWDevice> tmpDevices;

        // Get stake device
        QString fingerprint_stake = getFingerprint(true);
        if(!fingerprint_stake.isEmpty())
        {
            QtumHwiTool hwiTool;
            QList<HWDevice> _devices;
            if(hwiTool.enumerate(_devices, true))
            {
                for(HWDevice device : _devices)
                {
                    if(device.isValid() && device.fingerprint == fingerprint_stake)
                    {
                        tmpDevices.push_back(device);
                    }
                }
            }
        }

        // Get not stake device
        QString fingerprint_not_stake = getFingerprint();
        if(!fingerprint_not_stake.isEmpty())
        {
            QtumHwiTool hwiTool;
            QList<HWDevice> _devices;
            if(hwiTool.enumerate(_devices, false))
            {
                for(HWDevice device : _devices)
                {
                    if(device.isValid() && device.fingerprint == fingerprint_not_stake)
                    {
                        tmpDevices.push_back(device);
                    }
                }
            }
        }

        // Set update time
        deviceTime = GetTimeMillis();
        devices = tmpDevices;
    }
}

void WalletModel::join()
{
    // Stop timer
    if(timer)
        timer->stop();

    // Quit thread
    if(t.isRunning())
    {
        if(worker)
            worker->disconnect(this);
        t.quit();
        t.wait();
    }

    // Join models
    if(tokenItemModel)
        tokenItemModel->join();
    if(delegationItemModel)
        delegationItemModel->join();
    if(superStakerItemModel)
        superStakerItemModel->join();
}
