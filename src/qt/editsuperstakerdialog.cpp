#include "editsuperstakerdialog.h"
#include "qt/forms/ui_editsuperstakerdialog.h"

EditSuperStakerDialog::EditSuperStakerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditSuperStakerDialog)
{
    ui->setupUi(this);

    setWindowTitle(tr("Edit super staker"));

    ui->editSuperStakerButton->setEnabled(false);

    connect(ui->lineEditStakerName, &QLineEdit::textEdited, this, &EditSuperStakerDialog::on_updateEditStakerButton);
}

EditSuperStakerDialog::~EditSuperStakerDialog()
{
    delete ui;
}

void EditSuperStakerDialog::setData(const QString &superStakerName, const QString &superStakerAddress)
{
    ui->lineEditStakerName->setText(superStakerName);
    ui->lineEditStakerAddress->setText(superStakerAddress);
}

QString EditSuperStakerDialog::getSuperStakerName()
{
    return ui->lineEditStakerName->text();
}

void EditSuperStakerDialog::accept()
{
    QDialog::accept();
}

void EditSuperStakerDialog::reject()
{
    QDialog::reject();
}

void EditSuperStakerDialog::on_cancelButton_clicked()
{
    reject();
}

void EditSuperStakerDialog::on_editSuperStakerButton_clicked()
{
    accept();
}

void EditSuperStakerDialog::on_updateEditStakerButton()
{
    QString stakerName = ui->lineEditStakerName->text().trimmed();
    bool enabled = true;
    if(stakerName.isEmpty())
    {
        enabled = false;
    }

    ui->editSuperStakerButton->setEnabled(enabled);
}
