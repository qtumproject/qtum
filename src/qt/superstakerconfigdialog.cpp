#include <qt/superstakerconfigdialog.h>
#include <qt/forms/ui_superstakerconfigdialog.h>

#include <validation.h>
#include <qt/walletmodel.h>
#include <qt/clientmodel.h>
#include <qt/optionsmodel.h>
#include <qt/bitcoinaddressvalidator.h>

#include <QMessageBox>

class SuperStakerConfigDialogPriv
{
public:
    SuperStakerConfigDialogPriv()
    {}

    interfaces::SuperStakerInfo recommended;
    interfaces::SuperStakerInfo staker;
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

    ui->leMinUtxo->setValue(DEFAULT_STAKING_MIN_UTXO_VALUE);

    ui->cbListType->addItem(tr("Accept all"), All);
    ui->cbListType->addItem(tr("Allow list"), AllowList);
    ui->cbListType->addItem(tr("Exclude list"), ExcludeList);

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

void SuperStakerConfigDialog::setSuperStakerData(const QString &hash)
{
    if(m_model && !hash.isEmpty())
    {
        uint256 id;
        id.SetHex(hash.toStdString());
        if(id == d->staker.hash) return;
        d->staker = m_model->wallet().getSuperStaker(id);
    }

    ui->cbRecommended->setChecked(!d->staker.custom_config);
    ui->cbCustom->setChecked(d->staker.custom_config);

    updateData();
    ui->buttonOk->setEnabled(false);
}

void SuperStakerConfigDialog::chooseAddressType(int idx)
{
    switch(ui->cbListType->itemData(idx).toInt())
    {
    case All:
        setAddressListVisible(false);
        break;
    case AllowList: {
        setAddressListVisible(true);
        ui->labelAddressList->setText(ui->cbListType->currentText());
    } break;
    case ExcludeList: {
        setAddressListVisible(true);
        ui->labelAddressList->setText(ui->cbListType->currentText());
    } break;
    }
}

void SuperStakerConfigDialog::accept()
{
    clearAll();
    QDialog::accept();
}

void SuperStakerConfigDialog::reject()
{
    clearAll();
    QDialog::reject();
}

void SuperStakerConfigDialog::on_buttonOk_clicked()
{
    if(m_model)
    {
        if(ui->textAddressList->isVisible() && !ui->textAddressList->isValid())
            return;

        QString questionString = tr("Are you sure you want to update configuration for staker<br /><br />");
        questionString.append(tr("<b>%1</b>?")
                              .arg(ui->txtStaker->text()));

        QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm configuration change."), questionString,
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

        if(retval == QMessageBox::Yes)
        {
            interfaces::SuperStakerInfo updatedStaker;
            updatedStaker.hash = d->staker.hash;
            updatedStaker.staker_address = d->staker.staker_address;
            updatedStaker.staker_name = d->staker.staker_name;
            updatedStaker.time = d->staker.time;
            updatedStaker.custom_config = ui->cbCustom->isChecked();
            if(updatedStaker.custom_config)
            {
                updatedStaker.min_fee = ui->sbMinFee->value();
                updatedStaker.min_delegate_utxo = ui->leMinUtxo->value();
                updatedStaker.delegate_address_type = ui->cbListType->currentIndex();
                if(updatedStaker.delegate_address_type)
                {
                    std::vector<std::string> delegateAddressList;
                    for(QString address : ui->textAddressList->getLines())
                    {
                        delegateAddressList.push_back(address.toStdString());
                    }
                    updatedStaker.delegate_address_list = delegateAddressList;
                }
            }
            m_model->wallet().addSuperStakerEntry(updatedStaker);

            accept();
        }
    }
}

void SuperStakerConfigDialog::on_buttonCancel_clicked()
{
    reject();
}

void SuperStakerConfigDialog::changeConfigEnabled()
{
    ui->frameStakerConfig->setEnabled(ui->cbRecommended->isChecked() ? false : true);
    updateData();
    ui->buttonOk->setEnabled(true);
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
    ui->txtStaker->setText(QString::fromStdString(d->staker.staker_name));

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

    if(ui->cbCustom->isChecked() && d->staker.custom_config)
    {
        ui->sbMinFee->setValue(d->staker.min_fee);
        ui->leMinUtxo->setValue(d->staker.min_delegate_utxo);
        ui->cbListType->setCurrentIndex(d->staker.delegate_address_type);
        QStringList addressList;
        for(std::string sAddress : d->staker.delegate_address_list)
        {
            addressList.append(QString::fromStdString(sAddress));
        }
        ui->textAddressList->setLines(addressList);
    }
}

void SuperStakerConfigDialog::clearAll()
{
    d->staker = interfaces::SuperStakerInfo();
}
