#ifndef DERIVATIONPATHDIALOG_H
#define DERIVATIONPATHDIALOG_H

#include <QDialog>

namespace Ui {
class DerivationPathDialog;
}

class DerivationPathDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DerivationPathDialog(QWidget *parent = nullptr);
    ~DerivationPathDialog();

private Q_SLOTS:
    void on_backButton_clicked();

private:
    Ui::DerivationPathDialog *ui;
};

#endif // DERIVATIONPATHDIALOG_H
