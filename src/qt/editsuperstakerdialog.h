#ifndef EDITSUPERSTAKERDIALOG_H
#define EDITSUPERSTAKERDIALOG_H

#include <QDialog>

namespace Ui {
class EditSuperStakerDialog;
}

class EditSuperStakerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditSuperStakerDialog(QWidget *parent = nullptr);
    ~EditSuperStakerDialog();
    void setData(const QString& superStakerName, const QString& superStakerAddress);
    QString getSuperStakerName();

public Q_SLOTS:
    void accept();
    void reject();

private Q_SLOTS:
    void on_cancelButton_clicked();
    void on_editSuperStakerButton_clicked();
    void on_updateEditStakerButton();

private:
    Ui::EditSuperStakerDialog *ui;
};

#endif // EDITSUPERSTAKERDIALOG_H
