#include <qt/qtumledgerinstallerdialog.h>
#include <qt/forms/ui_qtumledgerinstallerdialog.h>

#include <QVariant>

class QtumLedgerInstallerDialogPriv
{
public:
    QtumLedgerInstallerDialogPriv(QObject *parent)
    {
        tool = new QtumHwiTool(parent);
    }

    QtumHwiTool* tool = 0;
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
    d->tool->installApp(getDeviceType());
    QDialog::accept();
}

void QtumLedgerInstallerDialog::on_removeButton_clicked()
{
    d->tool->removeApp(getDeviceType());
    QDialog::accept();
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
