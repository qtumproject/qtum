#include <qt/hardwaredevicedialog.h>
#include <qt/forms/ui_hardwaredevicedialog.h>

HardwareDeviceDialog::HardwareDeviceDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HardwareDeviceDialog)
{
    ui->setupUi(this);
}

HardwareDeviceDialog::~HardwareDeviceDialog()
{
    delete ui;
}

void HardwareDeviceDialog::on_cancelButton_clicked()
{
    QDialog::reject();
}
