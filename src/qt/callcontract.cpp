#include <qt/callcontract.h>
#include <qt/forms/ui_callcontract.h>
#include <qt/platformstyle.h>
#include <qt/walletmodel.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/rpcconsole.h>
#include <qt/execrpccommand.h>
#include <qt/abifunctionfield.h>
#include <qt/contractutil.h>
#include <qt/tabbarinfo.h>
#include <qt/contractresult.h>
#include <qt/contractbookpage.h>
#include <qt/editcontractinfodialog.h>
#include <qt/contracttablemodel.h>
#include <qt/styleSheet.h>
#include <qt/guiutil.h>
#include <QClipboard>

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
    m_model(0),
    m_clientModel(0),
    m_contractModel(0),
    m_execRPCCommand(0),
    m_ABIFunctionField(0),
    m_contractABI(0),
    m_tabInfo(0),
    m_results(1)
{
    m_platformStyle = platformStyle;
    // Setup ui components
    ui->setupUi(this);
    ui->saveInfoButton->setIcon(platformStyle->MultiStatesIcon(":/icons/filesave", PlatformStyle::PushButtonIcon));
    ui->loadInfoButton->setIcon(platformStyle->MultiStatesIcon(":/icons/address-book", PlatformStyle::PushButtonIcon));
    ui->pasteAddressButton->setIcon(platformStyle->MultiStatesIcon(":/icons/editpaste", PlatformStyle::PushButtonIcon));
    // Format tool buttons
    GUIUtil::formatToolButtons(ui->saveInfoButton, ui->loadInfoButton, ui->pasteAddressButton);

    // Set stylesheet
    SetObjectStyleSheet(ui->pushButtonClearAll, StyleSheetNames::ButtonDark);

    m_ABIFunctionField = new ABIFunctionField(platformStyle, ABIFunctionField::Call, ui->scrollAreaFunction);
    ui->scrollAreaFunction->setWidget(m_ABIFunctionField);

    ui->labelContractAddress->setToolTip(tr("The account address."));
    ui->labelSenderAddress->setToolTip(tr("The sender address hex string."));
    ui->pushButtonCallContract->setEnabled(false);
    ui->lineEditSenderAddress->setSenderAddress(true);
    ui->lineEditSenderAddress->setComboBoxEditable(true);

    m_tabInfo = new TabBarInfo(ui->stackedWidget);
    m_tabInfo->addTab(0, tr("Call Contract"));

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
    connect(ui->pushButtonClearAll, &QPushButton::clicked, this, &CallContract::on_clearAllClicked);
    connect(ui->pushButtonCallContract, &QPushButton::clicked, this, &CallContract::on_callContractClicked);
    connect(ui->lineEditContractAddress, &QValidatedLineEdit::textChanged, this, &CallContract::on_updateCallContractButton);
    connect(ui->textEditInterface, &QValidatedTextEdit::textChanged, this, &CallContract::on_newContractABI);
    connect(ui->stackedWidget, &QStackedWidget::currentChanged, this, &CallContract::on_updateCallContractButton);
    connect(ui->saveInfoButton, &QToolButton::clicked, this, &CallContract::on_saveInfoClicked);
    connect(ui->loadInfoButton, &QToolButton::clicked, this, &CallContract::on_loadInfoClicked);
    connect(ui->pasteAddressButton, &QToolButton::clicked, this, &CallContract::on_pasteAddressClicked);
    connect(ui->lineEditContractAddress, &QValidatedLineEdit::textChanged, this, &CallContract::on_contractAddressChanged);

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
}

void CallContract::setModel(WalletModel *_model)
{
    m_model = _model;
    m_contractModel = m_model->getContractTableModel();
    ui->lineEditSenderAddress->setWalletModel(m_model);
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

void CallContract::setContractAddress(const QString &address)
{
    ui->lineEditContractAddress->setText(address);
    ui->lineEditContractAddress->setFocus();
}

void CallContract::on_clearAllClicked()
{
    ui->lineEditContractAddress->clear();
    ui->lineEditSenderAddress->setCurrentIndex(-1);
    ui->textEditInterface->clear();
    m_tabInfo->clear();
}

void CallContract::on_callContractClicked()
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
        if(errorMessage.isEmpty() && m_execRPCCommand->exec(m_model->node(), m_model, lstParams, result, resultJson, errorMessage))
        {
            ContractResult *widgetResult = new ContractResult(ui->stackedWidget);
            widgetResult->setResultData(result, m_contractABI->functions[func], m_ABIFunctionField->getParamsValues(), ContractResult::CallResult);
            ui->stackedWidget->addWidget(widgetResult);
            int position = ui->stackedWidget->count() - 1;
            m_results = position == 1 ? 1 : m_results + 1;

            m_tabInfo->addTab(position, tr("Result %1").arg(m_results));
            m_tabInfo->setCurrent(position);
        }
        else
        {
            QMessageBox::warning(this, tr("Call contract"), errorMessage);
        }
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

void CallContract::on_saveInfoClicked()
{
    if(!m_contractModel)
        return;

    bool valid = true;

    if(!isValidContractAddress())
        valid = false;

    if(!isValidInterfaceABI())
        valid = false;

    if(!valid)
        return;

    QString contractAddress = ui->lineEditContractAddress->text();
    int row = m_contractModel->lookupAddress(contractAddress);
    EditContractInfoDialog::Mode dlgMode = row > -1 ? EditContractInfoDialog::EditContractInfo : EditContractInfoDialog::NewContractInfo;
    EditContractInfoDialog dlg(dlgMode, this);
    dlg.setModel(m_contractModel);
    if(dlgMode == EditContractInfoDialog::EditContractInfo)
    {
        dlg.loadRow(row);
    }
    dlg.setAddress(ui->lineEditContractAddress->text());
    dlg.setABI(ui->textEditInterface->toPlainText());
    if(dlg.exec())
    {
        ui->lineEditContractAddress->setText(dlg.getAddress());
        ui->textEditInterface->setText(dlg.getABI());
        on_contractAddressChanged();
    }
}

void CallContract::on_loadInfoClicked()
{
    ContractBookPage dlg(m_platformStyle, this);
    dlg.setModel(m_model->getContractTableModel());
    if(dlg.exec())
    {
        ui->lineEditContractAddress->setText(dlg.getAddressValue());
        on_contractAddressChanged();
    }
}

void CallContract::on_pasteAddressClicked()
{
    setContractAddress(QApplication::clipboard()->text());
}

void CallContract::on_contractAddressChanged()
{
    if(isValidContractAddress() && m_contractModel)
    {
        QString contractAddress = ui->lineEditContractAddress->text();
        if(m_contractModel->lookupAddress(contractAddress) > -1)
        {
            QString contractAbi = m_contractModel->abiForAddress(contractAddress);
            if(ui->textEditInterface->toPlainText() != contractAbi)
            {
                ui->textEditInterface->setText(m_contractModel->abiForAddress(contractAddress));
            }
        }
    }
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
        errorMessage = ContractUtil::errorMessage(function, errors, true);
    }
    return "";
}
