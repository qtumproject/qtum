// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/walletmodel.h>

#include <qt/addresstablemodel.h>
#include <qt/guiconstants.h>
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
#include <ui_interface.h>
#include <util/system.h> // for GetBoolArg
#include <wallet/coincontrol.h>
#include <wallet/wallet.h>

#include <stdint.h>

#include <QDebug>
#include <QMessageBox>
#include <QSet>
#include <QTimer>
#include <QFile>

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
        // Update the model with results of task that take more time to be completed
        walletModel->checkCoinAddressesChanged();
        walletModel->checkStakeWeightChanged();
    }
};

#include <qt/walletmodel.moc>

WalletModel::WalletModel(std::unique_ptr<interfaces::Wallet> wallet, interfaces::Node& node, const PlatformStyle *platformStyle, OptionsModel *_optionsModel, QObject *parent) :
    QObject(parent), m_wallet(std::move(wallet)), m_node(node), optionsModel(_optionsModel), addressTableModel(nullptr),
    contractTableModel(nullptr),
    transactionTableModel(nullptr),
    recentRequestsTableModel(nullptr),
    tokenItemModel(nullptr),
    tokenTransactionTableModel(nullptr),
    delegationItemModel(nullptr),
    superStakerItemModel(nullptr),
    delegationStakerItemModel(nullptr),
    cachedEncryptionStatus(Unencrypted),
    cachedNumBlocks(0),
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

    subscribeToCoreSignals();
}

WalletModel::~WalletModel()
{
    unsubscribeFromCoreSignals();

    t.quit();
    t.wait();
}

void WalletModel::startPollBalance()
{
    // This timer will be fired repeatedly to update the balance
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &WalletModel::pollBalanceChanged);
    connect(timer, SIGNAL(timeout()), worker, SLOT(updateModel()));
    timer->start(MODEL_UPDATE_DELAY);
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
    // Try to get balances and return early if locks can't be acquired. This
    // avoids the GUI from getting stuck on periodical polls if the core is
    // holding the locks for a longer time - for example, during a wallet
    // rescan.
    interfaces::WalletBalances new_balances;
    int numBlocks = -1;
    if (!m_wallet->tryGetBalances(new_balances, numBlocks)) {
        return;
    }

    // Get node synchronization information
    bool isSyncing = false;
    m_node.getSyncInfo(numBlocks, isSyncing);
    bool cachedNumBlocksChanged = numBlocks != cachedNumBlocks;
    if(fForceCheckBalanceChanged || cachedNumBlocksChanged)
    {
        fForceCheckBalanceChanged = false;

        // Balance and number of transactions might have changed
        cachedNumBlocks = numBlocks;

        bool balanceChanged = checkBalanceChanged(new_balances);
        if(transactionTableModel)
            transactionTableModel->updateConfirmations();

        if(tokenTransactionTableModel)
            tokenTransactionTableModel->updateConfirmations();

        if(cachedNumBlocksChanged)
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
        if(!isSyncing && (balanceChanged || cachedNumBlocksChanged))
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
    if(tokenItemModel)
    {
        tokenItemModel->checkTokenBalanceChanged();
    }
}

void WalletModel::checkDelegationChanged()
{
    if(delegationItemModel)
    {
        delegationItemModel->checkDelegationChanged();
    }
}

void WalletModel::checkSuperStakerChanged()
{
    if(superStakerItemModel)
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

#ifdef ENABLE_BIP70
        if (rcp.paymentRequest.IsInitialized())
        {   // PaymentRequest...
            CAmount subtotal = 0;
            const payments::PaymentDetails& details = rcp.paymentRequest.getDetails();
            for (int i = 0; i < details.outputs_size(); i++)
            {
                const payments::Output& out = details.outputs(i);
                if (out.amount() <= 0) continue;
                subtotal += out.amount();
                const unsigned char* scriptStr = (const unsigned char*)out.script().data();
                CScript scriptPubKey(scriptStr, scriptStr+out.script().size());
                CAmount nAmount = out.amount();
                CRecipient recipient = {scriptPubKey, nAmount, rcp.fSubtractFeeFromAmount};
                vecSend.push_back(recipient);
            }
            if (subtotal <= 0)
            {
                return InvalidAmount;
            }
            total += subtotal;
        }
        else
#endif
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
        std::string strFailReason;

        auto& newTx = transaction.getWtx();
        newTx = m_wallet->createTransaction(vecSend, coinControl, true /* sign */, nChangePosRet, nFeeRequired, strFailReason);
        transaction.setTransactionFee(nFeeRequired);
        if (fSubtractFeeFromAmount && newTx)
            transaction.reassignAmounts(nChangePosRet);

        if(!newTx)
        {
            if(!fSubtractFeeFromAmount && (total + nFeeRequired) > nBalance)
            {
                return SendCoinsReturn(AmountWithFeeExceedsBalance);
            }
            Q_EMIT message(tr("Send Coins"), QString::fromStdString(strFailReason),
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
#ifdef ENABLE_BIP70
            if (rcp.paymentRequest.IsInitialized())
            {
                // Make sure any payment requests involved are still valid.
                if (PaymentServer::verifyExpired(rcp.paymentRequest.getDetails())) {
                    return PaymentRequestExpired;
                }

                // Store PaymentRequests in wtx.vOrderForm in wallet.
                std::string value;
                rcp.paymentRequest.SerializeToString(&value);
                vOrderForm.emplace_back("PaymentRequest", std::move(value));
            }
            else
#endif
            if (!rcp.message.isEmpty()) // Message from normal bitcoin:URI (bitcoin:123...?message=example)
                vOrderForm.emplace_back("Message", rcp.message.toStdString());
        }

        auto& newTx = transaction.getWtx();
        std::string rejectReason;
        if (!wallet().commitTransaction(newTx, {} /* mapValue */, std::move(vOrderForm), rejectReason))
            return SendCoinsReturn(TransactionCommitFailed, QString::fromStdString(rejectReason));

        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << *newTx;
        transaction_array.append(&(ssTx[0]), ssTx.size());
    }

    // Add addresses / update labels that we've sent to the address book,
    // and emit coinsSent signal for each recipient
    for (const SendCoinsRecipient &rcp : transaction.getRecipients())
    {
        // Don't touch the address book when we have a payment request
#ifdef ENABLE_BIP70
        if (!rcp.paymentRequest.IsInitialized())
#endif
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

bool WalletModel::setWalletEncrypted(bool encrypted, const SecureString &passphrase)
{
    if(encrypted)
    {
        // Encrypt
        return m_wallet->encryptWallet(passphrase);
    }
    else
    {
        // Decrypt -- TODO; not supported yet
        return false;
    }
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
        fs::path pathWalletBak = GetDataDir() / strprintf("wallet.%d.bak", GetTime());
        std::string walletBak = pathWalletBak.string();
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
    m_handler_can_get_addrs_changed = m_wallet->handleCanGetAddressesChanged(boost::bind(NotifyCanGetAddressesChanged, this));
    m_handler_contract_book_changed = m_wallet->handleContractBookChanged(boost::bind(NotifyContractBookChanged, this, _1, _2, _3, _4));
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

void WalletModel::loadReceiveRequests(std::vector<std::string>& vReceiveRequests)
{
    vReceiveRequests = m_wallet->getDestValues("rr"); // receive request
}

bool WalletModel::saveReceiveRequest(const std::string &sAddress, const int64_t nId, const std::string &sRequest)
{
    CTxDestination dest = DecodeDestination(sAddress);

    std::stringstream ss;
    ss << nId;
    std::string key = "rr" + ss.str(); // "rr" prefix = "receive request" in destdata

    if (sRequest.empty())
        return m_wallet->eraseDestData(dest, key);
    else
        return m_wallet->addDestData(dest, key, sRequest);
}

bool WalletModel::bumpFee(uint256 hash, uint256& new_hash)
{
    CCoinControl coin_control;
    coin_control.m_signal_bip125_rbf = true;
    std::vector<std::string> errors;
    CAmount old_fee;
    CAmount new_fee;
    CMutableTransaction mtx;
    if (!m_wallet->createBumpTransaction(hash, coin_control, 0 /* totalFee */, errors, old_fee, new_fee, mtx)) {
        QMessageBox::critical(nullptr, tr("Fee bump error"), tr("Increasing transaction fee failed") + "<br />(" +
            (errors.size() ? QString::fromStdString(errors[0]) : "") +")");
         return false;
    }

    // allow a user based fee verification
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
    SendConfirmationDialog confirmationDialog(tr("Confirm fee bump"), questionString);
    confirmationDialog.exec();
    QMessageBox::StandardButton retval = static_cast<QMessageBox::StandardButton>(confirmationDialog.result());

    // cancel sign&broadcast if user doesn't want to bump the fee
    if (retval != QMessageBox::Yes) {
        return false;
    }

    WalletModel::UnlockContext ctx(requestUnlock());
    if(!ctx.isValid())
    {
        return false;
    }

    // sign bumped transaction
    if (!m_wallet->signBumpTransaction(mtx)) {
        QMessageBox::critical(nullptr, tr("Fee bump error"), tr("Can't sign transaction."));
        return false;
    }
    // commit the bumped transaction
    if(!m_wallet->commitBumpTransaction(hash, std::move(mtx), errors, new_hash)) {
        QMessageBox::critical(nullptr, tr("Fee bump error"), tr("Could not commit transaction") + "<br />(" +
            QString::fromStdString(errors[0])+")");
         return false;
    }
    return true;
}

bool WalletModel::isWalletEnabled()
{
   return !gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET);
}

bool WalletModel::privateKeysDisabled() const
{
    return m_wallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
}

bool WalletModel::canGetAddresses() const
{
    return m_wallet->canGetAddresses();
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
    return m_node.getWallets().size() > 1;
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
