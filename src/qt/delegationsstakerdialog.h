#ifndef DELEGATIONSSTAKERDIALOG_H
#define DELEGATIONSSTAKERDIALOG_H

#include <QDialog>

namespace Ui {
class DelegationsStakerDialog;
}

class StakerDelegationView;
class WalletModel;
class DelegationsStakerDialogPriv;

class DelegationsStakerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DelegationsStakerDialog(QWidget *parent = nullptr);
    ~DelegationsStakerDialog();

    void setModel(WalletModel *model);
    void setSuperStakerData(const QString& name, const QString& address, const int& fee, const QString& hash);

private:
    void updateData();

private:
    Ui::DelegationsStakerDialog *ui;
    WalletModel *model;
    StakerDelegationView *m_stakerDelegationView;
    DelegationsStakerDialogPriv *d;
};

#endif // DELEGATIONSSTAKERDIALOG_H
