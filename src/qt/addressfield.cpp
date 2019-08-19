// Copyright (c) 2016-2017 The Qtum Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/addressfield.h>
#include <qt/walletmodel.h>
#include <validation.h>
#include <key_io.h>
#include <qt/qvalidatedlineedit.h>
#include <qt/bitcoinaddressvalidator.h>
#include <script/standard.h>
#include <QLineEdit>
#include <QCompleter>

using namespace std;

AddressField::AddressField(QWidget *parent) :
    QComboBox(parent),
    m_addressType(AddressField::UTXO),
    m_addressTableModel(0),
    m_walletModel(0),
    m_addressColumn(0),
    m_typeRole(Qt::UserRole),
    m_senderAddress(false),
    m_includeZeroValue(false),
    m_isSetIncludeZeroValue(false)

{
    // Set editable state
    setComboBoxEditable(false);

    // Connect signals and slots
    connect(this, &AddressField::addressTypeChanged, this, &AddressField::on_addressTypeChanged);
}

QString AddressField::currentText() const
{
    if(isEditable())
    {
        return lineEdit()->text();
    }

    int index = currentIndex();
    if(index == -1)
    {
        return QString();
    }

    return itemText(index);
}

bool AddressField::isValidAddress()
{
    if(!isEditable())
    {
        if(currentIndex() != -1)
            return true;
        else
            return false;
    }

    ((QValidatedLineEdit*)lineEdit())->checkValidity();
    return ((QValidatedLineEdit*)lineEdit())->isValid();
}

void AddressField::setComboBoxEditable(bool editable)
{
    QValidatedLineEdit *validatedLineEdit = new QValidatedLineEdit(this);
    setLineEdit(validatedLineEdit);
    setEditable(editable);
    if(editable)
    {
        QValidatedLineEdit *validatedLineEdit = (QValidatedLineEdit*)lineEdit();
        validatedLineEdit->setCheckValidator(new BitcoinAddressCheckValidator(parent(), m_senderAddress));
        completer()->setCompletionMode(QCompleter::InlineCompletion);
        connect(validatedLineEdit, &QValidatedLineEdit::editingFinished, this, &AddressField::on_editingFinished);
    }
}

void AddressField::on_refresh()
{
    // Initialize variables
    QString currentAddress = currentText();
    m_stringList.clear();
    if(m_walletModel)
    {
        // Fill the list with address
        if(m_addressType == AddressField::UTXO)
        {
            QStringList addresses;

            // Add all available addresses
            if(m_includeZeroValue)
            {
                // Include zero or unconfirmed coins too
                addresses = m_allAddresses;
            }
            else
            {
                // List only the spendable coins
                addresses = m_spendableAddresses;
            }

            for(QString address : addresses) {
                appendAddress(address);
            }
        }
    }

    // Update the current index
    int index = m_stringList.indexOf(currentAddress);
    m_stringModel.setStringList(m_stringList);
    setModel(&m_stringModel);
    setCurrentIndex(index);
}

void AddressField::on_addressTypeChanged()
{
    m_stringList.clear();
    on_refresh();
}

void AddressField::on_editingFinished()
{
    Q_EMIT editTextChanged(QComboBox::currentText());
}

void AddressField::appendAddress(const QString &strAddress)
{
    if(m_walletModel)
    {
        CTxDestination address = DecodeDestination(strAddress.toStdString());
        if(m_senderAddress && !IsValidContractSenderAddress(address))
            return;

        m_stringList.append(strAddress);
    }
}

void AddressField::setTypeRole(int typeRole)
{
    m_typeRole = typeRole;
}

void AddressField::setAddressColumn(int addressColumn)
{
    m_addressColumn = addressColumn;
}

void AddressField::setSenderAddress(bool senderAddress)
{
    m_senderAddress = senderAddress;
}

void AddressField::setWalletModel(WalletModel *walletModel)
{
    m_walletModel = walletModel;

    connect(m_walletModel, &WalletModel::availableAddressesChanged, this, &AddressField::on_availableAddressesChanged);
}

void AddressField::on_availableAddressesChanged(QStringList spendableAddresses, QStringList allAddresses, bool includeZeroValue)
{
    // The addresses are checked that are mine in the model
    m_spendableAddresses = spendableAddresses;
    m_allAddresses = allAddresses;

    // Use the slot value as default in case the Include Zero Value is not set in the component
    if(!m_isSetIncludeZeroValue)
    {
        m_includeZeroValue = includeZeroValue;
    }

    on_refresh();
}

void AddressField::setIncludeZeroValue(bool includeZeroValue)
{
    m_includeZeroValue = includeZeroValue;
    m_isSetIncludeZeroValue = true;

    on_refresh();
}
