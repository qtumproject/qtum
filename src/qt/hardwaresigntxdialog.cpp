#include <qt/hardwaresigntxdialog.h>
#include <qt/forms/ui_hardwaresigntxdialog.h>
#include <qt/walletmodel.h>

HardwareSignTxDialog::HardwareSignTxDialog(const QString &tx, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HardwareSignTxDialog),
    m_model(0)
{
    ui->setupUi(this);
    ui->textEditTxData->setText(tx);
}

HardwareSignTxDialog::~HardwareSignTxDialog()
{
    delete ui;
}

void HardwareSignTxDialog::on_cancelButton_clicked()
{
    QDialog::reject();
}

void HardwareSignTxDialog::setModel(WalletModel *model)
{
    m_model = model;
}
