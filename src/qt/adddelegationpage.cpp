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

    ui->labelStaker->setToolTip(tr("Super staker address."));
    ui->labelFee->setToolTip(tr("Super staker fee in percentage."));
    ui->labelAddress->setToolTip(tr("Delegate address to the staker."));

    GUIUtil::setupAddressWidget(ui->lineEditStaker, this);

    ui->lineEditAddress->setSenderAddress(true);

    // Set defaults
    ui->lineEditGasPrice->setValue(DEFAULT_GAS_PRICE);
    ui->lineEditGasPrice->setSingleStep(SINGLE_STEP);

    ui->lineEditGasLimit->setMinimum(MINIMUM_GAS_LIMIT);
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
    lstTranslations[PARAM_STAKER] = ui->labelStaker->text();
    lstTranslations[PARAM_FEE] = ui->spinBoxFee->text();
    lstTranslations[PARAM_ADDRESS] = ui->labelAddress->text();
    lstTranslations[PARAM_GASLIMIT] = ui->labelGasLimit->text();
    lstTranslations[PARAM_GASPRICE] = ui->labelGasPrice->text();

    m_execRPCCommand = new ExecRPCCommand(PRC_COMMAND, lstMandatory, lstOptional, lstTranslations, this);

    connect(ui->addDelegationButton, &QPushButton::clicked, this, &AddDelegationPage::on_addDelegationClicked);
    connect(ui->lineEditAddress, &QComboBox::currentTextChanged, this, &AddDelegationPage::on_updateAddDelegationButton);
    connect(ui->lineEditStaker, &QValidatedLineEdit::textChanged, this, &AddDelegationPage::on_updateAddDelegationButton);
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
    ui->lineEditStaker->setText("");
    ui->spinBoxFee->setValue(DEFAULT_STAKING_MIN_FEE);
    ui->lineEditAddress->setCurrentIndex(-1);
    ui->lineEditGasLimit->setValue(DEFAULT_GAS_LIMIT_OP_SEND);
    ui->lineEditGasPrice->setValue(DEFAULT_GAS_PRICE);
    ui->addDelegationButton->setEnabled(false);
}

bool AddDelegationPage::isValidStakerAddress()
{
    bool retval = true;
    if (!m_model->validateAddress(ui->lineEditStaker->text()))
    {
        ui->lineEditStaker->setValid(false);
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
    ui->labelGasLimit->setToolTip(tr("Gas limit. Default = %1, Max = %2").arg(DEFAULT_GAS_LIMIT_OP_CREATE).arg(blockGasLimit));
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
    ui->lineEditStaker->setFocus();
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

        WalletModel::UnlockContext ctx(m_model->requestUnlock());
        if(!ctx.isValid())
        {
            return;
        }

        // Initialize variables
        QMap<QString, QString> lstParams;
        QVariant result;
        QString errorMessage;
        QString resultJson;
        int unit = BitcoinUnits::BTC;
        uint64_t gasLimit = ui->lineEditGasLimit->value();
        CAmount gasPrice = ui->lineEditGasPrice->value();

        // Append params to the list
        QString delegateAddress = ui->lineEditAddress->currentText();
        QString stakerAddress = ui->lineEditStaker->text();
        int stakerFee = ui->spinBoxFee->value();
        ExecRPCCommand::appendParam(lstParams, PARAM_STAKER, stakerAddress);
        ExecRPCCommand::appendParam(lstParams, PARAM_FEE, QString::number(stakerFee));
        ExecRPCCommand::appendParam(lstParams, PARAM_ADDRESS, delegateAddress);
        ExecRPCCommand::appendParam(lstParams, PARAM_GASLIMIT, QString::number(gasLimit));
        ExecRPCCommand::appendParam(lstParams, PARAM_GASPRICE, BitcoinUnits::format(unit, gasPrice, false, BitcoinUnits::separatorNever));


        QString questionString = tr("Are you sure you want to delegate the address to the staker<br /><br />");
        questionString.append(tr("<b>%1</b>?")
                              .arg(ui->lineEditStaker->text()));

        SendConfirmationDialog confirmationDialog(tr("Confirm address delegation to staker."), questionString, "", "", SEND_CONFIRM_DELAY, this);
        confirmationDialog.exec();

        QMessageBox::StandardButton retval = (QMessageBox::StandardButton)confirmationDialog.result();
        if(retval == QMessageBox::Yes)
        {
            // Execute RPC command line
            if(!m_execRPCCommand->exec(m_model->node(), m_model, lstParams, result, resultJson, errorMessage))
            {
                QMessageBox::warning(this, tr("Set delegation to address"), errorMessage);
            }
            else
            {
                QVariantMap variantMap = result.toMap();
                std::string txid = variantMap.value("txid").toString().toStdString();
                interfaces::DelegationInfo delegation;
                delegation.delegate_address = delegateAddress.toStdString();
                delegation.staker_address = stakerAddress.toStdString();
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
    QString staker = ui->lineEditStaker->text();
    QString delegate = ui->lineEditAddress->currentText();
    if(staker.isEmpty() || delegate.isEmpty() || staker == delegate)
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
