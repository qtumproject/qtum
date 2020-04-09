#include <qt/superstakerconfigdialog.h>
#include <qt/forms/ui_superstakerconfigdialog.h>

#include <validation.h>
#include <qt/walletmodel.h>
#include <qt/clientmodel.h>
#include <qt/optionsmodel.h>
#include <qt/bitcoinaddressvalidator.h>

SuperStakerConfigDialog::SuperStakerConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SuperStakerConfigDialog)
{
    ui->setupUi(this);

    // Set defaults
    ui->sbFee->setMinimum(0);
    ui->sbFee->setMaximum(100);

    ui->leMinUtxo->SetMinValue(DEFAULT_STAKING_MIN_UTXO_VALUE);

    ui->cbListType->addItem(tr("Accept all"), All);
    ui->cbListType->addItem(tr("White list"), WhiteList);
    ui->cbListType->addItem(tr("Black list"), BlackList);

    ui->buttonOk->setEnabled(false);

    connect(ui->cbRecommended, &QCheckBox::stateChanged, this, &SuperStakerConfigDialog::changeConfigEnabled);
    connect(ui->cbCustom, &QCheckBox::stateChanged, this, &SuperStakerConfigDialog::changeConfigEnabled);
    connect(ui->cbListType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SuperStakerConfigDialog::chooseAddressType);

    connect(ui->sbFee, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SuperStakerConfigDialog::on_enableOkButton);
    connect(ui->leMinUtxo, &BitcoinAmountField::valueChanged, this,  &SuperStakerConfigDialog::on_enableOkButton);
    connect(ui->cbListType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SuperStakerConfigDialog::on_enableOkButton);
    connect(ui->textAddressList, &QValidatedTextEdit::textChanged, this, &SuperStakerConfigDialog::on_enableOkButton);

    changeConfigEnabled();
    setAddressListVisible(false);

    ui->textAddressList->setMultiLineAddressField(true);
    ui->textAddressList->setCheckValidator(new BitcoinAddressCheckValidator(parent));
}

SuperStakerConfigDialog::~SuperStakerConfigDialog()
{
    delete ui;
}

void SuperStakerConfigDialog::setModel(WalletModel *_model)
{
    m_model = _model;

    if (m_model && m_model->getOptionsModel())
        connect(m_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &SuperStakerConfigDialog::updateDisplayUnit);

    // update the display unit, to not use the default ("QTUM")
    updateDisplayUnit();
}

void SuperStakerConfigDialog::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;
}

void SuperStakerConfigDialog::setSuperStakerData(const QString &_address, const QString &_hash)
{
    address = _address;
    hash = _hash;
}

void SuperStakerConfigDialog::chooseAddressType(int idx)
{
    switch(ui->cbListType->itemData(idx).toInt())
    {
    case All:
        setAddressListVisible(false);
        break;
    case WhiteList: {
        setAddressListVisible(true);
        ui->labelAddressList->setText(ui->cbListType->currentText());
    } break;
    case BlackList: {
        setAddressListVisible(true);
        ui->labelAddressList->setText(ui->cbListType->currentText());
    } break;
    }
}

void SuperStakerConfigDialog::accept()
{
    QDialog::accept();
}

void SuperStakerConfigDialog::reject()
{
    QDialog::reject();
}

void SuperStakerConfigDialog::on_buttonOk_clicked()
{
    accept();
}

void SuperStakerConfigDialog::on_buttonCancel_clicked()
{
    reject();
}

void SuperStakerConfigDialog::changeConfigEnabled()
{
    ui->frameStakerConfig->setEnabled(ui->cbRecommended->isChecked() ? false : true);
}

void SuperStakerConfigDialog::updateDisplayUnit()
{
    if(m_model && m_model->getOptionsModel())
    {
        // Update gasPriceAmount with the current unit
        ui->leMinUtxo->setDisplayUnit(m_model->getOptionsModel()->getDisplayUnit());
    }
}

void SuperStakerConfigDialog::setAddressListVisible(bool _visible)
{
    ui->labelAddressList->setVisible(_visible);
    ui->textAddressList->setVisible(_visible);
}

void SuperStakerConfigDialog::on_enableOkButton()
{
    ui->buttonOk->setEnabled(true);
}
