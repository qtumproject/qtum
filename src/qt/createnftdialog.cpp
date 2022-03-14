#include <qt/createnftdialog.h>
#include <qt/forms/ui_createnftdialog.h>
#include <qt/guiconstants.h>
#include <wallet/wallet.h>
#include <qt/clientmodel.h>
#include <qt/walletmodel.h>
#include <qt/nft.h>
#include <qt/qvalidatedlineedit.h>
#include <qt/contractutil.h>
#include <validation.h>
#include <qt/addresstablemodel.h>
#include <qt/optionsmodel.h>
#include <qt/styleSheet.h>

#include <QRegularExpressionValidator>
#include <QMessageBox>

CreateNftDialog::CreateNftDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateNftDialog),
    m_nftABI(0),
    m_model(0),
    m_clientModel(0)
{
    ui->setupUi(this);

    // Set stylesheet
    SetObjectStyleSheet(ui->clearButton, StyleSheetNames::ButtonDark);

    ui->labelDescription->setText(tr("(This is your wallet address which will be tied to the NFT for send/receive operations)"));
    QFont font = QApplication::font();
    font.setPointSizeF(font.pointSizeF() * 0.8);
    ui->labelDescription->setFont(font);
    ui->labelSpacer->setFont(font);

    m_nftABI = new Nft();

    connect(ui->lineEditNftUri, &QLineEdit::textChanged, this, &CreateNftDialog::on_addressChanged);
    connect(ui->lineEditNftName, &QLineEdit::textChanged, this, &CreateNftDialog::on_updateConfirmButton);
    connect(ui->lineEditSenderAddress, &QComboBox::currentTextChanged, this, &CreateNftDialog::on_updateConfirmButton);

    ui->lineEditSenderAddress->setAddressColumn(AddressTableModel::Address);
    ui->lineEditSenderAddress->setTypeRole(AddressTableModel::TypeRole);
    ui->lineEditSenderAddress->setSenderAddress(true);
    if(ui->lineEditSenderAddress->isEditable())
        ((QValidatedLineEdit*)ui->lineEditSenderAddress->lineEdit())->setEmptyIsValid(false);
    m_validNftAddress = false;
}

CreateNftDialog::~CreateNftDialog()
{
    delete ui;

    if(m_nftABI)
        delete m_nftABI;
    m_nftABI = 0;
}

void CreateNftDialog::setClientModel(ClientModel *clientModel)
{
    m_clientModel = clientModel;
}

void CreateNftDialog::clearAll()
{
    ui->lineEditNftUri->setText("");
    ui->lineEditNftName->setText("");
    ui->lineEditSenderAddress->setCurrentIndex(-1);
}

void CreateNftDialog::setModel(WalletModel *_model)
{
    m_model = _model;
    on_zeroBalanceAddressToken(m_model->getOptionsModel()->getZeroBalanceAddressToken());
    connect(m_model->getOptionsModel(), SIGNAL(zeroBalanceAddressTokenChanged(bool)), this, SLOT(on_zeroBalanceAddressToken(bool)));

    ui->lineEditSenderAddress->setWalletModel(m_model);
    m_nftABI->setModel(m_model);
}

void CreateNftDialog::on_clearButton_clicked()
{
    clearAll();
    QDialog::reject();
}

void CreateNftDialog::on_confirmButton_clicked()
{
    if(ui->lineEditSenderAddress->isValidAddress())
    {
        interfaces::NftInfo nftInfo;
        nftInfo.nft_name = ui->lineEditNftName->text().toStdString();
        nftInfo.sender_address = ui->lineEditSenderAddress->currentText().toStdString();

        if(m_model)
        {
            if(!m_model->wallet().isMineAddress(nftInfo.sender_address))
            {
                QString address = QString::fromStdString(nftInfo.sender_address);
                QString message = tr("The %1 address \"%2\" is not yours, please change it to new one.\n").arg("NFT", address);
                QMessageBox::warning(this, tr("Invalid NFT address"), message);
            }
            else if(m_model->wallet().existNftEntry(nftInfo))
            {
                QMessageBox::information(this, tr("NFT exist"), tr("The NFT already exist with the specified contract and sender addresses."));
            }
            else
            {
                m_model->wallet().addNftEntry(nftInfo);

                if(!fLogEvents)
                {
                    QMessageBox::information(this, tr("Log events"), tr("Enable log events from the option menu in order to receive NFT transactions."));
                }

                clearAll();
                QDialog::accept();
            }
        }
    }
}

void CreateNftDialog::on_addressChanged()
{
    if(m_nftABI)
    {
        std::string name;
        bool ret = m_nftABI->name(name);
        ui->lineEditNftName->setText(QString::fromStdString(name));
        m_validNftAddress = ret;
    }
    on_updateConfirmButton();
}

void CreateNftDialog::on_updateConfirmButton()
{
    bool enabled = true;
    if(ui->lineEditNftName->text().isEmpty())
    {
        enabled = false;
    }
    if(!ui->lineEditSenderAddress->isValidAddress())
    {
        enabled = false;
    }
    enabled &= m_validNftAddress;
    ui->confirmButton->setEnabled(enabled);
}

void CreateNftDialog::on_zeroBalanceAddressToken(bool enable)
{
    ui->lineEditSenderAddress->setIncludeZeroValue(enable);
}
