#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/delegationitemwidget.h>
#include <qt/platformstyle.h>
#include <qt/forms/ui_delegationitemwidget.h>
#include <qt/bitcoinunits.h>
#include <qt/optionsmodel.h>
#include <qt/walletmodel.h>
#include <qt/delegationitemmodel.h>
#include <interfaces/node.h>
#include <chainparams.h>
#include <rpc/server.h>
#include <qt/guiutil.h>

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
    int64_t weight = 0;
    bool staking = false;
    int32_t status = 0;
    QLabel* light = 0;
};

#define DELEGATION_ITEM_ICONSIZE 24
#define LIGHT_ICONSIZE 14
const QString LIGHT_STYLE = "QLabel{background-color: %1; border-radius: 7px; border: 2px solid transparent;}";
#define DELEGATION_STAKER_SIZE 210
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
    ui->buttonSplit->setIcon(platformStyle->MultiStatesIcon(":/icons/split", PlatformStyle::PushButtonIcon));
    ui->buttonRemove->setIcon(platformStyle->MultiStatesIcon(":/icons/remove_entry", PlatformStyle::PushButtonIcon));
    ui->buttonAdd->setIcon(platformStyle->MultiStatesIcon(":/icons/plus_full", PlatformStyle::PushButtonIcon));
    ui->buttonRestore->setIcon(platformStyle->MultiStatesIcon(":/icons/restore", PlatformStyle::PushButtonIcon));
    ui->delegationLogo->setPixmap(platformStyle->MultiStatesIcon(m_type == New ? ":/icons/delegate" : ":/icons/staking_off").pixmap(DELEGATION_ITEM_ICONSIZE, DELEGATION_ITEM_ICONSIZE));

    ui->buttonSplit->setToolTip(tr("Split coins for offline staking."));
    ui->buttonRemove->setToolTip(tr("Remove delegation."));
    ui->buttonAdd->setToolTip(tr("Add delegation."));
    ui->buttonRestore->setToolTip(tr("Restore delegations."));

    d = new DelegationItemWidgetPriv();

    if(m_type == Record)
    {
        d->light = new QLabel();
        d->light->setFixedSize(LIGHT_ICONSIZE, LIGHT_ICONSIZE);
        d->light->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        QVBoxLayout *layout = new QVBoxLayout;
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(d->light, 0, Qt::AlignRight|Qt::AlignBottom);
        ui->delegationLogo->setLayout(layout);
        setLight(Transparent);
    }
}

DelegationItemWidget::~DelegationItemWidget()
{
    delete ui;
    delete d;
}

void DelegationItemWidget::setData(const QString &fee, const QString &staker, const QString &address, const int32_t &blockHight, const int64_t &balance, const int64_t &stake, const int64_t &weight, const int32_t &status)
{
    // Set data
    d->fee = fee;
    d->staker = staker;
    d->address = address;
    d->blockHight = blockHight;
    d->balance = balance;
    d->stake = stake;
    d->weight = weight;
    d->status = status;

    // Update GUI
    if(d->fee != ui->labelFee->text())
        ui->labelFee->setText(d->fee);
    if(d->staker != ui->labelStaker->toolTip())
        updateLabelStaker();
    if(d->address != ui->labelAddress->text())
        ui->labelAddress->setText(d->address);
    d->staking = (d->blockHight > 0 && d->weight > 0);
    updateLogo();
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

void DelegationItemWidget::on_buttonRestore_clicked()
{
    Q_EMIT clicked(m_position, Buttons::Restore);
}

int DelegationItemWidget::position() const
{
    return m_position;
}

void DelegationItemWidget::updateLogo()
{
    if(!m_model)
        return;

    if(m_model->node().shutdownRequested())
        return;

    QString filename = d->staking ? ":/icons/staking_on" : ":/icons/staking_off";
    if(m_filename != filename)
    {
        m_filename = filename;
        QPixmap pixmap = m_platfromStyle->MultiStatesIcon(m_filename).pixmap(DELEGATION_ITEM_ICONSIZE, DELEGATION_ITEM_ICONSIZE);
        ui->delegationLogo->setPixmap(pixmap);
    }

    uint64_t nWeight = d->weight;
    if (d->staking)
    {
        uint64_t nNetworkWeight = GetPoSKernelPS();
        static int64_t nTargetSpacing = Params().GetConsensus().nPowTargetSpacing;

        unsigned nEstimateTime = nTargetSpacing * nNetworkWeight / nWeight;

        QString text;
        if (nEstimateTime < 60)
        {
            text = tr("%n second(s)", "", nEstimateTime);
        }
        else if (nEstimateTime < 60*60)
        {
            text = tr("%n minute(s)", "", nEstimateTime/60);
        }
        else if (nEstimateTime < 24*60*60)
        {
            text = tr("%n hour(s)", "", nEstimateTime/(60*60));
        }
        else
        {
            text = tr("%n day(s)", "", nEstimateTime/(60*60*24));
        }

        nWeight /= COIN;
        nNetworkWeight /= COIN;

        ui->delegationLogo->setToolTip(tr("Offline staking.<br>Your weight is %1<br>Network weight is %2<br>Expected time to earn reward is %3").arg(nWeight).arg(nNetworkWeight).arg(text));
    }
    else
    {
        if(d->blockHight < 0)
            ui->delegationLogo->setToolTip(tr("Not staking because the delegation is not confirmed"));
        else if (!nWeight)
            ui->delegationLogo->setToolTip(tr("Not staking because you don't have mature coins"));
        else
            ui->delegationLogo->setToolTip(tr("Not staking"));
    }

    switch (d->status)
    {
    case DelegationItemModel::CreateTxConfirmed:
        setLight(Green);
        d->light->setToolTip(tr("Create transaction confirmed"));
        break;
    case DelegationItemModel::CreateTxNotConfirmed:
        setLight(Orange);
        d->light->setToolTip(tr("Create transaction not confirmed"));
        break;
    case DelegationItemModel::CreateTxError:
        setLight(Red);
        d->light->setToolTip(tr("Create transaction error"));
        break;
    case DelegationItemModel::RemoveTxNotConfirmed:
        setLight(Orange);
        d->light->setToolTip(tr("Remove transaction not confirmed"));
        break;
    case DelegationItemModel::RemoveTxError:
        setLight(Red);
        d->light->setToolTip(tr("Remove transaction error"));
        break;
    default:
        setLight(Transparent);
        break;
    }
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

void DelegationItemWidget::setLight(DelegationItemWidget::LightType type)
{
    QString lightColor;
    switch (type) {
    case Red:
        lightColor = "#DC143C";
        break;
    case Orange:
        lightColor = "#FFA500";
        break;
    case Green:
        lightColor = "#32CD32";
        break;
    default:
        break;
    }

    if(d && d->light)
    {
        if(lightColor != "")
        {
            d->light->setStyleSheet(LIGHT_STYLE.arg(lightColor));
            d->light->setVisible(true);
        }
        else
        {
            d->light->setVisible(false);
        }
    }
}

void DelegationItemWidget::updateLabelStaker()
{
    QString text = d->staker;
    QFontMetrics fm = ui->labelStaker->fontMetrics();
    for(int i = d->staker.length(); i>3; i--)
    {
        text = GUIUtil::cutString(d->staker, i);
        if(GUIUtil::TextWidth(fm, text) < DELEGATION_STAKER_SIZE)
            break;
    }
    ui->labelStaker->setText(text);
    ui->labelStaker->setToolTip(d->staker);
}
