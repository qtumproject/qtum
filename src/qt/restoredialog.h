#ifndef RESTOREDIALOG_H
#define RESTOREDIALOG_H

#include <QDialog>

namespace Ui {
class RestoreDialog;
}

class RestoreDialog : public QDialog
{
    Q_OBJECT

public:
    enum StartCommand{
        Reindex = 0,
        Salvage
    };

    explicit RestoreDialog(QWidget *parent = 0);
    ~RestoreDialog();

    int getStartCommand();

    QString getFileName();

private Q_SLOTS:
    void on_btnReset_clicked();

    void on_btnBoxRestore_accepted();

    void on_btnBoxRestore_rejected();

    void on_toolWalletPath_clicked();

private:
    Ui::RestoreDialog *ui;
};

#endif // RESTOREDIALOG_H
