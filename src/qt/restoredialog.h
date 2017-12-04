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
    explicit RestoreDialog(QWidget *parent = 0);
    ~RestoreDialog();

private slots:
    void on_btnReset_clicked();

    void on_btnBoxRestore_accepted();

    void on_btnBoxRestore_rejected();

private:
    Ui::RestoreDialog *ui;
};

#endif // RESTOREDIALOG_H
