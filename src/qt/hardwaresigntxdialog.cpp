#include <qt/hardwaresigntxdialog.h>
#include <qt/forms/ui_hardwaresigntxdialog.h>

HardwareSignTxDialog::HardwareSignTxDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HardwareSignTxDialog)
{
    ui->setupUi(this);
}

HardwareSignTxDialog::~HardwareSignTxDialog()
{
    delete ui;
}

void HardwareSignTxDialog::on_cancelButton_clicked()
{
    QDialog::reject();
}
