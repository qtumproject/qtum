#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/delegationsstakerdialog.h>
#include <qt/forms/ui_delegationsstakerdialog.h>
#include <qt/stakerdelegationview.h>


class DelegationsStakerDialogPriv
{
public:
    DelegationsStakerDialogPriv():
        fee()
    {}

    QString address;
    QString name;
    int fee;
    QString hash;
};

DelegationsStakerDialog::DelegationsStakerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DelegationsStakerDialog),
    model(0),
    m_stakerDelegationView(0)
{
    ui->setupUi(this);
    d = new DelegationsStakerDialogPriv();

    setWindowTitle(tr("Delegations for super staker"));

    m_stakerDelegationView = new StakerDelegationView(this);
    m_stakerDelegationView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->delegationViewLayout->addWidget(m_stakerDelegationView);
    connect(ui->okButton, &QPushButton::clicked, this, &DelegationsStakerDialog::accept);
}

DelegationsStakerDialog::~DelegationsStakerDialog()
{
    delete ui;
    delete d;
}

void DelegationsStakerDialog::setModel(WalletModel *_model)
{
    this->model = _model;

    if(_model) {
         m_stakerDelegationView->setModel(_model);
    }
}

void DelegationsStakerDialog::setSuperStakerData(const QString& _name, const QString &_address, const int &_fee, const QString &_hash)
{
    if(d->hash == _hash)
        return;

    d->name = _name;
    d->address = _address;
    d->fee = _fee;
    d->hash = _hash;

    updateData();
}

void DelegationsStakerDialog::updateData()
{
    ui->txtStaker->setText(d->name);
    m_stakerDelegationView->setSuperStakerData(d->address, d->fee);
}
