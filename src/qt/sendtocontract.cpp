#include "sendtocontract.h"
#include "ui_sendtocontract.h"
#include "platformstyle.h"
#include "walletmodel.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "rpcconsole.h"
#include "execrpccommand.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "validation.h"
#include "utilmoneystr.h"

namespace SendToContract_NS
{
// Contract data names
static const QString PRC_COMMAND = "sendtocontract";
static const QString PARAM_ADDRESS = "address";
static const QString PARAM_DATAHEX = "datahex";
static const QString PARAM_AMOUNT = "amount";
static const QString PARAM_GASLIMIT = "gaslimit";
static const QString PARAM_GASPRICE = "gasprice";
static const QString PARAM_SENDER = "sender";

static const CAmount SINGLE_STEP = 0.00000001*COIN;
static const CAmount HIGH_GASPRICE = 0.001*COIN;
}
using namespace SendToContract_NS;

SendToContract::SendToContract(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SendToContract),
    m_model(0),
    m_clientModel(0),
    m_execRPCCommand(0)
{
    // Setup ui components
    Q_UNUSED(platformStyle);
    ui->setupUi(this);
    ui->groupBoxOptional->setStyleSheet(STYLE_GROUPBOX);

    ui->labelContractAddress->setToolTip(tr("The contract address that will receive the funds and data."));
    ui->labelDataHex->setToolTip(tr("The data to send."));
    ui->labelAmount->setToolTip(tr("The amount in QTUM to send. Default = 0."));
    ui->labelSenderAddress->setToolTip(tr("The quantum address that will be used as sender."));
   
    // Set defaults
    ui->lineEditGasPrice->setValue(DEFAULT_GAS_PRICE);
    ui->lineEditGasPrice->setSingleStep(SINGLE_STEP);
    ui->lineEditGasLimit->setMinimum(MINIMUM_GAS_LIMIT);
    ui->lineEditGasLimit->setMaximum(DEFAULT_GAS_LIMIT_OP_SEND);
    ui->lineEditGasLimit->setValue(DEFAULT_GAS_LIMIT_OP_SEND);
    ui->pushButtonSendToContract->setEnabled(false);

    // Create new PRC command line interface
    QStringList lstMandatory;
    lstMandatory.append(PARAM_ADDRESS);
    lstMandatory.append(PARAM_DATAHEX);
    QStringList lstOptional;
    lstOptional.append(PARAM_AMOUNT);
    lstOptional.append(PARAM_GASLIMIT);
    lstOptional.append(PARAM_GASPRICE);
    lstOptional.append(PARAM_SENDER);
    QMap<QString, QString> lstTranslations;
    lstTranslations[PARAM_ADDRESS] = ui->labelContractAddress->text();
    lstTranslations[PARAM_DATAHEX] = ui->labelDataHex->text();
    lstTranslations[PARAM_AMOUNT] = ui->labelAmount->text();
    lstTranslations[PARAM_GASLIMIT] = ui->labelGasLimit->text();
    lstTranslations[PARAM_GASPRICE] = ui->labelGasPrice->text();
    lstTranslations[PARAM_SENDER] = ui->labelSenderAddress->text();
    m_execRPCCommand = new ExecRPCCommand(PRC_COMMAND, lstMandatory, lstOptional, lstTranslations, this);

    // Connect signals with slots
    connect(ui->pushButtonClearAll, SIGNAL(clicked()), SLOT(on_clearAll_clicked()));
    connect(ui->pushButtonSendToContract, SIGNAL(clicked()), SLOT(on_sendToContract_clicked()));
    connect(ui->lineEditContractAddress, SIGNAL(textChanged(QString)), SLOT(on_updateCallContractButton()));
    connect(ui->lineEditDataHex, SIGNAL(textChanged(QString)), SLOT(on_updateSendToContractButton()));
}

SendToContract::~SendToContract()
{
    delete ui;
}

void SendToContract::setModel(WalletModel *_model)
{
    m_model = _model;
}

void SendToContract::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;

    if (m_clientModel) 
    {
        connect(m_clientModel, SIGNAL(numBlocksChanged(int,QDateTime,double,bool)), this, SLOT(on_numBlocksChanged()));
        on_numBlocksChanged();
    }
}

void SendToContract::on_clearAll_clicked()
{
    ui->lineEditContractAddress->clear();
    ui->lineEditDataHex->clear();
    ui->lineEditAmount->clear();
    ui->lineEditGasLimit->setValue(DEFAULT_GAS_LIMIT_OP_SEND);
    ui->lineEditGasPrice->setValue(DEFAULT_GAS_PRICE);
    ui->lineEditSenderAddress->setCurrentIndex(-1);
}

void SendToContract::on_sendToContract_clicked()
{
    // Initialize variables
    QMap<QString, QString> lstParams;
    QVariant result;
    QString errorMessage;
    QString resultJson;
    int unit = m_model->getOptionsModel()->getDisplayUnit();
    uint64_t gasLimit = ui->lineEditGasLimit->value();
    CAmount gasPrice = ui->lineEditGasPrice->value();

    // Check the for high gas price
    if(gasPrice > HIGH_GASPRICE)
    {
        QString message = tr("The Gas Price is too high, are you sure you want to possibly spend a max of %1 for this transaction?");
        if(QMessageBox::question(this, tr("High Gas price"), message.arg(BitcoinUnits::formatWithUnit(unit, gasLimit * gasPrice))) == QMessageBox::No)
            return;
    }

    // Append params to the list
    ExecRPCCommand::appendParam(lstParams, PARAM_ADDRESS, ui->lineEditContractAddress->text());
    ExecRPCCommand::appendParam(lstParams, PARAM_DATAHEX, ui->lineEditDataHex->text());
    ExecRPCCommand::appendParam(lstParams, PARAM_AMOUNT, BitcoinUnits::format(unit, ui->lineEditAmount->value()));
    ExecRPCCommand::appendParam(lstParams, PARAM_GASLIMIT, QString::number(gasLimit));
    ExecRPCCommand::appendParam(lstParams, PARAM_GASPRICE, BitcoinUnits::format(unit, gasPrice));
    ExecRPCCommand::appendParam(lstParams, PARAM_SENDER, ui->lineEditSenderAddress->currentText());

    // Execute RPC command line
    if(m_execRPCCommand->exec(lstParams, result, resultJson, errorMessage))
    {
        QString message = tr("Send to the contract is performed successfully.\n\n") + resultJson;
        QMessageBox::information(this, tr("Send to contract"), message);
    }
    else
    {
        QMessageBox::warning(this, tr("Send to contract"), errorMessage);
    }
}

void SendToContract::on_numBlocksChanged()
{
    if(m_clientModel)
    {
        uint64_t blockGasLimit = 0;
        uint64_t minGasPrice = 0;
        uint64_t nGasPrice = 0;
        m_clientModel->getGasInfo(blockGasLimit, minGasPrice, nGasPrice);

        ui->labelGasLimit->setToolTip(tr("Gas limit: Default = %1, Max = %2.").arg(DEFAULT_GAS_LIMIT_OP_SEND).arg(blockGasLimit));
        ui->labelGasPrice->setToolTip(tr("Gas price: QTUM price per gas unit. Default = %1, Min = %2.").arg(QString::fromStdString(FormatMoney(DEFAULT_GAS_PRICE))).arg(QString::fromStdString(FormatMoney(minGasPrice))));
        ui->lineEditGasPrice->setMinimum(minGasPrice);
        ui->lineEditGasLimit->setMaximum(blockGasLimit);

        ui->lineEditSenderAddress->on_refresh();
    }
}

void SendToContract::on_updateSendToContractButton()
{
    if(ui->lineEditContractAddress->text().isEmpty() || ui->lineEditDataHex->text().isEmpty())
    {
        ui->pushButtonSendToContract->setEnabled(false);
    }
    else
    {
        ui->pushButtonSendToContract->setEnabled(true);
    }
}
