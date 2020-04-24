#include <qt/superstakerconfigdialog.h>
#include <qt/forms/ui_superstakerconfigdialog.h>

#include <validation.h>
#include <qt/walletmodel.h>
#include <qt/clientmodel.h>
#include <qt/optionsmodel.h>
#include <qt/bitcoinaddressvalidator.h>

class SuperStakerConfigDialogPriv
{
public:
    SuperStakerConfigDialogPriv():
        fee()
    {}

    QString address;
    int fee;
    QString hash;

    interfaces::SuperStakerInfo recommended;
};

SuperStakerConfigDialog::SuperStakerConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SuperStakerConfigDialog)
{
    ui->setupUi(this);
    d = new SuperStakerConfigDialogPriv();

    // Set defaults
    ui->sbMinFee->setMinimum(0);
    ui->sbMinFee->setMaximum(100);

    ui->leMinUtxo->SetMinValue(DEFAULT_STAKING_MIN_UTXO_VALUE);
    ui->leMinUtxo->setValue(DEFAULT_STAKING_MIN_UTXO_VALUE);

    ui->cbListType->addItem(tr("Accept all"), All);
    ui->cbListType->addItem(tr("White list"), WhiteList);
    ui->cbListType->addItem(tr("Black list"), BlackList);

    ui->buttonOk->setEnabled(false);

    connect(ui->cbRecommended, &QCheckBox::stateChanged, this, &SuperStakerConfigDialog::changeConfigEnabled);
    connect(ui->cbCustom, &QCheckBox::stateChanged, this, &SuperStakerConfigDialog::changeConfigEnabled);
    connect(ui->cbListType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SuperStakerConfigDialog::chooseAddressType);

    connect(ui->sbMinFee, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SuperStakerConfigDialog::on_enableOkButton);
    connect(ui->leMinUtxo, &BitcoinAmountField::valueChanged, this,  &SuperStakerConfigDialog::on_enableOkButton);
    connect(ui->cbListType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SuperStakerConfigDialog::on_enableOkButton);
    connect(ui->textAddressList, &QValidatedTextEdit::textChanged, this, &SuperStakerConfigDialog::on_enableOkButton);

    changeConfigEnabled();
    setAddressListVisible(false);

    ui->textAddressList->setCheckValidator(new BitcoinAddressCheckValidator(parent, true), true, true);
}

SuperStakerConfigDialog::~SuperStakerConfigDialog()
{
    delete ui;
    delete d;
}

void SuperStakerConfigDialog::setModel(WalletModel *_model)
{
    m_model = _model;

    if (m_model && m_model->getOptionsModel())
        connect(m_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &SuperStakerConfigDialog::updateDisplayUnit);

    if(m_model)
    {
        d->recommended = m_model->wallet().getSuperStakerRecommendedConfig();
    }

    // update the display unit, to not use the default ("QTUM")
    updateDisplayUnit();
}

void SuperStakerConfigDialog::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;
}

void SuperStakerConfigDialog::setSuperStakerData(const QString &_address, const int &_fee, const QString &_hash)
{
    d->address = _address;
    d->fee = _fee;
    d->hash = _hash;

    updateData();
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

void SuperStakerConfigDialog::updateData()
{
    ui->txtStaker->setText(d->address);
    if(ui->cbRecommended->isChecked())
    {
        ui->sbMinFee->setValue(d->recommended.min_fee);
        ui->leMinUtxo->setValue(d->recommended.min_delegate_utxo);
        ui->cbListType->setCurrentIndex(d->recommended.delegate_address_type);
        QStringList addressList;
        for(std::string sAddress : d->recommended.delegate_address_list)
        {
            addressList.append(QString::fromStdString(sAddress));
        }
        ui->textAddressList->setLines(addressList);
    }
}
