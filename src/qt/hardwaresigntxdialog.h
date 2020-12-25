#ifndef HARDWARESIGNTXDIALOG_H
#define HARDWARESIGNTXDIALOG_H

#include <QDialog>

namespace Ui {
class HardwareSignTxDialog;
}

class HardwareSignTxDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HardwareSignTxDialog(QWidget *parent = nullptr);
    ~HardwareSignTxDialog();

private Q_SLOTS:
    void on_cancelButton_clicked();

private:
    Ui::HardwareSignTxDialog *ui;
};

#endif // HARDWARESIGNTXDIALOG_H
