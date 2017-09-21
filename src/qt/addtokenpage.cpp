#include "addtokenpage.h"
#include "ui_addtokenpage.h"

AddTokenPage::AddTokenPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AddTokenPage)
{
    ui->setupUi(this);
}

AddTokenPage::~AddTokenPage()
{
    delete ui;
}

void AddTokenPage::clearAll()
{
    ui->lineEditContractAddress->setText("");
    ui->lineEditTokenName->setText("");
    ui->lineEditTokenSymbol->setText("");
    ui->lineEditDecimals->setText("");
}

void AddTokenPage::on_clearButton_clicked()
{
    clearAll();
}

void AddTokenPage::on_confirmButton_clicked()
{
    QString address = ui->lineEditContractAddress->text();
    QString name = ui->lineEditTokenName->text();
    QString symbol = ui->lineEditTokenSymbol->text();
    int decimals = ui->lineEditDecimals->text().toInt();

    Q_EMIT on_addNewToken(address, name, symbol, decimals, 234234.234324);
}
