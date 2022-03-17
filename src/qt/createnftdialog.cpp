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
#include <qt/addresstablemodel.h>
#include <qt/optionsmodel.h>
#include <qt/sendcoinsdialog.h>
#include <qt/styleSheet.h>
#include <qt/hardwaresigntx.h>
#include <qt/guiutil.h>
#include <util/moneystr.h>
#include <node/ui_interface.h>

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
    if(!ui->lineEditSenderAddress->isValidAddress())
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
        std::string owner = ui->lineEditSenderAddress->currentText().toStdString();
        std::string name = ui->lineEditNftName->text().toStdString();
        std::string url = ui->lineEditNftUri->text().toStdString();
        std::string desc = ui->lineEditNftDesc->text().toStdString();
        int32_t count = ui->spinBoxNftAmount->value();
        QString countFormated = QString::number(count);

        m_nftABI->setSender(owner);
        m_nftABI->setGasLimit(QString::number(gasLimit).toStdString());
        m_nftABI->setGasPrice(BitcoinUnits::format(unit, gasPrice, false, BitcoinUnits::SeparatorStyle::NEVER).toStdString());

        QString questionString;
        if (bCreateUnsigned) {
            questionString.append(tr("Do you want to draft this create NFT transaction?"));
            questionString.append("<br /><span style='font-size:10pt;'>");
            questionString.append(tr("Please, review your transaction proposal. This will produce a Partially Signed Qtum Transaction (PSBT) which you can copy and then sign with e.g. an offline %1 wallet, or a PSBT-compatible hardware wallet.").arg(PACKAGE_NAME));
            questionString.append("</span><br /><br />");
        } else {
            questionString.append(tr("Are you sure you want to create NFT? <br /><br />"));
        }
        questionString.append(tr("<b>%1 %2 </b> to ")
                              .arg(countFormated).arg(QString::fromStdString(name)));
        questionString.append(tr("<br />%3 <br />")
                              .arg(QString::fromStdString(owner)));

        const QString confirmation = bCreateUnsigned ? tr("Confirm create NFT proposal.") : tr("Confirm create NFT.");
        const QString confirmButtonText = bCreateUnsigned ? tr("Copy PSBT to clipboard") : tr("Send");
        SendConfirmationDialog confirmationDialog(confirmation, questionString, "", "", SEND_CONFIRM_DELAY, confirmButtonText, this);
        confirmationDialog.exec();
        QMessageBox::StandardButton retval = (QMessageBox::StandardButton)confirmationDialog.result();
        if(retval == QMessageBox::Yes)
        {
            if(m_nftABI->createNFT(name, url, desc, countFormated.toStdString(), true))
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
                        nftTx.receiver = owner;
                        nftTx.value = count;
                        nftTx.tx_hash = uint256S(m_nftABI->getTxId());
                        m_model->wallet().addNftTxEntry(nftTx);
                    }
                }
            }
            else
            {
                QMessageBox::warning(this, tr("Create NFT"), QString::fromStdString(m_nftABI->getErrorMessage()));
            }
            clearAll();
            QDialog::accept();
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
