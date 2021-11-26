#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/hardwarekeystoredialog.h>
#include <qt/forms/ui_hardwarekeystoredialog.h>
#include <qt/qtumhwitool.h>

#include <QRadioButton>
#include <QVBoxLayout>
#include <QButtonGroup>
#include <QMessageBox>

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

bool HardwareKeystoreDialog::SelectDevice(QString &fingerprint,  QString& errorMessage, bool& canceled, bool stake, QWidget *parent)
{
    // Enumerate devices
    QtumHwiTool hwiTool(parent);
    QList<HWDevice> devices;
    canceled = false;
    if(hwiTool.enumerate(devices, stake))
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
            if(fingerprint != "" && listDeviceKey.contains(fingerprint))
            {
                return true;
            }

            HardwareKeystoreDialog keystoreDialog(listDeviceValue, parent);
            int result = keystoreDialog.exec();
            if(result == QDialog::Accepted &&
                    keystoreDialog.currentIndex() != -1)
            {
                fingerprint = listDeviceKey[keystoreDialog.currentIndex()];
                return true;
            }
            if(result == QDialog::Rejected)
            {
                canceled = true;
            }
        }
    }

    errorMessage = hwiTool.errorMessage();

    return false;
}

bool HardwareKeystoreDialog::AskDevice(QString &fingerprint, const QString &title, const QString &message, bool stake, QWidget *parent)
{
    QString errorMessage;
    bool canceled = false;
    if(SelectDevice(fingerprint, errorMessage, canceled, stake, parent))
        return true;

    QMessageBox box(QMessageBox::Question, title, message, QMessageBox::No | QMessageBox::Yes, parent);

    while(!canceled && box.exec() == QMessageBox::Yes)
    {
        if(SelectDevice(fingerprint, errorMessage, canceled, stake, parent))
            return true;
    }
    return false;
}

void HardwareKeystoreDialog::on_cancelButton_clicked()
{
    QDialog::reject();
}

void HardwareKeystoreDialog::on_okButton_clicked()
{
    QDialog::accept();
}
