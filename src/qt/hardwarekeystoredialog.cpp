#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/hardwarekeystoredialog.h>
#include <qt/forms/ui_hardwarekeystoredialog.h>

HardwareKeystoreDialog::HardwareKeystoreDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HardwareKeystoreDialog)
{
    ui->setupUi(this);
}

HardwareKeystoreDialog::~HardwareKeystoreDialog()
{
    delete ui;
}
