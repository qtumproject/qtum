#include "restoredialog.h"
#include "ui_restoredialog.h"
#include "guiutil.h"

RestoreDialog::RestoreDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RestoreDialog)
{
    ui->setupUi(this);
}

RestoreDialog::~RestoreDialog()
{
    delete ui;
}

void RestoreDialog::on_btnReset_clicked()
{
    ui->txtWalletPath->setText("");
    ui->rbReindex->setChecked(true);
}

void RestoreDialog::on_btnBoxRestore_accepted()
{
    accept();
}

void RestoreDialog::on_btnBoxRestore_rejected()
{
    reject();
}

void RestoreDialog::on_toolWalletPath_clicked()
{
    QString filename = GUIUtil::getOpenFileName(this,
        tr("Restore Wallet"), QString(),
        tr("Wallet Data (*.dat)"), NULL);

    if (filename.isEmpty())
        return;

    ui->txtWalletPath->setText(filename);
}
