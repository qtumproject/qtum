#include <qt/sendtocontract.h>
#include <qt/forms/ui_sendtocontract.h>
#include <qt/platformstyle.h>
#include <qt/walletmodel.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/rpcconsole.h>
#include <qt/execrpccommand.h>
#include <qt/bitcoinunits.h>
#include <qt/optionsmodel.h>
#include <validation.h>
#include <util/moneystr.h>
#include <qt/abifunctionfield.h>
#include <qt/contractutil.h>
#include <qt/tabbarinfo.h>
#include <qt/contractresult.h>
#include <qt/contractbookpage.h>
#include <qt/editcontractinfodialog.h>
#include <qt/contracttablemodel.h>
#include <qt/styleSheet.h>
#include <qt/guiutil.h>
#include <qt/sendcoinsdialog.h>
#include <qt/hardwaresigntx.h>
#include <QClipboard>
#include <interfaces/node.h>
#include <node/ui_interface.h>

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
    m_contractModel(0),
    m_execRPCCommand(0),
    m_ABIFunctionField(0),
    m_contractABI(0),
    m_tabInfo(0),
    m_results(1)
{
    m_platformStyle = platformStyle;

    // Setup ui components
    Q_UNUSED(platformStyle);
    ui->setupUi(this);
    ui->saveInfoButton->setIcon(platformStyle->MultiStatesIcon(":/icons/filesave", PlatformStyle::PushButtonIcon));
    ui->loadInfoButton->setIcon(platformStyle->MultiStatesIcon(":/icons/address-book", PlatformStyle::PushButtonIcon));
    ui->pasteAddressButton->setIcon(platformStyle->MultiStatesIcon(":/icons/editpaste", PlatformStyle::PushButtonIcon));
    // Format tool buttons
    GUIUtil::formatToolButtons(ui->saveInfoButton, ui->loadInfoButton, ui->pasteAddressButton);

    // Set stylesheet
    SetObjectStyleSheet(ui->pushButtonClearAll, StyleSheetNames::ButtonDark);

    m_ABIFunctionField = new ABIFunctionField(platformStyle, ABIFunctionField::SendTo, ui->scrollAreaFunction);
    ui->scrollAreaFunction->setWidget(m_ABIFunctionField);
    ui->lineEditAmount->setEnabled(true);
    ui->labelContractAddress->setToolTip(tr("The contract address that will receive the funds and data."));
    ui->labelAmount->setToolTip(tr("The amount in QTUM to send. Default = 0."));
    ui->labelSenderAddress->setToolTip(tr("The qtum address that will be used as sender."));

    m_tabInfo = new TabBarInfo(ui->stackedWidget);
    m_tabInfo->addTab(0, tr("Send To Contract"));

    // Set defaults
    ui->lineEditGasPrice->setValue(DEFAULT_GAS_PRICE);
    ui->lineEditGasPrice->setSingleStep(SINGLE_STEP);
    ui->lineEditGasLimit->setMinimum(MINIMUM_GAS_LIMIT);
    ui->lineEditGasLimit->setMaximum(DEFAULT_GAS_LIMIT_OP_SEND);
    ui->lineEditGasLimit->setValue(DEFAULT_GAS_LIMIT_OP_SEND);
    ui->textEditInterface->setIsValidManually(true);
    ui->pushButtonSendToContract->setEnabled(false);
    ui->lineEditSenderAddress->setSenderAddress(true);

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
    lstTranslations[PARAM_AMOUNT] = ui->labelAmount->text();
    lstTranslations[PARAM_GASLIMIT] = ui->labelGasLimit->text();
    lstTranslations[PARAM_GASPRICE] = ui->labelGasPrice->text();
    lstTranslations[PARAM_SENDER] = ui->labelSenderAddress->text();
    m_execRPCCommand = new ExecRPCCommand(PRC_COMMAND, lstMandatory, lstOptional, lstTranslations, this);
    m_contractABI = new ContractABI();

    // Connect signals with slots
    connect(ui->pushButtonClearAll, &QPushButton::clicked, this, &SendToContract::on_clearAllClicked);
    connect(ui->pushButtonSendToContract, &QPushButton::clicked, this, &SendToContract::on_sendToContractClicked);
    connect(ui->lineEditContractAddress, &QValidatedLineEdit::textChanged, this, &SendToContract::on_updateSendToContractButton);
    connect(ui->textEditInterface, &QValidatedTextEdit::textChanged, this, &SendToContract::on_newContractABI);
    connect(ui->stackedWidget, &QStackedWidget::currentChanged, this, &SendToContract::on_updateSendToContractButton);
    connect(m_ABIFunctionField, &ABIFunctionField::functionChanged, this, &SendToContract::on_functionChanged);
    connect(ui->saveInfoButton, &QToolButton::clicked, this, &SendToContract::on_saveInfoClicked);
    connect(ui->loadInfoButton, &QToolButton::clicked, this, &SendToContract::on_loadInfoClicked);
    connect(ui->pasteAddressButton, &QToolButton::clicked, this, &SendToContract::on_pasteAddressClicked);
    connect(ui->lineEditContractAddress, &QValidatedLineEdit::textChanged, this, &SendToContract::on_contractAddressChanged);

    // Set contract address validator
    QRegularExpression regEx;
    regEx.setPattern(paternAddress);
    QRegularExpressionValidator *addressValidatr = new QRegularExpressionValidator(ui->lineEditContractAddress);
    addressValidatr->setRegularExpression(regEx);
    ui->lineEditContractAddress->setCheckValidator(addressValidatr);
}

SendToContract::~SendToContract()
{
    delete m_contractABI;
    delete ui;
}

void SendToContract::setModel(WalletModel *_model)
{
    m_model = _model;
    m_contractModel = m_model->getContractTableModel();
    ui->lineEditSenderAddress->setWalletModel(m_model);

    if (m_model && m_model->getOptionsModel())
        connect(m_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &SendToContract::updateDisplayUnit);

    // update the display unit, to not use the default ("QTUM")
    updateDisplayUnit();

    bCreateUnsigned = m_model->createUnsigned();

    if (bCreateUnsigned) {
        ui->pushButtonSendToContract->setText(tr("Cr&eate Unsigned"));
        ui->pushButtonSendToContract->setToolTip(tr("Creates a Partially Signed Qtum Transaction (PSBT) for use with e.g. an offline %1 wallet, or a PSBT-compatible hardware wallet.").arg(PACKAGE_NAME));
    }
}

bool SendToContract::isValidContractAddress()
{
    ui->lineEditContractAddress->checkValidity();
    return ui->lineEditContractAddress->isValid();
}

bool SendToContract::isValidInterfaceABI()
{
    ui->textEditInterface->checkValidity();
    return ui->textEditInterface->isValid();
}

bool SendToContract::isDataValid()
{
    bool dataValid = true;

    if(!isValidContractAddress())
        dataValid = false;
    if(!isValidInterfaceABI())
        dataValid = false;
    if(!m_ABIFunctionField->isValid())
        dataValid = false;
    return dataValid;
}

void SendToContract::setContractAddress(const QString &address)
{
    ui->lineEditContractAddress->setText(address);
    ui->lineEditContractAddress->setFocus();
}

void SendToContract::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;

    if (m_clientModel)
    {
        connect(m_clientModel, SIGNAL(gasInfoChanged(quint64, quint64, quint64)), this, SLOT(on_gasInfoChanged(quint64, quint64, quint64)));
    }
}

void SendToContract::on_clearAllClicked()
{
    ui->lineEditContractAddress->clear();
    ui->lineEditAmount->clear();
    ui->lineEditAmount->setEnabled(true);
    ui->lineEditGasLimit->setValue(DEFAULT_GAS_LIMIT_OP_SEND);
    ui->lineEditGasPrice->setValue(DEFAULT_GAS_PRICE);
    ui->lineEditSenderAddress->setCurrentIndex(-1);
    ui->textEditInterface->clear();
    ui->textEditInterface->setIsValidManually(true);
    m_tabInfo->clear();
}

void SendToContract::on_sendToContractClicked()
{
    if(isDataValid())
    {
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
        int func = m_ABIFunctionField->getSelectedFunction();

        // Check for high gas price
        if(gasPrice > HIGH_GASPRICE)
        {
            QString message = tr("The Gas Price is too high, are you sure you want to possibly spend a max of %1 for this transaction?");
            if(QMessageBox::question(this, tr("High Gas price"), message.arg(BitcoinUnits::formatWithUnit(unit, gasLimit * gasPrice))) == QMessageBox::No)
                return;
        }

        // Append params to the list
        ExecRPCCommand::appendParam(lstParams, PARAM_ADDRESS, ui->lineEditContractAddress->text());
        ExecRPCCommand::appendParam(lstParams, PARAM_DATAHEX, toDataHex(func, errorMessage));
        QString amount = isFunctionPayable() ? BitcoinUnits::format(unit, ui->lineEditAmount->value(), false, BitcoinUnits::SeparatorStyle::NEVER) : "0";
        ExecRPCCommand::appendParam(lstParams, PARAM_AMOUNT, amount);
        ExecRPCCommand::appendParam(lstParams, PARAM_GASLIMIT, QString::number(gasLimit));
        ExecRPCCommand::appendParam(lstParams, PARAM_GASPRICE, BitcoinUnits::format(unit, gasPrice, false, BitcoinUnits::SeparatorStyle::NEVER));
        ExecRPCCommand::appendParam(lstParams, PARAM_SENDER, ui->lineEditSenderAddress->currentText());

        QString questionString;
        if (bCreateUnsigned) {
            questionString.append(tr("Do you want to draft this transaction?"));
            questionString.append("<br /><span style='font-size:10pt;'>");
            questionString.append(tr("Please, review your transaction proposal. This will produce a Partially Signed Qtum Transaction (PSBT) which you can copy and then sign with e.g. an offline %1 wallet, or a PSBT-compatible hardware wallet.").arg(PACKAGE_NAME));
            questionString.append("</span>");
            questionString.append(tr("<br /><br />Send to the contract:<br />"));
        } else {
            questionString.append(tr("Are you sure you want to send to the contract: <br /><br />"));
        }
        questionString.append(tr("<b>%1</b>?")
                              .arg(ui->lineEditContractAddress->text()));

        const QString confirmation = bCreateUnsigned ? tr("Confirm sending to contract proposal.") : tr("Confirm sending to contract.");
        const bool enable_send{!bCreateUnsigned};
        const bool always_show_unsigned{m_model->getOptionsModel()->getEnablePSBTControls()};
        SendConfirmationDialog confirmationDialog(confirmation, questionString, "", "", SEND_CONFIRM_DELAY, enable_send, always_show_unsigned, this);
        confirmationDialog.exec();
        QMessageBox::StandardButton retval = (QMessageBox::StandardButton)confirmationDialog.result();
        if(retval == QMessageBox::Yes || retval == QMessageBox::Save)
        {
            // Execute RPC command line
            if(errorMessage.isEmpty() && m_execRPCCommand->exec(m_model->node(), m_model, lstParams, result, resultJson, errorMessage))
            {
                if(bCreateUnsigned)
                {
                    QVariantMap variantMap = result.toMap();
                    GUIUtil::setClipboard(variantMap.value("psbt").toString());
                    Q_EMIT message(tr("PSBT copied"), "Copied to clipboard", CClientUIInterface::MSG_INFORMATION);
                }
                else
                {
                    bool isSent = true;
                    if(m_model->getSignPsbtWithHwiTool())
                    {
                        QVariantMap variantMap = result.toMap();
                        QString psbt = variantMap.value("psbt").toString();
                        if(!HardwareSignTx::process(this, m_model, psbt, variantMap))
                            isSent = false;
                        else
                            result = variantMap;
                    }

                    if(isSent)
                    {
                        ContractResult *widgetResult = new ContractResult(ui->stackedWidget);
                        widgetResult->setResultData(result, FunctionABI(), m_ABIFunctionField->getParamsValues(), ContractResult::SendToResult);
                        ui->stackedWidget->addWidget(widgetResult);
                        int position = ui->stackedWidget->count() - 1;
                        m_results = position == 1 ? 1 : m_results + 1;

                        m_tabInfo->addTab(position, tr("Result %1").arg(m_results));
                        m_tabInfo->setCurrent(position);
                    }
                }
            }
            else
            {
                QMessageBox::warning(this, tr("Send to contract"), errorMessage);
            }
        }
    }
}

void SendToContract::on_gasInfoChanged(quint64 blockGasLimit, quint64 minGasPrice, quint64 nGasPrice)
{
    Q_UNUSED(nGasPrice);
    ui->labelGasLimit->setToolTip(tr("Gas limit. Default = %1, Max = %2").arg(DEFAULT_GAS_LIMIT_OP_CREATE).arg(blockGasLimit));
    ui->labelGasPrice->setToolTip(tr("Gas price: QTUM price per gas unit. Default = %1, Min = %2").arg(QString::fromStdString(FormatMoney(DEFAULT_GAS_PRICE))).arg(QString::fromStdString(FormatMoney(minGasPrice))));
    ui->lineEditGasPrice->SetMinValue(minGasPrice);
    ui->lineEditGasLimit->setMaximum(blockGasLimit);
}

void SendToContract::on_updateSendToContractButton()
{
    int func = m_ABIFunctionField->getSelectedFunction();
    bool enabled = func >= -1;
    if(ui->lineEditContractAddress->text().isEmpty())
    {
        enabled = false;
    }
    enabled &= ui->stackedWidget->currentIndex() == 0;

    ui->pushButtonSendToContract->setEnabled(enabled);
}

void SendToContract::on_newContractABI()
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

    on_updateSendToContractButton();
}

void SendToContract::on_functionChanged()
{
    bool payable = isFunctionPayable();
    ui->lineEditAmount->setEnabled(payable);
    if(!payable)
    {
        ui->lineEditAmount->clear();
    }
}

void SendToContract::on_saveInfoClicked()
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

void SendToContract::on_loadInfoClicked()
{
    ContractBookPage dlg(m_platformStyle, this);
    dlg.setModel(m_model->getContractTableModel());
    if(dlg.exec())
    {
        ui->lineEditContractAddress->setText(dlg.getAddressValue());
        on_contractAddressChanged();
    }
}

void SendToContract::on_pasteAddressClicked()
{
    setContractAddress(QApplication::clipboard()->text());
}

void SendToContract::on_contractAddressChanged()
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

void SendToContract::updateDisplayUnit()
{
    if(m_model && m_model->getOptionsModel())
    {
        // Update sendAmount and gasPriceAmount with the current unit
        ui->lineEditGasPrice->setDisplayUnit(m_model->getOptionsModel()->getDisplayUnit());
        ui->lineEditAmount->setDisplayUnit(m_model->getOptionsModel()->getDisplayUnit());
    }
}

QString SendToContract::toDataHex(int func, QString& errorMessage)
{
    if(func == -1 || m_ABIFunctionField == NULL || m_contractABI == NULL)
    {
        std::string defSelector = FunctionABI::defaultSelector();
        return QString::fromStdString(defSelector);
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

bool SendToContract::isFunctionPayable()
{
    int func = m_ABIFunctionField->getSelectedFunction();
    if(func < 0) return true;
    FunctionABI function = m_contractABI->functions[func];
    return function.payable;
}
