// Copyright (c) 2016-2017 The Qtum Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "addressfield.h"
#include "wallet/wallet.h"
#include "validation.h"
#include "base58.h"
#include <boost/foreach.hpp>
#include <QLineEdit>
#include <QCompleter>

using namespace std;

AddressField::AddressField(QWidget *parent) :
    QComboBox(parent),
    m_addressType(AddressField::UTXO)
{
    connect(this, SIGNAL(addressTypeChanged(AddressType)), SLOT(on_addressTypeChanged()));
    setEditable(true);
    completer()->setCompletionMode(QCompleter::PopupCompletion);
    connect(lineEdit(), SIGNAL(editingFinished()), this, SLOT(on_editingFinished()));

    // Limit the number of visible items in the list
    setStyleSheet("combobox-popup: 0;");
    setMaxVisibleItems(15);
}

QString AddressField::currentText() const
{
    int index = currentIndex();
    if(index == -1)
        return QString();

    return itemText(index);
}

void AddressField::on_refresh()
{
    // Initialize variables
    QString currentAddress = currentText();
    m_stringList.clear();
    vector<COutput> vecOutputs;
    assert(pwalletMain != NULL);

    // Fill the list with address
    if(m_addressType == AddressField::UTXO)
    {
        // Fill the list with UTXO
        LOCK2(cs_main, pwalletMain->cs_wallet);
        pwalletMain->AvailableCoins(vecOutputs);

        BOOST_FOREACH(const COutput& out, vecOutputs) {
            CTxDestination address;
            const CScript& scriptPubKey = out.tx->tx->vout[out.i].scriptPubKey;
            bool fValidAddress = ExtractDestination(scriptPubKey, address);

            if (fValidAddress)
            {
                QString strAddress = QString::fromStdString(CBitcoinAddress(address).ToString());
                if(!m_stringList.contains(strAddress))
                {
                    m_stringList.append(strAddress);
                }
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
