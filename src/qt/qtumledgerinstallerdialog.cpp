#include <qt/qtumledgerinstallerdialog.h>
#include <qt/forms/ui_qtumledgerinstallerdialog.h>
#include <qt/waitmessagebox.h>

#include <QVariant>
#include <QMessageBox>

class QtumLedgerInstallerDialogPriv
{
public:
    QtumLedgerInstallerDialogPriv(QObject *parent)
    {
        tool = new QtumHwiTool(parent);
    }

    QtumHwiTool* tool = 0;
    bool ret = false;
};

QtumLedgerInstallerDialog::QtumLedgerInstallerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QtumLedgerInstallerDialog)
{
    ui->setupUi(this);
    d = new QtumLedgerInstallerDialogPriv(this);

    ui->cbLedgerType->addItem(tr("Ledger Nano S"), InstallDevice::NanoS);
}

QtumLedgerInstallerDialog::~QtumLedgerInstallerDialog()
{
    delete ui;
    delete d;
}

void QtumLedgerInstallerDialog::on_addButton_clicked()
{
    // Install Qtum app from ledger
    WaitMessageBox dlg(tr("Ledger Status"), tr("Confirm Qtum install on your Ledger device..."), [this]() {
        d->ret = d->tool->installApp(getDeviceType());
    }, this);

    dlg.exec();

    QString message;
    if(!d->ret && parseErrorMessage(message))
    {
        QMessageBox::warning(this, tr("Install problem"), message);
    }
    else
    {
        QDialog::accept();
    }
}

void QtumLedgerInstallerDialog::on_removeButton_clicked()
{
    // Remove Qtum app from ledger
    WaitMessageBox dlg(tr("Ledger Status"), tr("Confirm Qtum removal on your Ledger device..."), [this]() {
        d->ret = d->tool->removeApp(getDeviceType());
    }, this);

    dlg.exec();

    QString message;
    if(!d->ret && parseErrorMessage(message))
    {
        QMessageBox::warning(this, tr("Remove problem"), message);
    }
    else
    {
        QDialog::accept();
    }
}

void QtumLedgerInstallerDialog::on_cancelButton_clicked()
{
    QDialog::reject();
}

InstallDevice::DeviceType QtumLedgerInstallerDialog::getDeviceType()
{
    int deviceType = ui->cbLedgerType->currentData().toInt();
    switch (deviceType) {
    case InstallDevice::NanoS:
        return InstallDevice::NanoS;
    default:
        break;
    }

    return InstallDevice::NanoS;
}

bool QtumLedgerInstallerDialog::parseErrorMessage(QString &message)
{
    QString errorMessage = d->tool->errorMessage();
    if(errorMessage.contains("denied by the user", Qt::CaseInsensitive))
        return false;

    if(errorMessage.contains("ModuleNotFoundError", Qt::CaseInsensitive) && errorMessage.contains("ledgerblue", Qt::CaseInsensitive))
    {
        message = tr("Ledger loader not found, you can install it with the command:") + "\npip3 install ledgerblue";
        return true;
    }

    if(errorMessage.contains("No dongle found", Qt::CaseInsensitive))
    {
        message = tr("Please insert your Ledger. Verify the cable is connected and that no other application is using it.");
        return true;
    }

    message = d->tool->errorMessage();
    return true;
}
