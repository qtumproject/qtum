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

    ui->cbLedgerApp->addItem(tr("Qtum Wallet Nano S"), InstallDevice::WalletNanoS);
    ui->cbLedgerApp->addItem(tr("Qtum Stake Nano S"), InstallDevice::StakeNanoS);

    ui->labelApp->setStyleSheet("QLabel { color: red; }");
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
    int deviceType = ui->cbLedgerApp->currentData().toInt();
    switch (deviceType) {
    case InstallDevice::WalletNanoS:
        return InstallDevice::WalletNanoS;
    case InstallDevice::StakeNanoS:
        return InstallDevice::StakeNanoS;
    default:
        break;
    }

    return InstallDevice::WalletNanoS;
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

    if(errorMessage.contains("verify that the right application is opened", Qt::CaseInsensitive))
    {
        message = tr("Please close the Qtum application on your ledger.");
        return true;
    }

    message = d->tool->errorMessage();
    return true;
}

void QtumLedgerInstallerDialog::on_cbLedgerApp_currentIndexChanged(int index)
{
    int deviceType = index;
    switch (deviceType) {
    case InstallDevice::WalletNanoS:
        return ui->labelApp->setText("");
    case InstallDevice::StakeNanoS:
        return ui->labelApp->setText(tr("When Qtum Stake is installed, please turn off the auto lock:\n"
                                        "Nano S > Settings > Security > Auto-lock > OFF\n"
                                        "\n"
                                        "When Qtum Stake is removed, please turn on the auto lock:\n"
                                        "Nano S > Settings > Security > Auto-lock > 10 minutes\n"));
    default:
        break;
    }
}
