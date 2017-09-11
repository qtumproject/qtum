#include "contractresult.h"
#include "ui_contractresult.h"
#include "guiconstants.h"
#include "contractabi.h"
#include <QMessageBox>

ContractResult::ContractResult(QWidget *parent) :
    QStackedWidget(parent),
    ui(new Ui::ContractResult)
{
    ui->setupUi(this);
    ui->groupBoxCallContract->setStyleSheet(STYLE_GROUPBOX);
    ui->groupBoxResult->setStyleSheet(STYLE_GROUPBOX);
    ui->groupBoxCreateOrSendTo->setStyleSheet(STYLE_GROUPBOX);

    ui->scrollAreaParams->setStyleSheet(".QScrollArea { border: none;}");
}

ContractResult::~ContractResult()
{
    delete ui;
}

void ContractResult::setResultData(QVariant result, FunctionABI function, QStringList paramValues, ContractTxType type)
{
    switch (type) {
    case CreateResult:
        updateCreateResult(result);
        setCurrentWidget(ui->pageCreateOrSendToResult);
        break;

    case SendToResult:
        updateSendToResult(result);
        setCurrentWidget(ui->pageCreateOrSendToResult);
        break;

    case CallResult:
        updateCallResult(result, function, paramValues);
        setCurrentWidget(ui->pageCallResult);
        break;

    default:
        break;
    }
}

void ContractResult::setParamsData(FunctionABI function, QStringList paramValues)
{
    QStringList paramNames;
    for(std::vector<ParameterABI>::const_iterator param = function.inputs.begin(); param != function.inputs.end(); ++param)
    {
        paramNames.append(QString::fromStdString(param->name));
    }

    // Remove previous widget from scroll area
    QWidget *scrollWidget = ui->scrollAreaParams->widget();
        if(scrollWidget)
            scrollWidget->deleteLater();

    // Don't show empty list
    if(paramNames.isEmpty())
    {
        ui->scrollAreaParams->setVisible(false);
        return;
    }

    QWidget *widgetParams = new QWidget(this);
    QVBoxLayout *vLayout = new QVBoxLayout(widgetParams);
    vLayout->setSpacing(6);
    vLayout->setContentsMargins(0,0,0,0);
    widgetParams->setLayout(vLayout);

    // Add rows with params and values sent
    for(int i = 0; i < paramNames.count(); i++)
    {
        QHBoxLayout *hLayout = new QHBoxLayout();
        hLayout->setSpacing(30);
        hLayout->setContentsMargins(0,0,0,0);

        QLabel *paramName = new QLabel(this);
        QLineEdit *paramValue = new QLineEdit(this);
        paramValue->setReadOnly(true);
        paramName->setMinimumWidth(110);
        paramName->setText(paramNames[i]);
        paramValue->setText(paramValues[i]);

        hLayout->addWidget(paramName);
        hLayout->addWidget(paramValue);

        vLayout->addLayout(hLayout);
    }
    ui->scrollAreaParams->setWidget(widgetParams);
}

void ContractResult::updateCreateResult(QVariant result)
{
    ui->lineEditContractAddress->setVisible(true);
    ui->labelContractAddress->setVisible(true);

    QVariantMap variantMap = result.toMap();

    ui->lineEditTxID->setText(variantMap.value("txid").toString());
    ui->lineEditSenderAddress->setText(variantMap.value("sender").toString());
    ui->lineEditHash160->setText(variantMap.value("hash160").toString());
    ui->lineEditContractAddress->setText(variantMap.value("address").toString());
}

void ContractResult::updateSendToResult(QVariant result)
{
    ui->lineEditContractAddress->setVisible(false);
    ui->labelContractAddress->setVisible(false);

    QVariantMap variantMap = result.toMap();

    ui->lineEditTxID->setText(variantMap.value("txid").toString());
    ui->lineEditSenderAddress->setText(variantMap.value("sender").toString());
    ui->lineEditHash160->setText(variantMap.value("hash160").toString());
}

void ContractResult::updateCallResult(QVariant result, FunctionABI function, QStringList paramValues)
{
    QVariantMap variantMap = result.toMap();
    QVariantMap executionResultMap = variantMap.value("executionResult").toMap();

    ui->lineEditCallContractAddress->setText(variantMap.value("address").toString());
    ui->lineEditFunction->setText(QString::fromStdString(function.name));

    setParamsData(function, paramValues);

    ui->lineEditCallSenderAddress->setText(variantMap.value("sender").toString());
    std::string rawData = executionResultMap.value("output").toString().toStdString();
    std::vector<std::string> values;
    std::vector<ParameterABI::ErrorType> errors;
    if(function.abiOut(rawData, values, errors))
    {
        if(values.size() > 0)
        {
            ui->lineEditResult->setText(QString::fromStdString(values[0]));
        }
    }
    else
    {
        QString errorMessage;
        errorMessage = function.errorMessage(errors, false);
        QMessageBox::warning(this, tr("Create contract"), errorMessage);
    }
}
