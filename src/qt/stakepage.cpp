#include <qt/stakepage.h>
#include <qt/forms/ui_stakepage.h>

#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/transactionfilterproxy.h>
#include <qt/transactiontablemodel.h>
#include <qt/walletmodel.h>
#include <interfaces/wallet.h>
#include <interfaces/node.h>
#include <qt/transactiondescdialog.h>
#include <qt/styleSheet.h>

#include <miner.h>

#include <QSortFilterProxyModel>

Q_DECLARE_METATYPE(interfaces::WalletBalances)

#include <qt/stakepage.moc>

StakePage::StakePage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StakePage),
    clientModel(nullptr),
    walletModel(nullptr)
{
    ui->setupUi(this);
    ui->checkStake->setEnabled(gArgs.GetBoolArg("-staking", DEFAULT_STAKE));
}

StakePage::~StakePage()
{
    delete ui;
}

void StakePage::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;

    if (_clientModel) {
        connect(_clientModel, &ClientModel::numBlocksChanged, this, &StakePage::numBlocksChanged);
        int height = _clientModel->node().getNumBlocks();
        ui->labelHeight->setText(QString::number(height));
        m_subsidy = _clientModel->node().getBlockSubsidy(height);
        updateNetworkWeight();
    }
}

void StakePage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        ui->checkStake->setChecked(model->wallet().getEnabledStaking());

        // Keep up to date with wallet
        interfaces::Wallet& wallet = model->wallet();
        interfaces::WalletBalances balances = wallet.getBalances();
        setBalance(balances);
        connect(model, &WalletModel::balanceChanged, this, &StakePage::setBalance);

        connect(model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &StakePage::updateDisplayUnit);
    }

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

void StakePage::setBalance(const interfaces::WalletBalances& balances)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    m_balances = balances;
    ui->labelAssets->setText(BitcoinUnits::formatWithUnit(unit, balances.balance, false, BitcoinUnits::separatorAlways));
    ui->labelStake->setText(BitcoinUnits::formatWithUnit(unit, balances.stake, false, BitcoinUnits::separatorAlways));
}

void StakePage::on_checkStake_clicked(bool checked)
{
    this->walletModel->wallet().setEnabledStaking(checked);
}

void StakePage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if (m_balances.balance != -1) {
            setBalance(m_balances);
        }
        updateSubsidy();
    }
}

void StakePage::numBlocksChanged(int count, const QDateTime &blockDate, double nVerificationProgress, bool headers)
{
    if(!headers)
    {
        ui->labelHeight->setText(QString::number(count));
        m_subsidy = clientModel->node().getBlockSubsidy(count);
        updateSubsidy();
        updateNetworkWeight();
    }
}

void StakePage::updateSubsidy()
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    QString strSubsidy = BitcoinUnits::formatWithUnit(unit, m_subsidy, false, BitcoinUnits::separatorAlways) + "/Block";
    ui->labelReward->setText(strSubsidy);
}

void StakePage::updateNetworkWeight()
{
    uint64_t networkWeight = clientModel->node().getNetworkStakeWeight();
    ui->labelWeight->setText(QString::number(networkWeight));
}
