#include <qt/stakepage.h>
#include <qt/forms/ui_stakepage.h>

#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/platformstyle.h>
#include <qt/transactionfilterproxy.h>
#include <qt/transactiontablemodel.h>
#include <qt/walletmodel.h>
#include <interfaces/wallet.h>
#include <qt/transactiondescdialog.h>
#include <qt/styleSheet.h>

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
}

StakePage::~StakePage()
{
    delete ui;
}

void StakePage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
}

void StakePage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
}

void StakePage::setBalance(const interfaces::WalletBalances& balances)
{
}
