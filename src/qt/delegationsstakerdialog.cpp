#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/delegationsstakerdialog.h>
#include <qt/forms/ui_delegationsstakerdialog.h>

#include <qt/stakerdelegationview.h>

DelegationsStakerDialog::DelegationsStakerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DelegationsStakerDialog),
    model(0),
    m_stakerDelegationView(0)
{
    ui->setupUi(this);

    setWindowTitle(tr("Delegate for super staker"));

    m_stakerDelegationView = new StakerDelegationView(this);
    m_stakerDelegationView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->delegationViewLayout->addWidget(m_stakerDelegationView);

}

DelegationsStakerDialog::~DelegationsStakerDialog()
{
    delete ui;
}

void DelegationsStakerDialog::setModel(WalletModel *_model)
{
    this->model = _model;

    if(_model) {
         m_stakerDelegationView->setModel(_model);
    }
}

void DelegationsStakerDialog::setSuperStakerData(const QString &_address, const QString &_hash)
{
    address = _address;
    hash = _hash;
}
