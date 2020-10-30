#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/hardwarekeystoredialog.h>
#include <qt/forms/ui_hardwarekeystoredialog.h>
#include <QRadioButton>
#include <QVBoxLayout>

class HardwareKeystoreDialogPriv
{
public:
    QList<QRadioButton*> devices;
};

HardwareKeystoreDialog::HardwareKeystoreDialog(const QStringList& devices, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HardwareKeystoreDialog)
{
    ui->setupUi(this);

    d = new HardwareKeystoreDialogPriv();
    QVBoxLayout* boxLayout = new QVBoxLayout(ui->groupBoxDevices);
    for(QString deviceName : devices)
    {
        QRadioButton *deviceWidget = new QRadioButton(deviceName, this);
        boxLayout->addWidget(deviceWidget);
        d->devices.push_back(deviceWidget);
    }
    boxLayout->addStretch(1);
    setCurrentIndex(0);
}

HardwareKeystoreDialog::~HardwareKeystoreDialog()
{
    delete ui;
    delete d;
}

int HardwareKeystoreDialog::currentIndex() const
{
    for(int index = 0; index < d->devices.size(); index++)
    {
        if(d->devices[index]->isChecked())
            return index;
    }
    return -1;
}

void HardwareKeystoreDialog::setCurrentIndex(int index)
{
    if(index >=0 && index < d->devices.size())
    {
        d->devices[index]->setChecked(true);
    }
}
