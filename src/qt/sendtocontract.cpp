#include "sendtocontract.h"
#include "ui_sendtocontract.h"
#include "platformstyle.h"
#include "guiconstants.h"
#include "rpcconsole.h"
#include "execrpccommand.h"

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
static const QString PARAM_BROADCAST = "broadcast";
}
using namespace SendToContract_NS;

SendToContract::SendToContract(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SendToContract)
{
    ui->setupUi(this);
    ui->groupBoxOptional->setStyleSheet(STYLE_GROUPBOX);
    Q_UNUSED(platformStyle);

    // Create new PRC command line interface
    QStringList lstMandatory;
    lstMandatory.append(PARAM_ADDRESS);
    lstMandatory.append(PARAM_DATAHEX);
    QStringList lstOptional;
    lstOptional.append(PARAM_AMOUNT);
    lstOptional.append(PARAM_GASLIMIT);
    lstOptional.append(PARAM_GASPRICE);
    lstOptional.append(PARAM_SENDER);
    lstOptional.append(PARAM_BROADCAST);
    QMap<QString, QString> lstTranslations;
    lstTranslations[PARAM_ADDRESS] = ui->labelContractAddress->text();
    lstTranslations[PARAM_DATAHEX] = ui->labelDataHex->text();
    lstTranslations[PARAM_AMOUNT] = ui->labelAmount->text();
    lstTranslations[PARAM_GASLIMIT] = ui->labelGasLimit->text();
    lstTranslations[PARAM_GASPRICE] = ui->labelGasPrice->text();
    lstTranslations[PARAM_SENDER] = ui->labelSenderAddress->text();
    lstTranslations[PARAM_BROADCAST] = ui->labelBroadcast->text();
    m_execRPCCommand = new ExecRPCCommand(PRC_COMMAND, lstMandatory, lstOptional, lstTranslations, this);

    // Connect signals with slots
    connect(ui->pushButtonClearAll, SIGNAL(clicked()), SLOT(on_clearAll_clicked()));
    connect(ui->pushButtonSendToContract, SIGNAL(clicked()), SLOT(on_sendToContract_clicked()));
}

SendToContract::~SendToContract()
{
    delete ui;
}

void SendToContract::on_clearAll_clicked()
{
    ui->lineEditContractAddress->clear();
    ui->lineEditDataHex->clear();
    ui->lineEditAmount->clear();
    ui->lineEditGasLimit->clear();
    ui->lineEditGasPrice->clear();
    ui->lineEditSenderAddress->clear();
    ui->comboBoxBroadcast->setCurrentIndex(0);
}

void SendToContract::on_sendToContract_clicked()
{
    // Initialize variables
    QMap<QString, QString> lstParams;
    QVariant result;
    QString errorMessage;
    QString resultJson;

    // Append params to the list
    ExecRPCCommand::appendParam(lstParams, PARAM_ADDRESS, ui->lineEditContractAddress->text());
    ExecRPCCommand::appendParam(lstParams, PARAM_DATAHEX, ui->lineEditDataHex->text());
    ExecRPCCommand::appendParam(lstParams, PARAM_AMOUNT, ui->lineEditAmount->text());
    ExecRPCCommand::appendParam(lstParams, PARAM_GASLIMIT, ui->lineEditGasLimit->text());
    ExecRPCCommand::appendParam(lstParams, PARAM_GASPRICE, ui->lineEditGasPrice->text());
    ExecRPCCommand::appendParam(lstParams, PARAM_SENDER, ui->lineEditSenderAddress->text());
    QString broadcast;
    if(ui->comboBoxBroadcast->currentIndex() > 0)
    {
        broadcast = ui->comboBoxBroadcast->currentIndex() > 1 ? "false" : "true";
    }
    ExecRPCCommand::appendParam(lstParams, PARAM_BROADCAST, broadcast);

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
