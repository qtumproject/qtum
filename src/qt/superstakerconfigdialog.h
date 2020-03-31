#ifndef SUPERSTAKERCONFIGDIALOG_H
#define SUPERSTAKERCONFIGDIALOG_H

#include <QDialog>

namespace Ui {
class SuperStakerConfigDialog;
}

class WalletModel;
class ClientModel;

class SuperStakerConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SuperStakerConfigDialog(QWidget *parent = 0);
    ~SuperStakerConfigDialog();

    void setModel(WalletModel *_model);
    void setClientModel(ClientModel *clientModel);
    void setSuperStakerData(const QString& address, const QString& hash);

private:
    Ui::SuperStakerConfigDialog *ui;
    WalletModel* m_model;
    ClientModel* m_clientModel;
    QString address;
    QString hash;
};

#endif // SUPERSTAKERCONFIGDIALOG_H
