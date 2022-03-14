#include <qt/createnftdialog.h>
#include <qt/forms/ui_createnftdialog.h>
#include <qt/guiconstants.h>
#include <wallet/wallet.h>
#include <qt/clientmodel.h>
#include <qt/walletmodel.h>
#include <qt/nft.h>
#include <qt/bitcoinunits.h>
#include <qt/qvalidatedlineedit.h>
#include <qt/contractutil.h>
#include <validation.h>
#include <qt/addresstablemodel.h>
#include <qt/optionsmodel.h>
#include <qt/styleSheet.h>
#include <util/moneystr.h>

#include <QRegularExpressionValidator>
#include <QMessageBox>

namespace CreateNftDialog_NS
{
static const CAmount SINGLE_STEP = 0.00000001*COIN;
}
using namespace CreateNftDialog_NS;

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

    connect(ui->lineEditNftName, &QLineEdit::textChanged, this, &CreateNftDialog::on_updateConfirmButton);
    connect(ui->lineEditSenderAddress, &QComboBox::currentTextChanged, this, &CreateNftDialog::on_updateConfirmButton);

    ui->lineEditSenderAddress->setAddressColumn(AddressTableModel::Address);
    ui->lineEditSenderAddress->setTypeRole(AddressTableModel::TypeRole);
    ui->lineEditSenderAddress->setSenderAddress(true);
    if(ui->lineEditSenderAddress->isEditable())
        ((QValidatedLineEdit*)ui->lineEditSenderAddress->lineEdit())->setEmptyIsValid(false);

    // Set defaults
    ui->lineEditGasPrice->setValue(DEFAULT_GAS_PRICE);
    ui->lineEditGasPrice->setSingleStep(SINGLE_STEP);
    ui->lineEditGasLimit->setMaximum(DEFAULT_GAS_LIMIT_OP_CREATE);
    ui->lineEditGasLimit->setValue(DEFAULT_GAS_LIMIT_OP_CREATE);
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

    if (m_clientModel)
    {
        connect(m_clientModel, SIGNAL(gasInfoChanged(quint64, quint64, quint64)), this, SLOT(on_gasInfoChanged(quint64, quint64, quint64)));
    }
}

void CreateNftDialog::clearAll()
{
    ui->lineEditNftUri->setText("");
    ui->lineEditNftName->setText("");
    ui->lineEditSenderAddress->setCurrentIndex(-1);
    ui->lineEditGasLimit->setValue(DEFAULT_GAS_LIMIT_OP_CREATE);
    ui->lineEditGasPrice->setValue(DEFAULT_GAS_PRICE);
}

void CreateNftDialog::setModel(WalletModel *_model)
{
    m_model = _model;
    on_zeroBalanceAddressToken(m_model->getOptionsModel()->getZeroBalanceAddressToken());
    connect(m_model->getOptionsModel(), SIGNAL(zeroBalanceAddressTokenChanged(bool)), this, SLOT(on_zeroBalanceAddressToken(bool)));

    ui->lineEditSenderAddress->setWalletModel(m_model);
    m_nftABI->setModel(m_model);

    if (m_model && m_model->getOptionsModel())
        connect(m_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &CreateNftDialog::updateDisplayUnit);

    // update the display unit, to not use the default ("QTUM")
    updateDisplayUnit();

    bCreateUnsigned = m_model->createUnsigned();

    if (bCreateUnsigned) {
        ui->confirmButton->setText(tr("Cr&eate Unsigned"));
        ui->confirmButton->setToolTip(tr("Creates a Partially Signed Qtum Transaction (PSBT) for use with e.g. an offline %1 wallet, or a PSBT-compatible hardware wallet.").arg(PACKAGE_NAME));
    }
}

void CreateNftDialog::on_clearButton_clicked()
{
    clearAll();
    QDialog::reject();
}

void CreateNftDialog::on_gasInfoChanged(quint64 blockGasLimit, quint64 minGasPrice, quint64 nGasPrice)
{
    Q_UNUSED(nGasPrice);
    ui->labelGasLimit->setToolTip(tr("Gas limit. Default = %1, Max = %2").arg(DEFAULT_GAS_LIMIT_OP_CREATE).arg(blockGasLimit));
    ui->labelGasPrice->setToolTip(tr("Gas price: QTUM price per gas unit. Default = %1, Min = %2").arg(QString::fromStdString(FormatMoney(DEFAULT_GAS_PRICE))).arg(QString::fromStdString(FormatMoney(minGasPrice))));
    ui->lineEditGasPrice->SetMinValue(minGasPrice);
    ui->lineEditGasLimit->setMaximum(blockGasLimit);
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
            int unit = BitcoinUnits::BTC;
            uint64_t gasLimit = ui->lineEditGasLimit->value();
            CAmount gasPrice = ui->lineEditGasPrice->value();

            m_nftABI->setSender(nftInfo.sender_address);
            m_nftABI->setGasLimit(QString::number(gasLimit).toStdString());
            m_nftABI->setGasPrice(BitcoinUnits::format(unit, gasPrice, false, BitcoinUnits::SeparatorStyle::NEVER).toStdString());

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

void CreateNftDialog::updateDisplayUnit()
{
    if(m_model && m_model->getOptionsModel())
    {
        // Update gasPriceAmount with the current unit
        ui->lineEditGasPrice->setDisplayUnit(m_model->getOptionsModel()->getDisplayUnit());
    }
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
    ui->confirmButton->setEnabled(enabled);
}

void CreateNftDialog::on_zeroBalanceAddressToken(bool enable)
{
    ui->lineEditSenderAddress->setIncludeZeroValue(enable);
}
