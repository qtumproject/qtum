#ifndef HARDWAREDEVICEDIALOG_H
#define HARDWAREDEVICEDIALOG_H

#include <QDialog>

namespace Ui {
class HardwareDeviceDialog;
}

class HardwareDeviceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HardwareDeviceDialog(const QString& debugMessage, QWidget *parent = nullptr);
    ~HardwareDeviceDialog();

private Q_SLOTS:
    void on_cancelButton_clicked();
    void on_nextButton_clicked();

private:
    Ui::HardwareDeviceDialog *ui;
};

#endif // HARDWAREDEVICEDIALOG_H
