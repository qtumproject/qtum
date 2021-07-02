#include <qt/hardwaredevicedialog.h>
#include <qt/forms/ui_hardwaredevicedialog.h>
#include <QtGlobal>

HardwareDeviceDialog::HardwareDeviceDialog(const QString& debugMessage, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HardwareDeviceDialog)
{
    ui->setupUi(this);
    ui->textEditDebugMessage->setText(debugMessage);
#ifdef Q_OS_LINUX
    ui->labelLinuxPermissions->setVisible(true);
#else
    ui->labelLinuxPermissions->setVisible(false);
#endif
}

HardwareDeviceDialog::~HardwareDeviceDialog()
{
    delete ui;
}

void HardwareDeviceDialog::on_cancelButton_clicked()
{
    QDialog::reject();
}

void HardwareDeviceDialog::on_nextButton_clicked()
{
    QDialog::accept();
}
