#include "createcontract.h"
#include "ui_createcontract.h"
#include "platformstyle.h"
#include "guiconstants.h"
#include "rpcconsole.h"
#include "execrpccommand.h"

namespace CreateContract_NS
{
// Contract data names
static const QString PRC_COMMAND = "createcontract";
static const QString PARAM_BYTECODE = "bytecode";
static const QString PARAM_GASLIMIT = "gaslimit";
static const QString PARAM_GASPRICE = "gasprice";
static const QString PARAM_SENDER = "sender";
}
using namespace CreateContract_NS;

CreateContract::CreateContract(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CreateContract),
    m_execRPCCommand(0)
{
    // Setup ui components
    Q_UNUSED(platformStyle);
    ui->setupUi(this);
    ui->groupBoxOptional->setStyleSheet(STYLE_GROUPBOX);
    setLinkLabels();
    ui->labelBytecode->setToolTip(tr("The bytecode of the contract"));
    ui->labelGasLimit->setToolTip(tr("Gas limit. Default = 1000000, Max = 40000000"));
    ui->labelGasPrice->setToolTip(tr("Gas price: QTUM price per gas unit. Default = 0.00000001, Min = 0.00000001"));
    ui->labelSenderAddress->setToolTip(tr("The quantum address that will be used to create the contract."));

    // Create new PRC command line interface
    QStringList lstMandatory;
    lstMandatory.append(PARAM_BYTECODE);
    QStringList lstOptional;
    lstOptional.append(PARAM_GASLIMIT);
    lstOptional.append(PARAM_GASPRICE);
    lstOptional.append(PARAM_SENDER);
    QMap<QString, QString> lstTranslations;
    lstTranslations[PARAM_BYTECODE] = ui->labelBytecode->text();
    lstTranslations[PARAM_GASLIMIT] = ui->labelGasLimit->text();
    lstTranslations[PARAM_GASPRICE] = ui->labelGasPrice->text();
    lstTranslations[PARAM_SENDER] = ui->labelSenderAddress->text();
    m_execRPCCommand = new ExecRPCCommand(PRC_COMMAND, lstMandatory, lstOptional, lstTranslations, this);

    // Connect signals with slots
    connect(ui->pushButtonClearAll, SIGNAL(clicked()), SLOT(on_clearAll_clicked()));
    connect(ui->pushButtonCreateContract, SIGNAL(clicked()), SLOT(on_createContract_clicked()));
}

CreateContract::~CreateContract()
{
    delete ui;
}

void CreateContract::setLinkLabels()
{
    ui->labelSolidity->setOpenExternalLinks(true);
    ui->labelSolidity->setText("<a href=\"https://chriseth.github.io/browser-solidity/#version=soljson-latest.js\">Solidity</a>");

    ui->labelContractTemplate->setOpenExternalLinks(true);
    ui->labelContractTemplate->setText("<a href=\"https://www.qtum.org\">Contract Template</a>");

    ui->labelGenerateBytecode->setOpenExternalLinks(true);
    ui->labelGenerateBytecode->setText("<a href=\"https://www.qtum.org\">Generate Bytecode</a>");
}

void CreateContract::on_clearAll_clicked()
{
    ui->textEditBytecode->clear();
    ui->lineEditGasLimit->clear();
    ui->lineEditGasPrice->clear();
    ui->lineEditSenderAddress->clear();
}

void CreateContract::on_createContract_clicked()
{
    // Initialize variables
    QMap<QString, QString> lstParams;
    QVariant result;
    QString errorMessage;
    QString resultJson;

    // Append params to the list
    ExecRPCCommand::appendParam(lstParams, PARAM_BYTECODE, ui->textEditBytecode->toPlainText());
    ExecRPCCommand::appendParam(lstParams, PARAM_GASLIMIT, ui->lineEditGasLimit->text());
    ExecRPCCommand::appendParam(lstParams, PARAM_GASPRICE, ui->lineEditGasPrice->text());
    ExecRPCCommand::appendParam(lstParams, PARAM_SENDER, ui->lineEditSenderAddress->text());

    // Execute RPC command line
    if(m_execRPCCommand->exec(lstParams, result, resultJson, errorMessage))
    {
        QString message = tr("The contract is created successfully.\n\n") + resultJson;
        QMessageBox::information(this, tr("Create contract"), message);
    }
    else
    {
        QMessageBox::warning(this, tr("Create contract"), errorMessage);
    }
}
