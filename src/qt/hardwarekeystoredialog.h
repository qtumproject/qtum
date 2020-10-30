#ifndef HARDWAREKEYSTOREDIALOG_H
#define HARDWAREKEYSTOREDIALOG_H

#include <QDialog>

namespace Ui {
class HardwareKeystoreDialog;
}

class HardwareKeystoreDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HardwareKeystoreDialog(QWidget *parent = 0);
    ~HardwareKeystoreDialog();

private:
    Ui::HardwareKeystoreDialog *ui;
};

#endif // HARDWAREKEYSTOREDIALOG_H
