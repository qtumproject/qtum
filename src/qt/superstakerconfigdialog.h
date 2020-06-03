#ifndef SUPERSTAKERCONFIGDIALOG_H
#define SUPERSTAKERCONFIGDIALOG_H

#include <QDialog>

namespace Ui {
class SuperStakerConfigDialog;
}

class WalletModel;
class ClientModel;
class SuperStakerConfigDialogPriv;

class SuperStakerConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SuperStakerConfigDialog(QWidget *parent = 0);
    ~SuperStakerConfigDialog();

    // Addresses for filter
    enum AddressEnum
    {
        All,
        AllowList,
        ExcludeList
    };

    void setModel(WalletModel *_model);
    void setClientModel(ClientModel *clientModel);
    void setSuperStakerData(const QString& hash);
    void clearAll();

public Q_SLOTS:
    void chooseAddressType(int idx);
    void accept();
    void reject();

private Q_SLOTS:
    void on_buttonOk_clicked();
    void on_buttonCancel_clicked();
    void changeConfigEnabled();
    void updateDisplayUnit();
    void setAddressListVisible(bool visible);
    void on_enableOkButton();

private:
    void updateData();

private:
    Ui::SuperStakerConfigDialog *ui;
    WalletModel* m_model;
    ClientModel* m_clientModel;
    SuperStakerConfigDialogPriv* d;
};

#endif // SUPERSTAKERCONFIGDIALOG_H
