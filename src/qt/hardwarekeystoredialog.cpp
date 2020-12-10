#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/hardwarekeystoredialog.h>
#include <qt/forms/ui_hardwarekeystoredialog.h>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QButtonGroup>
#include <qt/qtumhwitool.h>

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

    // Populate list with devices
    d = new HardwareKeystoreDialogPriv();
    QVBoxLayout* boxLayout = new QVBoxLayout(ui->groupBoxDevices);
    QButtonGroup* buttongroup = new QButtonGroup(ui->groupBoxDevices);
    for(QString deviceName : devices)
    {
        QRadioButton *deviceWidget = new QRadioButton(deviceName, this);
        boxLayout->addWidget(deviceWidget);
        d->devices.push_back(deviceWidget);
        buttongroup->addButton(deviceWidget);
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
    // Get the current device index
    for(int index = 0; index < d->devices.size(); index++)
    {
        if(d->devices[index]->isChecked())
            return index;
    }
    return -1;
}

void HardwareKeystoreDialog::setCurrentIndex(int index)
{
    // Set the current device index
    if(index >=0 && index < d->devices.size())
    {
        d->devices[index]->setChecked(true);
    }
}

bool HardwareKeystoreDialog::SelectDevice(QString &fingerprint, QWidget *parent)
{
    // Enumerate devices
    QtumHwiTool hwiTool(parent);
    QList<HWDevice> devices;
    if(hwiTool.enumerate(devices))
    {
        // Get valid devices
        QStringList listDeviceKey;
        QStringList listDeviceValue;
        for(const HWDevice& device : devices)
        {
            if(device.isValid())
            {
                listDeviceKey << device.fingerprint;
                listDeviceValue << device.toString();
            }
        }

        // Select a device
        if(listDeviceKey.length() > 0)
        {
            HardwareKeystoreDialog keystoreDialog(listDeviceValue, parent);
            if(keystoreDialog.exec() == QDialog::Accepted &&
                    keystoreDialog.currentIndex() != -1)
            {
                fingerprint = listDeviceKey[keystoreDialog.currentIndex()];
                return true;
            }
        }
    }

    return false;
}
