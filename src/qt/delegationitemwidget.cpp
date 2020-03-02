#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/delegationitemwidget.h>
#include <qt/platformstyle.h>
#include <qt/forms/ui_delegationitemwidget.h>

#include <QFile>

#define DELEGATION_ITEM_ICONSIZE 24
DelegationItemWidget::DelegationItemWidget(const PlatformStyle *platformStyle, QWidget *parent, ItemType type) :
    QWidget(parent),
    ui(new Ui::DelegationItemWidget),
    m_platfromStyle(platformStyle),
    m_type(type),
    m_position(-1)

{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(type);
    ui->buttonRemove->setIcon(platformStyle->MultiStatesIcon(":/icons/remove_entry", PlatformStyle::PushButtonIcon));
    ui->buttonAdd->setIcon(platformStyle->MultiStatesIcon(":/icons/plus_full", PlatformStyle::PushButtonIcon));
    ui->delegationLogo->setPixmap(platformStyle->MultiStatesIcon(m_type == New ? ":/icons/export" : ":/icons/staking_off").pixmap(DELEGATION_ITEM_ICONSIZE, DELEGATION_ITEM_ICONSIZE));
}

DelegationItemWidget::~DelegationItemWidget()
{
    delete ui;
}

void DelegationItemWidget::setData(const QString &fee, const QString &staker, const QString &address, const int32_t &blockHight)
{
    if(fee != ui->labelFee->text())
        ui->labelFee->setText(fee);
    if(staker != ui->labelStaker->text())
        ui->labelStaker->setText(staker);
    if(address != ui->labelAddress->text())
        ui->labelAddress->setText(address);
    QString filename = blockHight > 0 ? ":/icons/staking_on" : ":/icons/staking_off";
    if(m_filename != filename)
    {
        m_filename = filename;
        updateLogo();
    }
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

int DelegationItemWidget::position() const
{
    return m_position;
}

void DelegationItemWidget::updateLogo()
{
    QPixmap pixmap = m_platfromStyle->MultiStatesIcon(m_filename).pixmap(DELEGATION_ITEM_ICONSIZE, DELEGATION_ITEM_ICONSIZE);
    ui->delegationLogo->setPixmap(pixmap);
}
