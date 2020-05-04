#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/superstakeritemwidget.h>
#include <qt/platformstyle.h>
#include <qt/forms/ui_superstakeritemwidget.h>
#include <qt/bitcoinunits.h>
#include <qt/optionsmodel.h>
#include <qt/walletmodel.h>

#include <QFile>

class SuperStakerItemWidgetPriv
{
public:
    QString fee;
    QString staker;
    bool staking_on = false;
    int64_t balance = 0;
    int64_t stake = 0;
};

#define SUPERSTAKER_ITEM_ICONSIZE 24
SuperStakerItemWidget::SuperStakerItemWidget(const PlatformStyle *platformStyle, QWidget *parent, ItemType type) :
    QWidget(parent),
    ui(new Ui::SuperStakerItemWidget),
    m_platfromStyle(platformStyle),
    m_type(type),
    m_position(-1),
    m_model(0)

{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(type);
    ui->buttonSplit->setIcon(platformStyle->MultiStatesIcon(":/icons/tx_inout", PlatformStyle::PushButtonIcon));
    ui->buttonConfig->setIcon(platformStyle->MultiStatesIcon(":/icons/configure", PlatformStyle::PushButtonIcon));
    ui->buttonRemove->setIcon(platformStyle->MultiStatesIcon(":/icons/remove_entry", PlatformStyle::PushButtonIcon));
    ui->buttonAdd->setIcon(platformStyle->MultiStatesIcon(":/icons/plus_full", PlatformStyle::PushButtonIcon));
    ui->superStakerLogo->setPixmap(platformStyle->MultiStatesIcon(m_type == New ? ":/icons/import" : ":/icons/staking_off").pixmap(SUPERSTAKER_ITEM_ICONSIZE, SUPERSTAKER_ITEM_ICONSIZE));
    d = new SuperStakerItemWidgetPriv();
}

SuperStakerItemWidget::~SuperStakerItemWidget()
{
    delete ui;
}

void SuperStakerItemWidget::setData(const QString &fee, const QString &staker, const bool &staking_on, const int64_t &balance, const int64_t &stake)
{
    // Set data
    d->fee = fee;
    d->staker = staker;
    d->staking_on = staking_on;
    d->balance = balance;
    d->stake = stake;

    // Update GUI
    if(d->fee != ui->labelFee->text())
        ui->labelFee->setText(d->fee);
    if(d->staker != ui->labelStaker->text())
        ui->labelStaker->setText(d->staker);
    QString filename = d->staking_on ? ":/icons/staking_on" : ":/icons/staking_off";
    if(m_filename != filename)
    {
        m_filename = filename;
        updateLogo();
    }
    updateBalance();
}

void SuperStakerItemWidget::setPosition(int position)
{
    m_position = position;
}

void SuperStakerItemWidget::on_buttonAdd_clicked()
{
    Q_EMIT clicked(m_position, Buttons::Add);
}

void SuperStakerItemWidget::on_buttonRemove_clicked()
{
    Q_EMIT clicked(m_position, Buttons::Remove);
}

void SuperStakerItemWidget::on_buttonConfig_clicked()
{
    Q_EMIT clicked(m_position, Buttons::Config);
}

void SuperStakerItemWidget::on_buttonDelegations_clicked()
{
    Q_EMIT clicked(m_position, Buttons::Delegations);
}

void SuperStakerItemWidget::on_buttonSplit_clicked()
{
    Q_EMIT clicked(m_position, Buttons::Split);
}

int SuperStakerItemWidget::position() const
{
    return m_position;
}

void SuperStakerItemWidget::updateLogo()
{
    QPixmap pixmap = m_platfromStyle->MultiStatesIcon(m_filename).pixmap(SUPERSTAKER_ITEM_ICONSIZE, SUPERSTAKER_ITEM_ICONSIZE);
    ui->superStakerLogo->setPixmap(pixmap);
}

void SuperStakerItemWidget::setModel(WalletModel *_model)
{
    m_model = _model;
    if(m_model && m_model->getOptionsModel())
    {
        connect(m_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &SuperStakerItemWidget::updateDisplayUnit);
    }
    updateDisplayUnit();
}

void SuperStakerItemWidget::updateDisplayUnit()
{
    updateBalance();
}

void SuperStakerItemWidget::updateBalance()
{
    int unit = BitcoinUnits::BTC;
    if(m_model && m_model->getOptionsModel())
        unit = m_model->getOptionsModel()->getDisplayUnit();
    ui->labelAssets->setText(BitcoinUnits::formatWithUnit(unit, d->balance, false, BitcoinUnits::separatorAlways));
    ui->labelStake->setText(BitcoinUnits::formatWithUnit(unit, d->stake, false, BitcoinUnits::separatorAlways));
}
