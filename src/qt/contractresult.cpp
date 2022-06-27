#include <qt/contractresult.h>
#include <qt/forms/ui_contractresult.h>
#include <qt/guiconstants.h>
#include <qt/contractutil.h>
#include <qt/styleSheet.h>
#include <qt/walletmodel.h>
#include <qt/clientmodel.h>
#include <qt/execrpccommand.h>
#include <validation.h>

#include <QMessageBox>

static const QString PRC_COMMAND = "gettransactionreceipt";
static const QString PARAM_TXID = "txid";

ContractResult::ContractResult(QWidget *parent) :
    QStackedWidget(parent),
    ui(new Ui::ContractResult),
    m_model(0),
    m_clientModel(0),
    m_execRPCCommand(0),
    m_showReceipt(false)
{
    ui->setupUi(this);
    ui->groupBoxReceipt->setVisible(fLogEvents);

    // Create new PRC command line interface
    QStringList lstMandatory;
    lstMandatory.append(PARAM_TXID);
    QStringList lstOptional;
    QMap<QString, QString> lstTranslations;
    lstTranslations[PARAM_TXID] = ui->labelTxID->text();
    m_execRPCCommand = new ExecRPCCommand(PRC_COMMAND, lstMandatory, lstOptional, lstTranslations, this);
}

ContractResult::~ContractResult()
{
    delete ui;
}

void ContractResult::setResultData(QVariant result, FunctionABI function, QList<QStringList> paramValues, ContractTxType type)
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

void ContractResult::setParamsData(FunctionABI function, QList<QStringList> paramValues)
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
    widgetParams->setObjectName("scrollAreaWidgetContents");

    QVBoxLayout *mainLayout = new QVBoxLayout(widgetParams);
    mainLayout->setSpacing(6);
    mainLayout->setContentsMargins(0,0,30,0);

    // Add rows with params and values sent
    int i = 0;
    for(std::vector<ParameterABI>::const_iterator param = function.inputs.begin(); param != function.inputs.end(); ++param)
    {
        QHBoxLayout *hLayout = new QHBoxLayout();
        hLayout->setSpacing(10);
        hLayout->setContentsMargins(0,0,0,0);
        QVBoxLayout *vNameLayout = new QVBoxLayout();
        vNameLayout->setSpacing(3);
        vNameLayout->setContentsMargins(0,0,0,0);
        QVBoxLayout *paramValuesLayout = new QVBoxLayout();
        paramValuesLayout->setSpacing(3);
        paramValuesLayout->setContentsMargins(0,0,0,0);

        QLabel *paramName = new QLabel(this);
        paramName->setFixedWidth(160);
        paramName->setFixedHeight(19);
        QFontMetrics metrix(paramName->font());
        int width = paramName->width() + 10;
        QString text(QString("%2 <b>%1").arg(QString::fromStdString(param->name)).arg(QString::fromStdString(param->type)));
        QString clippedText = metrix.elidedText(text, Qt::ElideRight, width);
        paramName->setText(clippedText);
        paramName->setToolTip(QString("%2 %1").arg(QString::fromStdString(param->name)).arg(QString::fromStdString(param->type)));

        vNameLayout->addWidget(paramName);
        hLayout->addLayout(vNameLayout);
        QStringList listValues = paramValues[i];
        if(listValues.size() > 0)
        {
            int spacerSize = 0;
            for(int j = 0; j < listValues.count(); j++)
            {
                QLineEdit *paramValue = new QLineEdit(this);
                paramValue->setReadOnly(true);
                paramValue->setText(listValues[j]);
                paramValuesLayout->addWidget(paramValue);
                if(j > 0)
                    spacerSize += 30; // Line edit height + spacing
            }
            if(spacerSize > 0)
                vNameLayout->addSpacerItem(new QSpacerItem(20, spacerSize, QSizePolicy::Fixed, QSizePolicy::Fixed));

        hLayout->addLayout(paramValuesLayout);
        }
        else
        {
            hLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Fixed));
        }

        mainLayout->addLayout(hLayout);
        i++;
    }
    widgetParams->setLayout(mainLayout);
    widgetParams->adjustSize();
    if(widgetParams->sizeHint().height() < 70)
        ui->scrollAreaParams->setMaximumHeight(widgetParams->sizeHint().height() + 2);
    else
        ui->scrollAreaParams->setMaximumHeight(140);
    ui->scrollAreaParams->setWidget(widgetParams);
    ui->scrollAreaParams->setVisible(true);
}

void ContractResult::updateCreateResult(QVariant result)
{
    updateCreateSendToResult(result, true);
}

void ContractResult::updateSendToResult(QVariant result)
{
    updateCreateSendToResult(result, false);
}

void ContractResult::updateCallResult(QVariant result, FunctionABI function, QList<QStringList> paramValues)
{
    QVariantMap variantMap = result.toMap();
    QVariantMap executionResultMap = variantMap.value("executionResult").toMap();

    ui->lineEditCallContractAddress->setText(variantMap.value("address").toString());
    ui->lineEditFunction->setText(QString::fromStdString(function.name) + "()");

    setParamsData(function, paramValues);

    ui->lineEditCallSenderAddress->setText(variantMap.value("sender").toString());
    std::string rawData = executionResultMap.value("output").toString().toStdString();
    std::vector<std::vector<std::string>> values;
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
            widgetResults->setObjectName("scrollAreaWidgetContents");

            QVBoxLayout *mainLayout = new QVBoxLayout(widgetResults);
            mainLayout->setSpacing(6);
            mainLayout->setContentsMargins(0,6,0,6);
            widgetResults->setLayout(mainLayout);

            for(size_t i = 0; i < values.size(); i++)
            {
                QHBoxLayout *hLayout = new QHBoxLayout();
                hLayout->setSpacing(10);
                hLayout->setContentsMargins(0,0,0,0);
                QVBoxLayout *vNameLayout = new QVBoxLayout();
                vNameLayout->setSpacing(3);
                vNameLayout->setContentsMargins(0,0,0,0);
                QVBoxLayout *paramValuesLayout = new QVBoxLayout();
                paramValuesLayout->setSpacing(3);
                paramValuesLayout->setContentsMargins(0,0,0,0);

                QLabel *resultName = new QLabel(this);
                resultName->setFixedWidth(160);
                resultName->setFixedHeight(19);
                QFontMetrics metrix(resultName->font());
                int width = resultName->width() + 10;
                QString text(QString("%2 <b>%1").arg(QString::fromStdString(function.outputs[i].name)).arg(QString::fromStdString(function.outputs[i].type)));
                QString clippedText = metrix.elidedText(text, Qt::ElideRight, width);
                resultName->setText(clippedText);
                resultName->setToolTip(QString("%2 %1").arg(QString::fromStdString(function.outputs[i].name)).arg(QString::fromStdString(function.outputs[i].type)));

                vNameLayout->addWidget(resultName);
                std::vector<std::string> listValues = values[i];
                hLayout->addLayout(vNameLayout);
                if(listValues.size() > 0)
                {
                    int spacerSize = 0;
                    for(size_t j = 0; j < listValues.size(); j++)
                    {
                        QLineEdit *resultValue = new QLineEdit(this);
                        resultValue->setReadOnly(true);
                        resultValue->setText(QString::fromStdString(listValues[j]));
                        paramValuesLayout->addWidget(resultValue);
                        if(j > 0)
                            spacerSize += 30; // Line edit height + spacing
                    }
                    if(spacerSize > 0)
                        vNameLayout->addSpacerItem(new QSpacerItem(20, spacerSize, QSizePolicy::Fixed, QSizePolicy::Fixed));
                    hLayout->addLayout(paramValuesLayout);
                }
                else
                {
                    hLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Fixed));
                }
                mainLayout->addLayout(hLayout);
            }
            widgetResults->adjustSize();
            if(widgetResults->sizeHint().height() < 70)
            {
                ui->scrollAreaResult->setMaximumHeight(widgetResults->sizeHint().height() + 2);
            }
            else
            {
                ui->scrollAreaResult->setMaximumHeight(140);
            }
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
        errorMessage = ContractUtil::errorMessage(function, errors, false);
        QMessageBox::warning(this, tr("Create contract"), errorMessage);
    }
}

void ContractResult::updateCreateSendToResult(const QVariant &result, bool create)
{
    ui->lineEditContractAddress->setVisible(create);
    ui->labelContractAddress->setVisible(create);

    QVariantMap variantMap = result.toMap();

    m_txid = variantMap.value("txid").toString();
    m_showReceipt = true;
    ui->lineEditTxID->setText(m_txid);
    ui->lineEditSenderAddress->setText(variantMap.value("sender").toString());
    ui->lineEditHash160->setText(variantMap.value("hash160").toString());
    if(create)
    {
        ui->lineEditContractAddress->setText(variantMap.value("address").toString());
    }
}

void ContractResult::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;
    if(fLogEvents)
    {
        connect(m_clientModel, &ClientModel::tipChanged, this, &ContractResult::tipChanged);
    }
}

void ContractResult::setModel(WalletModel *_model)
{
    m_model = _model;
}

void ContractResult::tipChanged()
{
    if(m_showReceipt && m_model)
    {
        uint256 txid;
        txid.SetHex(m_txid.toStdString());
        auto tx = m_model->wallet().getTx(txid);
        if (tx)
        {
            interfaces::WalletTx wtx = m_model->wallet().getWalletTx(txid);
            if(wtx.is_in_main_chain)
            {
                // Initialize variables
                QMap<QString, QString> lstParams;
                QVariant result;
                QString errorMessage;
                QString resultJson;

                // Append params to the list
                ExecRPCCommand::appendParam(lstParams, PARAM_TXID, m_txid);

                // Execute RPC command line
                if(m_execRPCCommand->exec(m_model->node(), m_model, lstParams, result, resultJson, errorMessage))
                {
                    ui->textEditReceipt->setText(resultJson);
                }
                else
                {
                    QMessageBox::warning(this, tr("Contract result"), errorMessage);
                }
                m_showReceipt = false;
            }
        }
        else
        {
            m_showReceipt = false;
        }
    }
}
