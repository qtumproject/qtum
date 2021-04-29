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

    bool importAddressesData(bool& rescan, bool& importPKH, bool& importP2SH, bool& importBech32, QString& pathPKH, QString& pathP2SH, QString& pathBech32);

private Q_SLOTS:
    void on_cancelButton_clicked();
    void on_okButton_clicked();
    void updateWidgets();

private:
    void widgetEnabled(QWidget *widget, bool enable);
    bool isDataValid();
    bool isDataSelected(bool rescan, bool importPKH, bool importP2SH, bool importBech32);

private:
    Ui::DerivationPathDialog *ui;
};

#endif // DERIVATIONPATHDIALOG_H
