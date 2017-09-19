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
    ui->scrollAreaResult->setStyleSheet(".QScrollArea { border: none;}");
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
    // Remove previous widget from scroll area
    QWidget *scrollWidget = ui->scrollAreaParams->widget();
    if(scrollWidget)
        scrollWidget->deleteLater();

    // Don't show empty list
    if(function.inputs.size() == 0)
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
    int i = 0;
    for(std::vector<ParameterABI>::const_iterator param = function.inputs.begin(); param != function.inputs.end(); ++param)
    {

        QHBoxLayout *hLayout = new QHBoxLayout();
        hLayout->setSpacing(10);
        hLayout->setContentsMargins(0,0,0,0);

        QLabel *paramName = new QLabel(this);
        QLineEdit *paramValue = new QLineEdit(this);
        paramValue->setReadOnly(true);
        paramName->setFixedWidth(160);

        QFontMetrics metrix(paramName->font());
        int width = paramName->width() + 10;
        QString text(QString("%2 <b>%1").arg(QString::fromStdString(param->name)).arg(QString::fromStdString(param->type)));
        QString clippedText = metrix.elidedText(text, Qt::ElideRight, width);

        paramName->setToolTip(QString("%2 %1").arg(QString::fromStdString(param->name)).arg(QString::fromStdString(param->type)));
        paramName->setText(clippedText);
        paramValue->setText(paramValues[i]);

        hLayout->addWidget(paramName);
        hLayout->addWidget(paramValue);

        vLayout->addLayout(hLayout);
        i++;
    }
    widgetParams->adjustSize();
    ui->scrollAreaParams->setMaximumHeight(widgetParams->sizeHint().height() + 2);
    ui->scrollAreaParams->setWidget(widgetParams);
    ui->scrollAreaParams->setVisible(true);
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
        // Remove previous widget from scroll area
        QWidget *scrollWidget = ui->scrollAreaResult->widget();
        if(scrollWidget)
            scrollWidget->deleteLater();

        if(values.size() > 0)
        {
            QWidget *widgetResults = new QWidget(this);
            QVBoxLayout *vLayout = new QVBoxLayout(widgetResults);
            vLayout->setSpacing(6);
            vLayout->setContentsMargins(0,6,0,6);
            widgetResults->setLayout(vLayout);

            for(size_t i = 0; i < values.size(); i++)
            {
                QHBoxLayout *hLayout = new QHBoxLayout();
                hLayout->setSpacing(10);
                hLayout->setContentsMargins(0,0,0,0);

                QLabel *resultName = new QLabel(this);
                QLineEdit *resultValue = new QLineEdit(this);
                resultValue->setReadOnly(true);
                resultName->setFixedWidth(160);

                QFontMetrics metrix(resultName->font());
                int width = resultName->width() + 10;
                QString text(QString("%2 <b>%1").arg(QString::fromStdString(function.outputs[i].name)).arg(QString::fromStdString(function.outputs[i].type)));
                QString clippedText = metrix.elidedText(text, Qt::ElideRight, width);

                resultName->setText(clippedText);
                resultName->setToolTip(QString("%2 %1").arg(QString::fromStdString(function.outputs[i].name)).arg(QString::fromStdString(function.outputs[i].type)));
                resultValue->setText(QString::fromStdString(values[i]));

                hLayout->addWidget(resultName);
                hLayout->addWidget(resultValue);

                vLayout->addLayout(hLayout);
            }
            widgetResults->adjustSize();
            ui->scrollAreaResult->setMaximumHeight(widgetResults->sizeHint().height() + 2);
            ui->scrollAreaResult->setWidget(widgetResults);
            ui->groupBoxResult->setVisible(true);
        }
        else
        {
            ui->groupBoxResult->setVisible(false);
        }
    }
    else
    {
        QString errorMessage;
        errorMessage = function.errorMessage(errors, false);
        QMessageBox::warning(this, tr("Create contract"), errorMessage);
    }
}
