#include <qt/addtokenpage.h>
#include <qt/forms/ui_addtokenpage.h>
#include <qt/guiconstants.h>
#include <wallet/wallet.h>
#include <qt/clientmodel.h>
#include <qt/walletmodel.h>
#include <qt/token.h>
#include <qt/qvalidatedlineedit.h>
#include <qt/contractutil.h>
#include <validation.h>
#include <qt/addresstablemodel.h>
#include <qt/optionsmodel.h>
#include <qt/styleSheet.h>

#include <QRegularExpressionValidator>
#include <QMessageBox>

AddTokenPage::AddTokenPage(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddTokenPage),
    m_tokenABI(0),
    m_model(0),
    m_clientModel(0)
{
    ui->setupUi(this);

    // Set stylesheet
    SetObjectStyleSheet(ui->clearButton, StyleSheetNames::ButtonDark);

    ui->labelDescription->setText(tr("(This is your wallet address which will be tied to the token for send/receive operations)"));
    QFont font = QApplication::font();
    font.setPointSizeF(font.pointSizeF() * 0.8);
    ui->labelDescription->setFont(font);
    ui->labelSpacer->setFont(font);

    m_tokenABI = new Token();

    connect(ui->lineEditContractAddress, &QLineEdit::textChanged, this, &AddTokenPage::on_addressChanged);
    connect(ui->lineEditTokenName, &QLineEdit::textChanged, this, &AddTokenPage::on_updateConfirmButton);
    connect(ui->lineEditTokenSymbol, &QLineEdit::textChanged, this, &AddTokenPage::on_updateConfirmButton);
    connect(ui->lineEditSenderAddress, &QComboBox::currentTextChanged, this, &AddTokenPage::on_updateConfirmButton);

    ui->lineEditSenderAddress->setAddressColumn(AddressTableModel::Address);
    ui->lineEditSenderAddress->setTypeRole(AddressTableModel::TypeRole);
    ui->lineEditSenderAddress->setSenderAddress(true);
    if(ui->lineEditSenderAddress->isEditable())
        ((QValidatedLineEdit*)ui->lineEditSenderAddress->lineEdit())->setEmptyIsValid(false);
    m_validTokenAddress = false;
}

AddTokenPage::~AddTokenPage()
{
    delete ui;

    if(m_tokenABI)
        delete m_tokenABI;
    m_tokenABI = 0;
}

void AddTokenPage::setClientModel(ClientModel *clientModel)
{
    m_clientModel = clientModel;
}

void AddTokenPage::clearAll()
{
    ui->lineEditContractAddress->setText("");
    ui->lineEditTokenName->setText("");
    ui->lineEditTokenSymbol->setText("");
    ui->lineEditDecimals->setText("");
    ui->lineEditSenderAddress->setCurrentIndex(-1);
}

void AddTokenPage::setModel(WalletModel *_model)
{
    m_model = _model;
    on_zeroBalanceAddressToken(m_model->getOptionsModel()->getZeroBalanceAddressToken());
    connect(m_model->getOptionsModel(), SIGNAL(zeroBalanceAddressTokenChanged(bool)), this, SLOT(on_zeroBalanceAddressToken(bool)));

    ui->lineEditSenderAddress->setWalletModel(m_model);
    m_tokenABI->setModel(m_model);
}

void AddTokenPage::on_clearButton_clicked()
{
    clearAll();
    QDialog::reject();
}

void AddTokenPage::on_confirmButton_clicked()
{
    if(ui->lineEditSenderAddress->isValidAddress())
    {
        interfaces::TokenInfo tokenInfo;
        tokenInfo.contract_address = ui->lineEditContractAddress->text().toStdString();
        tokenInfo.token_name = ui->lineEditTokenName->text().toStdString();
        tokenInfo.token_symbol = ui->lineEditTokenSymbol->text().toStdString();
        tokenInfo.decimals = ui->lineEditDecimals->text().toInt();
        tokenInfo.sender_address = ui->lineEditSenderAddress->currentText().toStdString();

        if(m_model)
        {
            if(!m_model->wallet().isMineAddress(tokenInfo.sender_address))
            {
                QString symbol = QString::fromStdString(tokenInfo.token_symbol);
                QString address = QString::fromStdString(tokenInfo.sender_address);
                QString message = tr("The %1 address \"%2\" is not yours, please change it to new one.\n").arg(symbol, address);
                QMessageBox::warning(this, tr("Invalid token address"), message);
            }
            else if(m_model->wallet().existTokenEntry(tokenInfo))
            {
                QMessageBox::information(this, tr("Token exist"), tr("The token already exist with the specified contract and sender addresses."));
            }
            else
            {
                m_model->wallet().addTokenEntry(tokenInfo);

                if(!fLogEvents)
                {
                    QMessageBox::information(this, tr("Log events"), tr("Enable log events from the option menu in order to receive token transactions."));
                }

                clearAll();
                QDialog::accept();
            }
        }
    }
}

void AddTokenPage::on_addressChanged()
{
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
        m_validTokenAddress = ret;
    }
    on_updateConfirmButton();
}

void AddTokenPage::on_updateConfirmButton()
{
    bool enabled = true;
    if(ui->lineEditTokenName->text().isEmpty())
    {
        enabled = false;
    }
    if(ui->lineEditTokenSymbol->text().isEmpty())
    {
        enabled = false;
    }
    if(!ui->lineEditSenderAddress->isValidAddress())
    {
        enabled = false;
    }
    enabled &= m_validTokenAddress;
    ui->confirmButton->setEnabled(enabled);
}

void AddTokenPage::on_zeroBalanceAddressToken(bool enable)
{
    ui->lineEditSenderAddress->setIncludeZeroValue(enable);
}
