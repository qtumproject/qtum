#ifndef QTUMLEDGERINSTALLERDIALOG_H
#define QTUMLEDGERINSTALLERDIALOG_H

#include <QDialog>

namespace Ui {
class QtumLedgerInstallerDialog;
}

class QtumLedgerInstallerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QtumLedgerInstallerDialog(QWidget *parent = nullptr);
    ~QtumLedgerInstallerDialog();

    enum LedgerType
    {
        NanoS
    };

private Q_SLOTS:
    void on_addButton_clicked();
    void on_removeButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::QtumLedgerInstallerDialog *ui;
};

#endif // QTUMLEDGERINSTALLERDIALOG_H
