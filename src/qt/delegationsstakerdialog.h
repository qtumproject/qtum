#ifndef DELEGATIONSSTAKERDIALOG_H
#define DELEGATIONSSTAKERDIALOG_H

#include <QDialog>

namespace Ui {
class DelegationsStakerDialog;
}

class WalletModel;

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

class DelegationsStakerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DelegationsStakerDialog(QWidget *parent = nullptr);
    ~DelegationsStakerDialog();

    void setModel(WalletModel *model);
    void setSuperStakerData(const QString& address, const QString& hash);

private Q_SLOTS:
    void contextualMenu(const QPoint &);
    void copyAddress();
    void copyFee();
    void copyPoD();

private:
    Ui::DelegationsStakerDialog *ui;
    WalletModel *model;
    QMenu *contextMenu;
    QString address;
    QString hash;
};

#endif // DELEGATIONSSTAKERDIALOG_H
