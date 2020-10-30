#ifndef HARDWAREKEYSTOREDIALOG_H
#define HARDWAREKEYSTOREDIALOG_H

#include <QDialog>
#include <QStringList>

namespace Ui {
class HardwareKeystoreDialog;
}

class HardwareKeystoreDialogPriv;

class HardwareKeystoreDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HardwareKeystoreDialog(const QStringList& devices, QWidget *parent = 0);
    ~HardwareKeystoreDialog();
    int currentIndex() const;
    void setCurrentIndex(int index);

private:
    Ui::HardwareKeystoreDialog *ui;
    HardwareKeystoreDialogPriv *d;
};

#endif // HARDWAREKEYSTOREDIALOG_H
