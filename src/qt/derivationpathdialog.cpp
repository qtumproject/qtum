#include <qt/derivationpathdialog.h>
#include <qt/forms/ui_derivationpathdialog.h>

DerivationPathDialog::DerivationPathDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DerivationPathDialog)
{
    ui->setupUi(this);

    // Connect signal and slots
    QObject::connect(ui->cbRescan, SIGNAL(clicked()), this, SLOT(updateButtons()));
    QObject::connect(ui->cbLegacy, SIGNAL(clicked()), this, SLOT(updateButtons()));
    QObject::connect(ui->cbP2SH, SIGNAL(clicked()), this, SLOT(updateButtons()));
    QObject::connect(ui->cbSegWit, SIGNAL(clicked()), this, SLOT(updateButtons()));
    updateButtons();
}

DerivationPathDialog::~DerivationPathDialog()
{
    delete ui;
}

void DerivationPathDialog::on_cancelButton_clicked()
{
    QDialog::reject();
}

void DerivationPathDialog::on_okButton_clicked()
{
    QDialog::accept();
}

bool DerivationPathDialog::importAddressesData(bool &rescan, bool &importPKH, bool &importP2SH, bool &importBech32)
{
    rescan = ui->cbRescan->isChecked();
    importPKH = ui->cbLegacy->isChecked();
    importP2SH = ui->cbP2SH->isChecked();
    importBech32 = ui->cbSegWit->isChecked();
    return rescan || importPKH || importP2SH || importBech32;
}

void DerivationPathDialog::updateButtons()
{
    bool enabled = ui->cbRescan->isChecked() || ui->cbLegacy->isChecked() || ui->cbP2SH->isChecked() || ui->cbSegWit->isChecked();
    ui->okButton->setEnabled(enabled);
}
