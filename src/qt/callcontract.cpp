#include "callcontract.h"
#include "ui_callcontract.h"
#include "platformstyle.h"
#include "guiconstants.h"
#include "rpcconsole.h"
#include "execrpccommand.h"

namespace CallContract_NS
{
// Contract data names
static const QString PRC_COMMAND = "callcontract";
static const QString PARAM_ADDRESS = "address";
static const QString PARAM_DATAHEX = "datahex";
static const QString PARAM_SENDER = "sender";
}
using namespace CallContract_NS;

CallContract::CallContract(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CallContract),
    m_execRPCCommand(0)
{
    // Setup ui components
    Q_UNUSED(platformStyle);
    ui->setupUi(this);
    ui->groupBoxOptional->setStyleSheet(STYLE_GROUPBOX);
    ui->labelContractAddress->setToolTip(tr("The account address."));
    ui->labelDataHex->setToolTip(tr("The data hex string."));
    ui->labelSenderAddress->setToolTip(tr("The sender address hex string."));

    // Create new PRC command line interface
    QStringList lstMandatory;
    lstMandatory.append(PARAM_ADDRESS);
    lstMandatory.append(PARAM_DATAHEX);
    QStringList lstOptional;
    lstOptional.append(PARAM_SENDER);
    QMap<QString, QString> lstTranslations;
    lstTranslations[PARAM_ADDRESS] = ui->labelContractAddress->text();
    lstTranslations[PARAM_DATAHEX] = ui->labelDataHex->text();
    lstTranslations[PARAM_SENDER] = ui->labelSenderAddress->text();
    m_execRPCCommand = new ExecRPCCommand(PRC_COMMAND, lstMandatory, lstOptional, lstTranslations, this);

    // Connect signals with slots
    connect(ui->pushButtonClearAll, SIGNAL(clicked()), SLOT(on_clearAll_clicked()));
    connect(ui->pushButtonCallContract, SIGNAL(clicked()), SLOT(on_callContract_clicked()));
}

CallContract::~CallContract()
{
    delete ui;
}

void CallContract::on_clearAll_clicked()
{
    ui->lineEditContractAddress->clear();
    ui->lineEditDataHex->clear();
    ui->lineEditSenderAddress->clear();
}

void CallContract::on_callContract_clicked()
{
    // Initialize variables
    QMap<QString, QString> lstParams;
    QVariant result;
    QString errorMessage;
    QString resultJson;

    // Append params to the list
    ExecRPCCommand::appendParam(lstParams, PARAM_ADDRESS, ui->lineEditContractAddress->text());
    ExecRPCCommand::appendParam(lstParams, PARAM_DATAHEX, ui->lineEditDataHex->text());
    ExecRPCCommand::appendParam(lstParams, PARAM_SENDER, ui->lineEditSenderAddress->text());

    // Execute RPC command line
    if(m_execRPCCommand->exec(lstParams, result, resultJson, errorMessage))
    {
        QString message = tr("The contract is called successfully.\n\n") + resultJson;
        QMessageBox::information(this, tr("Call contract"), message);
    }
    else
    {
        QMessageBox::warning(this, tr("Call contract"), errorMessage);
    }
}
