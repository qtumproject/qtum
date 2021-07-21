#include <qt/splitutxopage.h>
#include <qt/forms/ui_splitutxopage.h>

#include <qt/bitcoinunits.h>
#include <qt/execrpccommand.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/sendcoinsdialog.h>
#include <qt/walletmodel.h>
#include <validation.h>

namespace SplitUTXO_NS
{
static const QString PRC_COMMAND = "splitutxosforaddress";
static const QString PARAM_ADDRESS = "address";
static const QString PARAM_MIN_VALUE = "minValue";
static const QString PARAM_MAX_VALUE = "maxValue";
static const QString PARAM_MAX_OUTPUTS = "maxOutputs";
}
using namespace SplitUTXO_NS;

SplitUTXOPage::SplitUTXOPage(QWidget *parent, Mode mode) :
    QDialog(parent),
    ui(new Ui::SplitUTXOPage),
    m_model(nullptr),
    m_mode(mode)
{
    ui->setupUi(this);

    switch (m_mode) {
    case Normal:
        setWindowTitle(tr("Split coins for address"));
        ui->labelAddress->setText(tr("Address"));
        break;

    case Delegation:
        setWindowTitle(tr("Split coins for offline staking"));
        ui->labelAddress->setText(tr("Delegate address"));
        ui->labelDescription->setText(tr("Split coins for offline staking."));
        break;

    case SuperStaker:
        setWindowTitle(tr("Split coins for super staker"));
        ui->labelAddress->setText(tr("Staker address"));
        ui->labelDescription->setText(tr("Split coins for super staker. The UTXO value need to be minimum <b> %1 </b>.").
                                      arg(BitcoinUnits::formatHtmlWithUnit(BitcoinUnits::BTC, DEFAULT_STAKING_MIN_UTXO_VALUE)));
        ui->lineEditMinValue->SetMinValue(DEFAULT_STAKING_MIN_UTXO_VALUE);
        ui->lineEditMaxValue->SetMinValue(DEFAULT_STAKING_MIN_UTXO_VALUE);
        break;
    }

    ui->labelAddress->setToolTip(tr("The qtum address to split utxos."));
    ui->labelMinValue->setToolTip(tr("Select utxo which value is smaller than value (minimum 0.1 COIN)."));
    ui->labelMaxValue->setToolTip(tr("Select utxo which value is greater than value (minimum 0.1 COIN)."));
    ui->labelMaxOutputs->setToolTip(tr("Maximum outputs to create"));

    ui->lineEditAddress->setSenderAddress(true);
    ui->txtAddress->setReadOnly(true);
    ui->txtAddress->setVisible(false);

    QFont font = QApplication::font();
    font.setPointSizeF(font.pointSizeF() * 0.8);
    ui->labelDescription->setFont(font);

    // Set defaults
    ui->lineEditMinValue->setValue(DEFAULT_STAKING_MIN_UTXO_VALUE);
    ui->lineEditMaxValue->setValue(DEFAULT_STAKING_MIN_UTXO_VALUE * 2);
    ui->spinBoxMaxOutputs->setMinimum(1);
    ui->spinBoxMaxOutputs->setMaximum(10000);
    ui->spinBoxMaxOutputs->setValue(100);

    ui->splitCoinsButton->setEnabled(false);

    // Create new PRC command line interface
    QStringList lstMandatory;
    lstMandatory.append(PARAM_ADDRESS);
    lstMandatory.append(PARAM_MIN_VALUE);
    lstMandatory.append(PARAM_MAX_VALUE);

    QStringList lstOptional;
    lstOptional.append(PARAM_MAX_OUTPUTS);

    QMap<QString, QString> lstTranslations;
    lstTranslations[PARAM_ADDRESS] = ui->labelAddress->text();
    lstTranslations[PARAM_MIN_VALUE] = ui->labelMinValue->text();
    lstTranslations[PARAM_MAX_VALUE] = ui->labelMaxValue->text();
    lstTranslations[PARAM_MAX_OUTPUTS] = ui->labelMaxOutputs->text();

    m_execRPCCommand = new ExecRPCCommand(PRC_COMMAND, lstMandatory, lstOptional, lstTranslations, this);

    connect(ui->splitCoinsButton, &QPushButton::clicked, this, &SplitUTXOPage::on_splitCoinsClicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &SplitUTXOPage::on_cancelButtonClicked);
    connect(ui->lineEditAddress, &QComboBox::currentTextChanged, this, &SplitUTXOPage::on_updateSplitCoinsButton);
}

SplitUTXOPage::~SplitUTXOPage()
{
    delete ui;
}

void SplitUTXOPage::setModel(WalletModel *_model)
{
    m_model = _model;
    ui->lineEditAddress->setWalletModel(m_model);

    if (m_model && m_model->getOptionsModel())
        connect(m_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &SplitUTXOPage::updateDisplayUnit);

    // update the display unit, to not use the default ("QTUM")
    updateDisplayUnit();
}

void SplitUTXOPage::setAddress(const QString &address)
{
    ui->lineEditAddress->setVisible(false);
    ui->txtAddress->setVisible(true);
    ui->txtAddress->setText(address);

    if(m_mode == Normal)
        setWindowTitle(tr("Split coins for address %1").arg(address));

    ui->splitCoinsButton->setEnabled(true);
}

bool SplitUTXOPage::isDataValid()
{
    bool dataValid = true;
    ui->lineEditMinValue->setValid(true);
    ui->lineEditMaxValue->setValid(true);

    CAmount minValue = ui->lineEditMinValue->value();
    CAmount maxValue = ui->lineEditMaxValue->value();

    if(ui->lineEditAddress->isVisible() && !ui->lineEditAddress->isValidAddress())
        dataValid = false;
    if(minValue < COIN/10)
    {
        ui->lineEditMinValue->setValid(false);
        dataValid = false;
    }
    if(maxValue < COIN/10)
    {
        ui->lineEditMaxValue->setValid(false);
        dataValid = false;
    }
    if(minValue > COIN/10 && maxValue > COIN/10 && minValue > maxValue)
    {
        ui->lineEditMinValue->setValid(false);
        ui->lineEditMaxValue->setValid(false);
        dataValid = false;
    }

    return dataValid;
}

void SplitUTXOPage::clearAll()
{
    if(ui->lineEditAddress->isVisible())
    {
        ui->lineEditAddress->setCurrentIndex(-1);
        ui->splitCoinsButton->setEnabled(false);
    }
    ui->lineEditMinValue->setValue(DEFAULT_STAKING_MIN_UTXO_VALUE);
    ui->lineEditMaxValue->setValue(DEFAULT_STAKING_MIN_UTXO_VALUE * 2);
    ui->spinBoxMaxOutputs->setValue(100);
}

void SplitUTXOPage::accept()
{
    clearAll();
    QDialog::accept();
}

void SplitUTXOPage::reject()
{
    clearAll();
    QDialog::reject();
}

void SplitUTXOPage::updateDisplayUnit()
{
    if(m_model && m_model->getOptionsModel())
    {
        // Update min and max value with the current unit
        ui->lineEditMinValue->setDisplayUnit(m_model->getOptionsModel()->getDisplayUnit());
        ui->lineEditMaxValue->setDisplayUnit(m_model->getOptionsModel()->getDisplayUnit());
    }
}

void SplitUTXOPage::on_splitCoinsClicked()
{
    if(m_model && m_model->getOptionsModel())
    {
        if(!isDataValid())
            return;

        // Initialize variables
        QMap<QString, QString> lstParams;
        QVariant result;
        QString errorMessage;
        QString resultJson;
        int unit = BitcoinUnits::BTC;
        QString address = ui->lineEditAddress->isVisible() ? ui->lineEditAddress->currentText() : ui->txtAddress->text();
        CAmount minValue = ui->lineEditMinValue->value();
        CAmount maxValue = ui->lineEditMaxValue->value();
        CAmount maxOutputs = ui->spinBoxMaxOutputs->value();

        // Append params to the list
        ExecRPCCommand::appendParam(lstParams, PARAM_ADDRESS, address);
        ExecRPCCommand::appendParam(lstParams, PARAM_MIN_VALUE, BitcoinUnits::format(unit, minValue, false, BitcoinUnits::SeparatorStyle::NEVER));
        ExecRPCCommand::appendParam(lstParams, PARAM_MAX_VALUE, BitcoinUnits::format(unit, maxValue, false, BitcoinUnits::SeparatorStyle::NEVER));
        ExecRPCCommand::appendParam(lstParams, PARAM_MAX_OUTPUTS, QString::number(maxOutputs));

        QString questionString = tr("Are you sure you want to split coins for address");
        questionString.append(QString("<br/><br/><b>%1</b>?")
                              .arg(address));

        SendConfirmationDialog confirmationDialog(tr("Confirm splitting coins for address."), questionString, "", "", SEND_CONFIRM_DELAY, tr("Send"), this);
        confirmationDialog.exec();

        QMessageBox::StandardButton retval = (QMessageBox::StandardButton)confirmationDialog.result();
        if(retval == QMessageBox::Yes)
        {
            if(!m_execRPCCommand->exec(m_model->node(), m_model, lstParams, result, resultJson, errorMessage))
            {
                QMessageBox::warning(this, tr("Split coins for address"), errorMessage);
            }
            else
            {
                 QVariantMap variantMap = result.toMap();

                 QString selectedString = variantMap.value("selected").toString();
                 CAmount selected;
                 BitcoinUnits::parse(unit, selectedString, &selected);

                 QString splitedString = variantMap.value("splited").toString();
                 CAmount splited;
                 BitcoinUnits::parse(unit, splitedString, &splited);

                 int displayUnit = m_model->getOptionsModel()->getDisplayUnit();

                 QString infoString = tr("Selected: %1 less than %2 and above of %3.").
                         arg(BitcoinUnits::formatHtmlWithUnit(displayUnit, selected)).
                         arg(BitcoinUnits::formatHtmlWithUnit(displayUnit, minValue)).
                         arg(BitcoinUnits::formatHtmlWithUnit(displayUnit, maxValue));
                 infoString.append("<br/><br/>");
                 infoString.append(tr("Splitted: %1.").arg(BitcoinUnits::formatHtmlWithUnit(displayUnit, splited)));

                 QMessageBox::information(this, tr("Split coins for address"), infoString);

                 if(splited == selected || splited == 0)
                     accept();
            }
        }
    }
}

void SplitUTXOPage::on_cancelButtonClicked()
{
    reject();
}

void SplitUTXOPage::on_updateSplitCoinsButton()
{
    bool enabled = true;
    bool validAddress = !ui->lineEditAddress->isVisible() || (ui->lineEditAddress->isVisible() && !ui->lineEditAddress->currentText().isEmpty());

    if(!validAddress)
    {
        enabled = false;
    }

    ui->splitCoinsButton->setEnabled(enabled);
}
