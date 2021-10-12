#include "adddelegationpage.h"
#include "qt/forms/ui_adddelegationpage.h"

#include <qt/guiutil.h>
#include <validation.h>
#include <util/moneystr.h>
#include <wallet/wallet.h>
#include <qt/clientmodel.h>
#include <qt/optionsmodel.h>
#include <qt/bitcoinunits.h>
#include <qt/rpcconsole.h>
#include <qt/execrpccommand.h>
#include <qt/sendcoinsdialog.h>

namespace AddDelegation_NS
{
static const QString PRC_COMMAND = "setdelegateforaddress";
static const QString PARAM_STAKER = "staker";
static const QString PARAM_FEE = "fee";
static const QString PARAM_ADDRESS = "address";
static const QString PARAM_GASLIMIT = "gaslimit";
static const QString PARAM_GASPRICE = "gasprice";

static const CAmount SINGLE_STEP = 0.00000001*COIN;
}
using namespace AddDelegation_NS;

AddDelegationPage::AddDelegationPage(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddDelegationPage),
    m_model(nullptr),
    m_clientModel(nullptr),
    m_execRPCCommand(nullptr)
{
    ui->setupUi(this);

    setWindowTitle(tr("Add delegation"));

    ui->labelStakerName->setToolTip(tr("Super staker name."));
    ui->labelStakerAddress->setToolTip(tr("Super staker address."));
    ui->labelFee->setToolTip(tr("Super staker fee in percentage."));
    ui->labelAddress->setToolTip(tr("Delegate address to the staker."));

    GUIUtil::setupAddressWidget(ui->lineEditStakerAddress, this);

    ui->lineEditAddress->setSenderAddress(true);

    // Set defaults
    ui->lineEditGasPrice->setValue(DEFAULT_GAS_PRICE);
    ui->lineEditGasPrice->setSingleStep(SINGLE_STEP);

    ui->lineEditGasLimit->setMinimum(ADD_DELEGATION_MIN_GAS_LIMIT);
    ui->lineEditGasLimit->setMaximum(DEFAULT_GAS_LIMIT_OP_CREATE);
    ui->lineEditGasLimit->setValue(DEFAULT_GAS_LIMIT_OP_CREATE);

    ui->spinBoxFee->setMinimum(0);
    ui->spinBoxFee->setMaximum(100);
    ui->spinBoxFee->setValue(DEFAULT_STAKING_MIN_FEE);

    ui->addDelegationButton->setEnabled(false);

    // Create new PRC command line interface
    QStringList lstMandatory;
    lstMandatory.append(PARAM_STAKER);
    lstMandatory.append(PARAM_FEE);
    lstMandatory.append(PARAM_ADDRESS);

    QStringList lstOptional;
    lstOptional.append(PARAM_GASLIMIT);
    lstOptional.append(PARAM_GASPRICE);

    QMap<QString, QString> lstTranslations;
    lstTranslations[PARAM_STAKER] = ui->labelStakerAddress->text();
    lstTranslations[PARAM_FEE] = ui->spinBoxFee->text();
    lstTranslations[PARAM_ADDRESS] = ui->labelAddress->text();
    lstTranslations[PARAM_GASLIMIT] = ui->labelGasLimit->text();
    lstTranslations[PARAM_GASPRICE] = ui->labelGasPrice->text();

    m_execRPCCommand = new ExecRPCCommand(PRC_COMMAND, lstMandatory, lstOptional, lstTranslations, this);

    connect(ui->addDelegationButton, &QPushButton::clicked, this, &AddDelegationPage::on_addDelegationClicked);
    connect(ui->lineEditStakerName, &QLineEdit::textChanged, this, &AddDelegationPage::on_updateAddDelegationButton);
    connect(ui->lineEditAddress, &QComboBox::currentTextChanged, this, &AddDelegationPage::on_updateAddDelegationButton);
    connect(ui->lineEditStakerAddress, &QValidatedLineEdit::textChanged, this, &AddDelegationPage::on_updateAddDelegationButton);
}

AddDelegationPage::~AddDelegationPage()
{
    delete ui;
}

void AddDelegationPage::setModel(WalletModel *_model)
{
    m_model = _model;
    ui->lineEditAddress->setWalletModel(m_model);

    if (m_model && m_model->getOptionsModel())
        connect(m_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &AddDelegationPage::updateDisplayUnit);

    // update the display unit, to not use the default ("QTUM")
    updateDisplayUnit();
}

void AddDelegationPage::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;

    if (m_clientModel)
    {
        connect(m_clientModel, SIGNAL(gasInfoChanged(quint64, quint64, quint64)), this, SLOT(on_gasInfoChanged(quint64, quint64, quint64)));
    }
}

void AddDelegationPage::clearAll()
{
    ui->lineEditStakerName->setText("");
    ui->lineEditStakerAddress->setText("");
    ui->spinBoxFee->setValue(DEFAULT_STAKING_MIN_FEE);
    ui->lineEditAddress->setCurrentIndex(-1);
    ui->lineEditGasLimit->setValue(DEFAULT_GAS_LIMIT_OP_CREATE);
    ui->lineEditGasPrice->setValue(DEFAULT_GAS_PRICE);
    ui->addDelegationButton->setEnabled(false);
}

bool AddDelegationPage::isValidStakerAddress()
{
    bool retval = true;
    if (!m_model->validateAddress(ui->lineEditStakerAddress->text()))
    {
        ui->lineEditStakerAddress->setValid(false);
        retval = false;
    }
    return retval;
}

bool AddDelegationPage::isDataValid()
{
    bool dataValid = true;

    if(!isValidStakerAddress())
        dataValid = false;
    if(!ui->lineEditAddress->isValidAddress())
        dataValid = false;

    return dataValid;
}

void AddDelegationPage::on_gasInfoChanged(quint64 blockGasLimit, quint64 minGasPrice, quint64 nGasPrice)
{
    Q_UNUSED(nGasPrice)
    ui->labelGasLimit->setToolTip(tr("Gas limit. Default = %1, Min = %2, Max = %3").arg(DEFAULT_GAS_LIMIT_OP_CREATE).arg(ADD_DELEGATION_MIN_GAS_LIMIT).arg(blockGasLimit));
    ui->labelGasPrice->setToolTip(tr("Gas price: QTUM price per gas unit. Default = %1, Min = %2").arg(QString::fromStdString(FormatMoney(DEFAULT_GAS_PRICE))).arg(QString::fromStdString(FormatMoney(minGasPrice))));
    ui->lineEditGasPrice->SetMinValue(minGasPrice);
    ui->lineEditGasLimit->setMaximum(blockGasLimit);
}

void AddDelegationPage::accept()
{
    clearAll();
    QDialog::accept();
}

void AddDelegationPage::reject()
{
    clearAll();
    QDialog::reject();
}

void AddDelegationPage::show()
{
    ui->lineEditStakerName->setFocus();
    QDialog::show();
}

void AddDelegationPage::on_clearButton_clicked()
{
    reject();
}

void AddDelegationPage::on_addDelegationClicked()
{
    if(m_model)
    {
        if(!isDataValid())
            return;

        // Initialize variables
        QMap<QString, QString> lstParams;
        QVariant result;
        QString errorMessage;
        QString resultJson;
        int unit = BitcoinUnits::BTC;
        uint64_t gasLimit = ui->lineEditGasLimit->value();
        CAmount gasPrice = ui->lineEditGasPrice->value();
        QString delegateAddress = ui->lineEditAddress->currentText();
        QString stakerAddress = ui->lineEditStakerAddress->text();
        QString stakerName = ui->lineEditStakerName->text().trimmed();
        int stakerFee = ui->spinBoxFee->value();

        // Get delegation details
        std::string sDelegateAddress = delegateAddress.toStdString();
        std::string sStakerAddress = stakerAddress.toStdString();
        interfaces::DelegationDetails details = m_model->wallet().getDelegationDetails(sDelegateAddress);
        if(!details.c_contract_return)
            return;

        // Check if delegation exist in the wallet
        if(details.w_entry_exist)
        {
            QMessageBox::warning(this, tr("Set delegation for address"), tr("This address is already delegated."));
            return;
        }

        // Check if delegation exist in the contract
        if(details.c_entry_exist)
        {
            if(details.c_staker_address != sStakerAddress)
            {
                QMessageBox::warning(this, tr("Set delegation for address"), tr("The address is delegated to the staker:\n") + QString::fromStdString(details.c_staker_address));
                return;
            }
            else
            {
                // Add the delegation to the wallet
                QMessageBox::information(this, tr("Set delegation for address"), tr("Delegation already present. \nThe delegation for the address will be added in the wallet list."));
                interfaces::DelegationInfo info = details.toInfo(false);
                m_model->wallet().addDelegationEntry(info);
                accept();
                return;
            }
        }

        // Unlock wallet
        WalletModel::UnlockContext ctx(m_model->requestUnlock());
        if(!ctx.isValid())
        {
            return;
        }

        // Append params to the list
        ExecRPCCommand::appendParam(lstParams, PARAM_STAKER, stakerAddress);
        ExecRPCCommand::appendParam(lstParams, PARAM_FEE, QString::number(stakerFee));
        ExecRPCCommand::appendParam(lstParams, PARAM_ADDRESS, delegateAddress);
        ExecRPCCommand::appendParam(lstParams, PARAM_GASLIMIT, QString::number(gasLimit));
        ExecRPCCommand::appendParam(lstParams, PARAM_GASPRICE, BitcoinUnits::format(unit, gasPrice, false, BitcoinUnits::SeparatorStyle::NEVER));


        QString questionString = tr("Are you sure you want to delegate the address to the staker<br /><br />");
        questionString.append(tr("<b>%1</b>?")
                              .arg(ui->lineEditStakerAddress->text()));

        SendConfirmationDialog confirmationDialog(tr("Confirm address delegation to staker."), questionString, "", "", SEND_CONFIRM_DELAY, tr("Send"), this);
        confirmationDialog.exec();

        QMessageBox::StandardButton retval = (QMessageBox::StandardButton)confirmationDialog.result();
        if(retval == QMessageBox::Yes)
        {
            // Execute RPC command line
            if(!m_execRPCCommand->exec(m_model->node(), m_model, lstParams, result, resultJson, errorMessage))
            {
                QMessageBox::warning(this, tr("Set delegation for address"), errorMessage);
            }
            else
            {
                QVariantMap variantMap = result.toMap();
                std::string txid = variantMap.value("txid").toString().toStdString();
                interfaces::DelegationInfo delegation;
                delegation.delegate_address = delegateAddress.toStdString();
                delegation.staker_address = stakerAddress.toStdString();
                delegation.staker_name = stakerName.trimmed().toStdString();
                delegation.fee = stakerFee;
                delegation.create_tx_hash.SetHex(txid);
                m_model->wallet().addDelegationEntry(delegation);
            }

            accept();
        }
    }
}

void AddDelegationPage::on_updateAddDelegationButton()
{
    bool enabled = true;
    QString stakerName = ui->lineEditStakerName->text().trimmed();
    QString stakerAddress = ui->lineEditStakerAddress->text();
    QString delegate = ui->lineEditAddress->currentText();

    if(stakerAddress.isEmpty() || delegate.isEmpty() || stakerAddress == delegate || stakerName.isEmpty())
    {
        enabled = false;
    }
    else
    {
        enabled = isDataValid();
    }

    ui->addDelegationButton->setEnabled(enabled);
}

void AddDelegationPage::updateDisplayUnit()
{
    if(m_model && m_model->getOptionsModel())
    {
        // Update gasPriceAmount with the current unit
        ui->lineEditGasPrice->setDisplayUnit(m_model->getOptionsModel()->getDisplayUnit());
    }
}
