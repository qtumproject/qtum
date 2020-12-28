#ifndef HARDWARESIGNTXDIALOG_H
#define HARDWARESIGNTXDIALOG_H

#include <QDialog>

namespace Ui {
class HardwareSignTxDialog;
}
class HardwareSignTxDialogPriv;
class WalletModel;

class HardwareSignTxDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HardwareSignTxDialog(const QString &tx, QWidget *parent = nullptr);
    ~HardwareSignTxDialog();

    void setModel(WalletModel *model);

private Q_SLOTS:
    void on_cancelButton_clicked();
    void txChanged();

private:
    Ui::HardwareSignTxDialog *ui;
    HardwareSignTxDialogPriv* d;
};

#endif // HARDWARESIGNTXDIALOG_H
