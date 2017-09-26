#include "addtokenpage.h"
#include "ui_addtokenpage.h"
#include "guiconstants.h"
#include "token.h"

AddTokenPage::AddTokenPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AddTokenPage),
    m_tokenABI(0)
{
    ui->setupUi(this);
    ui->lineEditContractAddress->setStyleSheet(STYLE_UNDERLINE);
    ui->lineEditTokenName->setStyleSheet(STYLE_UNDERLINE);
    ui->lineEditTokenSymbol->setStyleSheet(STYLE_UNDERLINE);
    ui->lineEditDecimals->setStyleSheet(STYLE_UNDERLINE);
    m_tokenABI = new Token();

    connect(ui->lineEditContractAddress, SIGNAL(textChanged(const QString &)), this, SLOT(on_addressChanged()));
}

AddTokenPage::~AddTokenPage()
{
    delete ui;

    if(m_tokenABI)
        delete m_tokenABI;
    m_tokenABI = 0;
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

    clearAll();
}

void AddTokenPage::on_addressChanged()
{
    bool enableConfirm = false;
    QString tokenAddress = ui->lineEditContractAddress->text();
    if(m_tokenABI)
    {
        m_tokenABI->setAddress(tokenAddress.toStdString());
        std::string name, symbol, decimals;
        bool ret = m_tokenABI->name(name);
        ret &= m_tokenABI->symbol(symbol);
        ret &= m_tokenABI->decimals(decimals);
        ui->lineEditTokenName->setText(QString::fromStdString(name));
        ui->lineEditTokenSymbol->setText(QString::fromStdString(symbol));
        ui->lineEditDecimals->setText(QString::fromStdString(decimals));
        enableConfirm = ret;
    }
    ui->confirmButton->setEnabled(enableConfirm);
}
