#include <qt/sendnftdialog.h>
#include <qt/forms/ui_sendnftdialog.h>

#include <qt/walletmodel.h>
#include <qt/clientmodel.h>
#include <qt/optionsmodel.h>
#include <validation.h>
#include <util/moneystr.h>
#include <util/convert.h>
#include <qt/nft.h>
#include <qt/bitcoinunits.h>
#include <wallet/wallet.h>
#include <validation.h>
#include <qt/guiutil.h>
#include <qt/sendcoinsdialog.h>
#include <qt/bitcoinaddressvalidator.h>
#include <uint256.h>
#include <qt/styleSheet.h>
#include <qt/hardwaresigntx.h>
#include <interfaces/node.h>
#include <node/ui_interface.h>

static const CAmount SINGLE_STEP = 0.00000001*COIN;

struct SelectedNft{
    std::string sender;
    std::string id;
    std::string balance;
};

SendNftDialog::SendNftDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendNftDialog),
    m_model(0),
    m_clientModel(0),
    m_nftABI(0),
    m_selectedNft(0)
{
    // Setup ui components
    ui->setupUi(this);

    // Set stylesheet
    SetObjectStyleSheet(ui->clearButton, StyleSheetNames::ButtonDark);

    ui->labelAddress->setToolTip(tr("The address that will receive the NFTs."));
    ui->labelAmount->setToolTip(tr("The amount in NFT to send."));
    m_nftABI = new Nft();
    m_selectedNft = new SelectedNft();

    // Set defaults
    ui->lineEditGasPrice->setValue(DEFAULT_GAS_PRICE);
    ui->lineEditGasPrice->setSingleStep(SINGLE_STEP);
    ui->lineEditGasLimit->setMaximum(DEFAULT_GAS_LIMIT_OP_SEND);
    ui->lineEditGasLimit->setValue(DEFAULT_GAS_LIMIT_OP_SEND);
    ui->confirmButton->setEnabled(false);

    // Connect signals with slots
    connect(ui->lineEditAddress, &QValidatedLineEdit::textChanged, this, &SendNftDialog::on_updateConfirmButton);
    connect(ui->lineEditAmount, &TokenAmountField::valueChanged,this, &SendNftDialog::on_updateConfirmButton);
    connect(ui->confirmButton, &QPushButton::clicked, this, &SendNftDialog::on_confirmClicked);

    ui->lineEditAddress->setCheckValidator(new BitcoinAddressCheckValidator(parent, true));
}

SendNftDialog::~SendNftDialog()
{
    delete ui;

    if(m_nftABI)
        delete m_nftABI;
    m_nftABI = 0;
}

void SendNftDialog::setModel(WalletModel *_model)
{
    m_model = _model;
    m_nftABI->setModel(m_model);

    if (m_model && m_model->getOptionsModel())
        connect(m_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &SendNftDialog::updateDisplayUnit);

    // update the display unit, to not use the default ("QTUM")
    updateDisplayUnit();

    bCreateUnsigned = m_model->createUnsigned();

    if (bCreateUnsigned) {
        ui->confirmButton->setText(tr("Cr&eate Unsigned"));
        ui->confirmButton->setToolTip(tr("Creates a Partially Signed Qtum Transaction (PSBT) for use with e.g. an offline %1 wallet, or a PSBT-compatible hardware wallet.").arg(PACKAGE_NAME));
    }
}

void SendNftDialog::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;

    if (m_clientModel)
    {
        connect(m_clientModel, SIGNAL(gasInfoChanged(quint64, quint64, quint64)), this, SLOT(on_gasInfoChanged(quint64, quint64, quint64)));
    }
}

void SendNftDialog::clearAll()
{
    ui->lineEditAddress->setText("");
    ui->lineEditAmount->clear();
    ui->lineEditGasLimit->setValue(DEFAULT_GAS_LIMIT_OP_SEND);
    ui->lineEditGasPrice->setValue(DEFAULT_GAS_PRICE);
}

bool SendNftDialog::isValidAddress()
{
    ui->lineEditAddress->checkValidity();
    return ui->lineEditAddress->isValid();
}

bool SendNftDialog::isDataValid()
{
    bool dataValid = true;

    if(!isValidAddress())
        dataValid = false;
    if(!ui->lineEditAmount->validate())
        dataValid = false;
    if(ui->lineEditAmount->value(0) <= 0)
    {
        ui->lineEditAmount->setValid(false);
        dataValid = false;
    }
    return dataValid;
}

void SendNftDialog::on_clearButton_clicked()
{
    clearAll();
    QDialog::reject();
}

void SendNftDialog::on_gasInfoChanged(quint64 blockGasLimit, quint64 minGasPrice, quint64 nGasPrice)
{
    Q_UNUSED(nGasPrice);
    ui->labelGasLimit->setToolTip(tr("Gas limit. Default = %1, Max = %2").arg(DEFAULT_GAS_LIMIT_OP_CREATE).arg(blockGasLimit));
    ui->labelGasPrice->setToolTip(tr("Gas price: QTUM price per gas unit. Default = %1, Min = %2").arg(QString::fromStdString(FormatMoney(DEFAULT_GAS_PRICE))).arg(QString::fromStdString(FormatMoney(minGasPrice))));
    ui->lineEditGasPrice->SetMinValue(minGasPrice);
    ui->lineEditGasLimit->setMaximum(blockGasLimit);
}

void SendNftDialog::on_updateConfirmButton()
{
    bool enabled = true;
    if(ui->lineEditAddress->text().isEmpty() || ui->lineEditAmount->text().isEmpty() || !isDataValid())
    {
        enabled = false;
    }

    ui->confirmButton->setEnabled(enabled);
}

void SendNftDialog::on_confirmClicked()
{
    if(!isDataValid())
        return;

    WalletModel::UnlockContext ctx(m_model->requestUnlock());
    if(!ctx.isValid())
    {
        return;
    }

    if(m_model)
    {
        int unit = BitcoinUnits::BTC;
        uint64_t gasLimit = ui->lineEditGasLimit->value();
        CAmount gasPrice = ui->lineEditGasPrice->value();

        m_nftABI->setSender(m_selectedNft->sender);
        m_nftABI->setGasLimit(QString::number(gasLimit).toStdString());
        m_nftABI->setGasPrice(BitcoinUnits::format(unit, gasPrice, false, BitcoinUnits::SeparatorStyle::NEVER).toStdString());

        std::string toAddress = ui->lineEditAddress->text().toStdString();
        std::string amountToSend = ui->lineEditAmount->text().toStdString();
        QString amountFormated = BitcoinUnits::formatInt256(ui->lineEditAmount->value(), false, BitcoinUnits::SeparatorStyle::ALWAYS);

        QString questionString;
        if (bCreateUnsigned) {
            questionString.append(tr("Do you want to draft this send NFT transaction?"));
            questionString.append("<br /><span style='font-size:10pt;'>");
            questionString.append(tr("Please, review your transaction proposal. This will produce a Partially Signed Qtum Transaction (PSBT) which you can copy and then sign with e.g. an offline %1 wallet, or a PSBT-compatible hardware wallet.").arg(PACKAGE_NAME));
            questionString.append("</span><br /><br />");
        } else {
            questionString.append(tr("Are you sure you want to send? <br /><br />"));
        }
        questionString.append(tr("<b>%1 %2 </b> to ")
                              .arg(amountFormated).arg(tr("NFT")));
        questionString.append(tr("<br />%3 <br />")
                              .arg(QString::fromStdString(toAddress)));

        const QString confirmation = bCreateUnsigned ? tr("Confirm send NFT proposal.") : tr("Confirm send NFT.");
        const QString confirmButtonText = bCreateUnsigned ? tr("Copy PSBT to clipboard") : tr("Send");
        SendConfirmationDialog confirmationDialog(confirmation, questionString, "", "", SEND_CONFIRM_DELAY, confirmButtonText, this);
        confirmationDialog.exec();
        QMessageBox::StandardButton retval = (QMessageBox::StandardButton)confirmationDialog.result();
        if(retval == QMessageBox::Yes)
        {
            if(m_nftABI->safeTransfer(toAddress, m_selectedNft->id, amountToSend, true))
            {
                if(bCreateUnsigned)
                {
                    QString psbt = QString::fromStdString(m_nftABI->getPsbt());
                    GUIUtil::setClipboard(psbt);
                    Q_EMIT message(tr("PSBT copied"), "Copied to clipboard", CClientUIInterface::MSG_INFORMATION);
                }
                else
                {
                    bool isSent = true;
                    if(m_model->getSignPsbtWithHwiTool())
                    {
                        QVariantMap variantMap;
                        QString psbt = QString::fromStdString(m_nftABI->getPsbt());
                        if(!HardwareSignTx::process(this, m_model, psbt, variantMap))
                            isSent = false;
                        else
                        {
                            std::string txid = variantMap["txid"].toString().toStdString();
                            m_nftABI->setTxId(txid);
                        }
                    }

                    if(isSent)
                    {
                        interfaces::NftTx nftTx;
                        nftTx.sender_address = m_selectedNft->sender;
                        nftTx.id = uint256S(m_selectedNft->id);
                        nftTx.receiver_address = toAddress;
                        dev::u256 nValue(amountToSend);
                        nftTx.value = u256Touint(nValue);
                        nftTx.tx_hash = uint256S(m_nftABI->getTxId());
                        m_model->wallet().addNftTxEntry(nftTx);
                    }
                }
            }
            else
            {
                QMessageBox::warning(this, tr("Send NFT"), QString::fromStdString(m_nftABI->getErrorMessage()));
            }
            clearAll();
            QDialog::accept();
        }
    }
}

void SendNftDialog::updateDisplayUnit()
{
    if(m_model && m_model->getOptionsModel())
    {
        // Update gasPriceAmount with the current unit
        ui->lineEditGasPrice->setDisplayUnit(m_model->getOptionsModel()->getDisplayUnit());
    }
}

void SendNftDialog::setNftData(std::string sender, std::string id, std::string balance)
{
    // Update data with the current nft
    m_selectedNft->sender = sender;
    m_selectedNft->sender = id;
    m_selectedNft->balance = balance;

    // Convert values for different number of decimals
    int256_t totalSupply(balance);
    int256_t value(ui->lineEditAmount->value());
    if(value > totalSupply)
    {
        value = totalSupply;
    }

    // Update the amount field with the current nft data
    ui->lineEditAmount->clear();
    ui->lineEditAmount->setDecimalUnits(0);
    ui->lineEditAmount->setTotalSupply(totalSupply);
    if(value != 0)
    {
        ui->lineEditAmount->setValue(value);
    }
    ui->labelNftBalance->setText(BitcoinUnits::formatInt256(totalSupply, false, BitcoinUnits::SeparatorStyle::ALWAYS));
    setWindowTitle(tr("Send"));
}
