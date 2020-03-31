#ifndef SUPERSTAKERCONFIGDIALOG_H
#define SUPERSTAKERCONFIGDIALOG_H

#include <QDialog>

namespace Ui {
class SuperStakerConfigDialog;
}

class SuperStakerConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SuperStakerConfigDialog(QWidget *parent = 0);
    ~SuperStakerConfigDialog();

private:
    Ui::SuperStakerConfigDialog *ui;
};

#endif // SUPERSTAKERCONFIGDIALOG_H
