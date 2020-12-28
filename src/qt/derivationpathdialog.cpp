#include <qt/derivationpathdialog.h>
#include <qt/forms/ui_derivationpathdialog.h>

DerivationPathDialog::DerivationPathDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DerivationPathDialog)
{
    ui->setupUi(this);
}

DerivationPathDialog::~DerivationPathDialog()
{
    delete ui;
}

void DerivationPathDialog::on_backButton_clicked()
{
    QDialog::reject();
}
