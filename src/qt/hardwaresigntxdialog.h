#ifndef HARDWARESIGNTXDIALOG_H
#define HARDWARESIGNTXDIALOG_H

#include <QDialog>

namespace Ui {
class HardwareSignTxDialog;
}
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

private:
    Ui::HardwareSignTxDialog *ui;
    WalletModel* m_model;
};

#endif // HARDWARESIGNTXDIALOG_H
