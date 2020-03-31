#include <qt/superstakerconfigdialog.h>
#include <qt/forms/ui_superstakerconfigdialog.h>

SuperStakerConfigDialog::SuperStakerConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SuperStakerConfigDialog)
{
    ui->setupUi(this);
}

SuperStakerConfigDialog::~SuperStakerConfigDialog()
{
    delete ui;
}
