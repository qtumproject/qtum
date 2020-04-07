#ifndef DELEGATIONSSTAKERDIALOG_H
#define DELEGATIONSSTAKERDIALOG_H

#include <QDialog>

namespace Ui {
class DelegationsStakerDialog;
}

class StakerDelegationView;
class WalletModel;

class DelegationsStakerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DelegationsStakerDialog(QWidget *parent = nullptr);
    ~DelegationsStakerDialog();

    void setModel(WalletModel *model);
    void setSuperStakerData(const QString& address, const QString& hash);

private:
    Ui::DelegationsStakerDialog *ui;
    WalletModel *model;
    StakerDelegationView *m_stakerDelegationView;
    QString address;
    QString hash;
};

#endif // DELEGATIONSSTAKERDIALOG_H
