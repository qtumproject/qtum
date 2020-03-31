#include <qt/superstakerconfigdialog.h>
#include <qt/forms/ui_superstakerconfigdialog.h>
#include <qt/walletmodel.h>
#include <qt/clientmodel.h>

SuperStakerConfigDialog::SuperStakerConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SuperStakerConfigDialog)
{
    ui->setupUi(this);
}

SuperStakerConfigDialog::~SuperStakerConfigDialog()
{
    delete ui;
}

void SuperStakerConfigDialog::setModel(WalletModel *_model)
{
    m_model = _model;
}

void SuperStakerConfigDialog::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;
}

void SuperStakerConfigDialog::setSuperStakerData(const QString &_address, const QString &_hash)
{
    address = _address;
    hash = _hash;
}
