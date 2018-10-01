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
    ui(new Ui::TitleBar),
    m_model(0),
    m_tab(0)
{
    ui->setupUi(this);
    // Set size policy
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->tabWidget->setDrawBase(false);
    ui->tabWidget->setTabsClosable(true);
    setFixedHeight(titleHeight);
    m_iconCloseTab = platformStyle->TextColorIcon(":/icons/quit");
}

TitleBar::~TitleBar()
{
    delete ui;
}

void TitleBar::setModel(WalletModel *model)
{
    m_model = model;
    if(m_models.count(m_model))
    {
        setBalanceLabel(m_models[m_model]);
    }
}

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

void TitleBar::on_navigationResized(const QSize &_size)
{
    ui->widgetLogo->setFixedWidth(_size.width());
}

void TitleBar::setWalletSelector(QLabel *walletSelectorLabel, QComboBox *walletSelector)
{
    QLayout* layout = ui->widgetLogo->layout();

    if(walletSelectorLabel)
    {
        layout->addWidget(walletSelectorLabel);
    }

    if(walletSelector)
    {
        layout->addWidget(walletSelector);
    }

    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(spacer);
}

void TitleBar::addWallet(WalletModel *_model)
{
    if(_model)
    {
        m_models[_model] = _model->wallet().getBalances();
        connect(_model, SIGNAL(balanceChanged(interfaces::WalletBalances)), this, SLOT(setBalance(interfaces::WalletBalances)));
    }
}

void TitleBar::removeWallet(WalletModel *_model)
{
    if(_model)
    {
        disconnect(_model, SIGNAL(balanceChanged(interfaces::WalletBalances)), this, SLOT(setBalance(interfaces::WalletBalances)));
        m_models.erase(_model);
    }
}

void TitleBar::setBalanceLabel(const interfaces::WalletBalances &balances)
{
    if(m_model && m_model->getOptionsModel())
    {
        ui->lblBalance->setText(BitcoinUnits::formatWithUnit(m_model->getOptionsModel()->getDisplayUnit(), balances.balance));
    }
}
