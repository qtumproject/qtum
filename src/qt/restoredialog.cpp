#include "restoredialog.h"
#include "ui_restoredialog.h"
#include "guiutil.h"
#include "walletmodel.h"

RestoreDialog::RestoreDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RestoreDialog),
    model(0)
{
    ui->setupUi(this);
}

RestoreDialog::~RestoreDialog()
{
    delete ui;
}

QString RestoreDialog::getParam()
{
    return ui->rbReindex->isChecked() ? "-reindex" : "-salvagewallet";
}

QString RestoreDialog::getFileName()
{
    return ui->txtWalletPath->text();
}

void RestoreDialog::setModel(WalletModel *model)
{
    this->model = model;
}

void RestoreDialog::on_btnReset_clicked()
{
    ui->txtWalletPath->setText("");
    ui->rbReindex->setChecked(true);
}

void RestoreDialog::on_btnBoxRestore_accepted()
{
    if(model)
    {
        if(model->restoreWallet(getFileName(), getParam()))
        {
            QApplication::quit();
        }
    }
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
