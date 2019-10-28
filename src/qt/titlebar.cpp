#include <qt/titlebar.h>
#include <qt/forms/ui_titlebar.h>
#include <qt/bitcoinunits.h>
#include <qt/optionsmodel.h>
#include <qt/tabbarinfo.h>

#include <QPixmap>
#include <qt/platformstyle.h>

namespace TitleBar_NS {
const int titleHeight = 35;
}
using namespace TitleBar_NS;

TitleBar::TitleBar(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TitleBar)
{
    ui->setupUi(this);
    // Set size policy
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->tabWidget->setDrawBase(false);
    ui->tabWidget->setTabsClosable(true);
    setFixedHeight(titleHeight);
    m_iconCloseTab = platformStyle->MultiStatesIcon(":/icons/quit", PlatformStyle::PushButtonIcon);
    ui->lblBalance->setVisible(false);
}

TitleBar::~TitleBar()
{
    delete ui;
}

#ifdef ENABLE_WALLET
void TitleBar::setModel(WalletModel *model)
{
    m_model = model;
    if(m_models.count(m_model))
    {
        setBalanceLabel(m_models[m_model]);
    }
}
#endif

void TitleBar::setTabBarInfo(QObject *info)
{
    if(m_tab)
    {
        m_tab->detach();
    }

    if(info && info->inherits("TabBarInfo"))
    {
        TabBarInfo* tab = (TabBarInfo*)info;
        m_tab = tab;
        m_tab->attach(ui->tabWidget, &m_iconCloseTab);
    }
}

#ifdef ENABLE_WALLET
void TitleBar::setBalance(const interfaces::WalletBalances& balances)
{
    QObject* _model = sender();
    if(m_models.count(_model))
    {
        m_models[_model] = balances;
        if(_model == m_model)
        {
            setBalanceLabel(balances);
        }
    }
}
#endif

void TitleBar::on_navigationResized(const QSize &_size)
{
    ui->widgetLogo->setFixedWidth(_size.width());
}

#ifdef ENABLE_WALLET
void TitleBar::updateDisplayUnit()
{
    if(m_model && m_model->getOptionsModel())
    {
        ui->lblBalance->setText(BitcoinUnits::formatWithUnit(m_model->getOptionsModel()->getDisplayUnit(), m_models[m_model].balance));
    }
}
#endif

void TitleBar::setWalletSelector(QLabel *walletSelectorLabel, QComboBox *walletSelector)
{
    QLayout* layout = ui->widgetWallet->layout();

    if(walletSelectorLabel)
    {
        layout->addWidget(walletSelectorLabel);
    }

    if(walletSelector)
    {
        layout->addWidget(walletSelector);
    }
}

#ifdef ENABLE_WALLET
void TitleBar::addWallet(WalletModel *_model)
{
    if(_model)
    {
        m_models[_model] = _model->wallet().getBalances();
        connect(_model, &WalletModel::balanceChanged, this, &TitleBar::setBalance);
    }
}

void TitleBar::removeWallet(WalletModel *_model)
{
    if(_model)
    {
        disconnect(_model, &WalletModel::balanceChanged, this, &TitleBar::setBalance);
        m_models.erase(_model);
        if(m_models.size() == 0)
        {
            ui->lblBalance->setText("");
        }
    }
}

void TitleBar::setBalanceLabel(const interfaces::WalletBalances &balances)
{
    if(m_model && m_model->getOptionsModel())
    {
        ui->lblBalance->setText(BitcoinUnits::formatWithUnit(m_model->getOptionsModel()->getDisplayUnit(), balances.balance));
    }
}
#endif
