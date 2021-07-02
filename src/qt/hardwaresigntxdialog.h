#ifndef HARDWARESIGNTXDIALOG_H
#define HARDWARESIGNTXDIALOG_H

#include <QDialog>

namespace Ui {
class HardwareSignTxDialog;
}
class HardwareSignTx;
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
    void on_signButton_clicked();
    void on_sendButton_clicked();
    void on_importButton_clicked();
    void txChanged();

private:
    bool askDevice();
    bool importAddressesData(bool& rescan, bool& importPKH, bool& importP2SH, bool& importBech32, QString& pathPKH, QString& pathP2SH, QString& pathBech32);

private:
    Ui::HardwareSignTxDialog *ui;
    HardwareSignTx* d;
};

#endif // HARDWARESIGNTXDIALOG_H
