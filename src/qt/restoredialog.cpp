#include <qt/restoredialog.h>
#include <qt/forms/ui_restoredialog.h>
#include <qt/guiutil.h>
#include <qt/walletmodel.h>
#include <QMessageBox>
#include <QFile>
#include <qt/styleSheet.h>

RestoreDialog::RestoreDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RestoreDialog),
    model(0)
{
    ui->setupUi(this);
    SetObjectStyleSheet(ui->btnReset, StyleSheetNames::ButtonWhite);
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
    QString filename = getFileName();
    QString param = getParam();
    if(model && QFile::exists(filename))
    {
        QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm wallet restoration"),
                 tr("Warning: The wallet will be restored from location <b>%1</b> and restarted with parameter <b>%2</b>.").arg(filename, param)
                 + tr("<br><br>Are you sure you wish to restore your wallet?"),
                 QMessageBox::Yes|QMessageBox::Cancel,
                 QMessageBox::Cancel);
        if(retval == QMessageBox::Yes)
        {
            if(model->restoreWallet(getFileName(), getParam()))
            {
                QApplication::quit();
            }
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
