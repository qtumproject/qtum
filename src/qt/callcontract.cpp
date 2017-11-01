#include "callcontract.h"
#include "ui_callcontract.h"
#include "platformstyle.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "rpcconsole.h"
#include "execrpccommand.h"
#include "abifunctionfield.h"
#include "contractabi.h"
#include "tabbarinfo.h"
#include "contractresult.h"

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
    m_clientModel(0),
    m_execRPCCommand(0),
    m_ABIFunctionField(0),
    m_contractABI(0),
    m_tabInfo(0)
{
    // Setup ui components
    ui->setupUi(this);
    ui->groupBoxOptional->setStyleSheet(STYLE_GROUPBOX);
    ui->groupBoxFunction->setStyleSheet(STYLE_GROUPBOX);
    ui->scrollAreaFunction->setStyleSheet(".QScrollArea {border: none;}");
    m_ABIFunctionField = new ABIFunctionField(platformStyle, ABIFunctionField::Call, ui->scrollAreaFunction);
    ui->scrollAreaFunction->setWidget(m_ABIFunctionField);

    ui->labelContractAddress->setToolTip(tr("The account address."));
    ui->labelSenderAddress->setToolTip(tr("The sender address hex string."));
    ui->pushButtonCallContract->setEnabled(false);
    ui->lineEditSenderAddress->setComboBoxEditable(true);

    m_tabInfo = new TabBarInfo(ui->stackedWidget);
    m_tabInfo->addTab(0, tr("CallContract"));

    // Create new PRC command line interface
    QStringList lstMandatory;
    lstMandatory.append(PARAM_ADDRESS);
    lstMandatory.append(PARAM_DATAHEX);
    QStringList lstOptional;
    lstOptional.append(PARAM_SENDER);
    QMap<QString, QString> lstTranslations;
    lstTranslations[PARAM_ADDRESS] = ui->labelContractAddress->text();
    lstTranslations[PARAM_SENDER] = ui->labelSenderAddress->text();
    m_execRPCCommand = new ExecRPCCommand(PRC_COMMAND, lstMandatory, lstOptional, lstTranslations, this);
    m_contractABI = new ContractABI();

    // Connect signals with slots
    connect(ui->pushButtonClearAll, SIGNAL(clicked()), SLOT(on_clearAll_clicked()));
    connect(ui->pushButtonCallContract, SIGNAL(clicked()), SLOT(on_callContract_clicked()));
    connect(ui->lineEditContractAddress, SIGNAL(textChanged(QString)), SLOT(on_updateCallContractButton()));
    connect(ui->textEditInterface, SIGNAL(textChanged()), SLOT(on_newContractABI()));
    connect(ui->stackedWidget, SIGNAL(currentChanged(int)), SLOT(on_updateCallContractButton()));

    // Set contract address validator
    QRegularExpression regEx;
    regEx.setPattern(paternAddress);
    QRegularExpressionValidator *addressValidatr = new QRegularExpressionValidator(ui->lineEditContractAddress);
    addressValidatr->setRegularExpression(regEx);
    ui->lineEditContractAddress->setCheckValidator(addressValidatr);
}

CallContract::~CallContract()
{
    delete m_contractABI;
    delete ui;
}

void CallContract::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;

    if (m_clientModel)
    {
        connect(m_clientModel, SIGNAL(numBlocksChanged(int,QDateTime,double,bool)), this, SLOT(on_numBlocksChanged()));
        on_numBlocksChanged();
    }
}

bool CallContract::isValidContractAddress()
{
    ui->lineEditContractAddress->checkValidity();
    return ui->lineEditContractAddress->isValid();
}

bool CallContract::isValidInterfaceABI()
{
    ui->textEditInterface->checkValidity();
    return ui->textEditInterface->isValid();
}

bool CallContract::isDataValid()
{
    bool dataValid = true;

    if(!isValidContractAddress())
        dataValid = false;
    if(!isValidInterfaceABI())
        dataValid = false;
    if(!m_ABIFunctionField->isValid())
        dataValid = false;
    if(!ui->lineEditSenderAddress->isValidAddress())
        dataValid = false;

    return dataValid;
}

void CallContract::on_clearAll_clicked()
{
    ui->lineEditContractAddress->clear();
    ui->lineEditSenderAddress->setCurrentIndex(-1);
    ui->textEditInterface->clear();

    for(int i = ui->stackedWidget->count() - 1; i > 0; i--)
    {
        QWidget* widget = ui->stackedWidget->widget(i);
        ui->stackedWidget->removeWidget(widget);
        widget->deleteLater();
        m_tabInfo->removeTab(i);
    }
    m_tabInfo->setCurrent(0);
}

void CallContract::on_callContract_clicked()
{
    if(isDataValid())
    {
        // Initialize variables
        QMap<QString, QString> lstParams;
        QVariant result;
        QString errorMessage;
        QString resultJson;
        int func = m_ABIFunctionField->getSelectedFunction();

        // Append params to the list
        ExecRPCCommand::appendParam(lstParams, PARAM_ADDRESS, ui->lineEditContractAddress->text());
        ExecRPCCommand::appendParam(lstParams, PARAM_DATAHEX, toDataHex(func, errorMessage));
        ExecRPCCommand::appendParam(lstParams, PARAM_SENDER, ui->lineEditSenderAddress->currentText());

        // Execute RPC command line
        if(errorMessage.isEmpty() && m_execRPCCommand->exec(lstParams, result, resultJson, errorMessage))
        {
            ContractResult *widgetResult = new ContractResult(ui->stackedWidget);
            widgetResult->setResultData(result, m_contractABI->functions[func], m_ABIFunctionField->getParamsValues(), ContractResult::CallResult);
            ui->stackedWidget->addWidget(widgetResult);
            int position = ui->stackedWidget->count() - 1;

            m_tabInfo->addTab(position, tr("Result %1").arg(position));
            m_tabInfo->setCurrent(position);
        }
        else
        {
            QMessageBox::warning(this, tr("Call contract"), errorMessage);
        }
    }
}

void CallContract::on_numBlocksChanged()
{
    if(m_clientModel)
    {
        ui->lineEditSenderAddress->on_refresh();
    }
}

void CallContract::on_updateCallContractButton()
{
    int func = m_ABIFunctionField->getSelectedFunction();
    bool enabled = func != -1;
    if(ui->lineEditContractAddress->text().isEmpty())
    {
        enabled = false;
    }
    enabled &= ui->stackedWidget->currentIndex() == 0;

    ui->pushButtonCallContract->setEnabled(enabled);
}

void CallContract::on_newContractABI()
{
    std::string json_data = ui->textEditInterface->toPlainText().toStdString();
    if(!m_contractABI->loads(json_data))
    {
        m_contractABI->clean();
        ui->textEditInterface->setIsValidManually(false);
    }
    else
    {
        ui->textEditInterface->setIsValidManually(true);
    }
    m_ABIFunctionField->setContractABI(m_contractABI);

    on_updateCallContractButton();
}

QString CallContract::toDataHex(int func, QString& errorMessage)
{
    if(func == -1 || m_ABIFunctionField == NULL || m_contractABI == NULL)
    {
        return "";
    }

    std::string strData;
    std::vector<std::vector<std::string>> values = m_ABIFunctionField->getValuesVector();
    FunctionABI function = m_contractABI->functions[func];
    std::vector<ParameterABI::ErrorType> errors;

    if(function.abiIn(values, strData, errors))
    {
        return QString::fromStdString(strData);
    }
    else
    {
        errorMessage = function.errorMessage(errors, true);
    }
    return "";
}
