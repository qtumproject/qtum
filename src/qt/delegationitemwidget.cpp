#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/delegationitemwidget.h>
#include <qt/platformstyle.h>
#include <qt/forms/ui_delegationitemwidget.h>
#include <qt/bitcoinunits.h>
#include <qt/optionsmodel.h>
#include <qt/walletmodel.h>

#include <QFile>

class DelegationItemWidgetPriv
{
public:
    QString fee;
    QString staker;
    QString address;
    int32_t blockHight = -1;
    int64_t balance = 0;
    int64_t stake = 0;
};

#define DELEGATION_ITEM_ICONSIZE 24
DelegationItemWidget::DelegationItemWidget(const PlatformStyle *platformStyle, QWidget *parent, ItemType type) :
    QWidget(parent),
    ui(new Ui::DelegationItemWidget),
    m_platfromStyle(platformStyle),
    m_type(type),
    m_position(-1),
    m_model(0)

{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(type);
    ui->buttonSplit->setIcon(platformStyle->MultiStatesIcon(":/icons/tx_inout", PlatformStyle::PushButtonIcon));
    ui->buttonRemove->setIcon(platformStyle->MultiStatesIcon(":/icons/remove_entry", PlatformStyle::PushButtonIcon));
    ui->buttonAdd->setIcon(platformStyle->MultiStatesIcon(":/icons/plus_full", PlatformStyle::PushButtonIcon));
    ui->delegationLogo->setPixmap(platformStyle->MultiStatesIcon(m_type == New ? ":/icons/export" : ":/icons/staking_off").pixmap(DELEGATION_ITEM_ICONSIZE, DELEGATION_ITEM_ICONSIZE));
    d = new DelegationItemWidgetPriv();
}

DelegationItemWidget::~DelegationItemWidget()
{
    delete ui;
    delete d;
}

void DelegationItemWidget::setData(const QString &fee, const QString &staker, const QString &address, const int32_t &blockHight, const int64_t &balance, const int64_t &stake)
{
    // Set data
    d->fee = fee;
    d->staker = staker;
    d->address = address;
    d->blockHight = blockHight;
    d->balance = balance;
    d->stake = stake;

    // Update GUI
    if(d->fee != ui->labelFee->text())
        ui->labelFee->setText(d->fee);
    if(d->staker != ui->labelStaker->text())
        ui->labelStaker->setText(d->staker);
    if(d->address != ui->labelAddress->text())
        ui->labelAddress->setText(d->address);
    QString filename = d->blockHight > 0 ? ":/icons/staking_on" : ":/icons/staking_off";
    if(m_filename != filename)
    {
        m_filename = filename;
        updateLogo();
    }
    updateBalance();
}

void DelegationItemWidget::setPosition(int position)
{
    m_position = position;
}

void DelegationItemWidget::on_buttonAdd_clicked()
{
    Q_EMIT clicked(m_position, Buttons::Add);
}

void DelegationItemWidget::on_buttonRemove_clicked()
{
    Q_EMIT clicked(m_position, Buttons::Remove);
}

void DelegationItemWidget::on_buttonSplit_clicked()
{
    Q_EMIT clicked(m_position, Buttons::Split);
}

int DelegationItemWidget::position() const
{
    return m_position;
}

void DelegationItemWidget::updateLogo()
{
    QPixmap pixmap = m_platfromStyle->MultiStatesIcon(m_filename).pixmap(DELEGATION_ITEM_ICONSIZE, DELEGATION_ITEM_ICONSIZE);
    ui->delegationLogo->setPixmap(pixmap);
}

void DelegationItemWidget::setModel(WalletModel *_model)
{
    m_model = _model;
    if(m_model && m_model->getOptionsModel())
    {
        connect(m_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &DelegationItemWidget::updateDisplayUnit);
    }
    updateDisplayUnit();
}

void DelegationItemWidget::updateDisplayUnit()
{
    updateBalance();
}

void DelegationItemWidget::updateBalance()
{
    int unit = BitcoinUnits::BTC;
    if(m_model && m_model->getOptionsModel())
        unit = m_model->getOptionsModel()->getDisplayUnit();
    ui->labelAssets->setText(BitcoinUnits::formatWithUnit(unit, d->balance, false, BitcoinUnits::separatorAlways));
    ui->labelStake->setText(BitcoinUnits::formatWithUnit(unit, d->stake, false, BitcoinUnits::separatorAlways));
}
