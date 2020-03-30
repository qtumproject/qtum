#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/superstakeritemwidget.h>
#include <qt/platformstyle.h>
#include <qt/forms/ui_superstakeritemwidget.h>

#include <QFile>

#define SUPERSTAKER_ITEM_ICONSIZE 24
SuperStakerItemWidget::SuperStakerItemWidget(const PlatformStyle *platformStyle, QWidget *parent, ItemType type) :
    QWidget(parent),
    ui(new Ui::SuperStakerItemWidget),
    m_platfromStyle(platformStyle),
    m_type(type),
    m_position(-1)

{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(type);
    ui->buttonConfig->setIcon(platformStyle->MultiStatesIcon(":/icons/configure", PlatformStyle::PushButtonIcon));
    ui->buttonRemove->setIcon(platformStyle->MultiStatesIcon(":/icons/remove_entry", PlatformStyle::PushButtonIcon));
    ui->buttonAdd->setIcon(platformStyle->MultiStatesIcon(":/icons/plus_full", PlatformStyle::PushButtonIcon));
    ui->superStakerLogo->setPixmap(platformStyle->MultiStatesIcon(m_type == New ? ":/icons/export" : ":/icons/staking_off").pixmap(SUPERSTAKER_ITEM_ICONSIZE, SUPERSTAKER_ITEM_ICONSIZE));
}

SuperStakerItemWidget::~SuperStakerItemWidget()
{
    delete ui;
}

void SuperStakerItemWidget::setData(const QString &fee, const QString &staker, const bool &staking_on)
{
    if(fee != ui->labelFee->text())
        ui->labelFee->setText(fee);
    if(staker != ui->labelStaker->text())
        ui->labelStaker->setText(staker);
    QString filename = staking_on ? ":/icons/staking_on" : ":/icons/staking_off";
    if(m_filename != filename)
    {
        m_filename = filename;
        updateLogo();
    }
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

int SuperStakerItemWidget::position() const
{
    return m_position;
}

void SuperStakerItemWidget::updateLogo()
{
    QPixmap pixmap = m_platfromStyle->MultiStatesIcon(m_filename).pixmap(SUPERSTAKER_ITEM_ICONSIZE, SUPERSTAKER_ITEM_ICONSIZE);
    ui->superStakerLogo->setPixmap(pixmap);
}
