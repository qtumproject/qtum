#include "editcontractinfodialog.h"
#include "ui_editcontractinfodialog.h"

#include "contracttablemodel.h"

#include <QDataWidgetMapper>
#include <QMessageBox>

EditContractInfoDialog::EditContractInfoDialog(Mode _mode, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditContractInfoDialog),
    mapper(0),
    mode(_mode),
    model(0)
{
    ui->setupUi(this);

    switch(mode)
    {
    case NewContractInfo:
        setWindowTitle(tr("New contract info"));
        break;
    case EditContractInfo:
        setWindowTitle(tr("Edit contract info"));
        break;
    }

    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
}

EditContractInfoDialog::~EditContractInfoDialog()
{
    delete ui;
}

void EditContractInfoDialog::setModel(ContractTableModel *_model)
{
    this->model = _model;
    if(!_model)
        return;

    mapper->setModel(_model);
    mapper->addMapping(ui->labelEdit, ContractTableModel::Label);
    mapper->addMapping(ui->addressEdit, ContractTableModel::Address);
    mapper->addMapping(ui->ABIEdit, ContractTableModel::ABI, "plainText");
}

void EditContractInfoDialog::loadRow(int row)
{
    mapper->setCurrentIndex(row);
}

bool EditContractInfoDialog::saveCurrentRow()
{
    if(!model)
        return false;

    switch(mode)
    {
    case NewContractInfo:
        address = model->addRow(
                ui->labelEdit->text(),
                ui->addressEdit->text(),
                ui->ABIEdit->toPlainText());
        if(!address.isEmpty())
            this->ABI = ui->ABIEdit->toPlainText();
        break;
    case EditContractInfo:
        if(mapper->submit())
        {
            address = ui->addressEdit->text();
        }
        break;
    }
    return !address.isEmpty();
}

void EditContractInfoDialog::accept()
{
    if(!model)
        return;

    if(!saveCurrentRow())
    {
        switch(model->getEditStatus())
        {
        case ContractTableModel::OK:
            // Failed with unknown reason. Just reject.
            break;
        case ContractTableModel::NO_CHANGES:
            // No changes were made during edit operation. Just reject.
            break;
        case ContractTableModel::DUPLICATE_ADDRESS:
            QMessageBox::warning(this, windowTitle(),
                                 tr("The entered address \"%1\" is already in the contract book.").arg(ui->addressEdit->text()),
                                 QMessageBox::Ok, QMessageBox::Ok);
            break;

        }
        return;
    }
    QDialog::accept();

}

QString EditContractInfoDialog::getAddress() const
{
    return address;
}

void EditContractInfoDialog::setAddress(const QString &_address)
{
    this->address = _address;
    ui->addressEdit->setText(_address);
}

QString EditContractInfoDialog::getABI() const
{
    return ABI;
}

void EditContractInfoDialog::setABI(const QString &_ABI)
{
    this->ABI = _ABI;
    ui->ABIEdit->setText(_ABI);
}
