#include "qt/qtumledgerinstallerdialog.h"
#include "qt/forms/ui_qtumledgerinstallerdialog.h"

QtumLedgerInstallerDialog::QtumLedgerInstallerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QtumLedgerInstallerDialog)
{
    ui->setupUi(this);

    ui->cbLedgerType->addItem(tr("Ledger Nano S"), NanoS);
}

QtumLedgerInstallerDialog::~QtumLedgerInstallerDialog()
{
    delete ui;
}

void QtumLedgerInstallerDialog::on_addButton_clicked()
{

}

void QtumLedgerInstallerDialog::on_removeButton_clicked()
{

}

void QtumLedgerInstallerDialog::on_cancelButton_clicked()
{
    QDialog::reject();
}
