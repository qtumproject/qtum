#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/hardwaresigntxdialog.h>
#include <qt/forms/ui_hardwaresigntxdialog.h>
#include <qt/walletmodel.h>
#include <qt/qtumhwitool.h>
#include <qt/waitmessagebox.h>
#include <qt/hardwarekeystoredialog.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/derivationpathdialog.h>
#include <qt/sendcoinsdialog.h>
#include <qt/bitcoinunits.h>
#include <qt/hardwaresigntx.h>

#include <QFile>
#include <QMessageBox>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

HardwareSignTxDialog::HardwareSignTxDialog(const QString &tx, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HardwareSignTxDialog)
{
    // Init variables
    ui->setupUi(this);
    d = new HardwareSignTx(this);
    ui->textEditTxData->setText(tx);
    ui->textEditTxData->setReadOnly(tx != "");
    ui->textEditTxDetails->setReadOnly(true);

    setStyleSheet("");

    // Connect slots
    connect(ui->textEditTxData, &QTextEdit::textChanged, this, &HardwareSignTxDialog::txChanged);
}

HardwareSignTxDialog::~HardwareSignTxDialog()
{
    delete ui;
}

void HardwareSignTxDialog::on_cancelButton_clicked()
{
    QDialog::reject();
}

void HardwareSignTxDialog::setModel(WalletModel *model)
{
    d->setModel(model);
    txChanged();
    if(!d->model->wallet().privateKeysDisabled())
    {
        ui->textEditTxData->setEnabled(false);
        ui->textEditTxDetails->setEnabled(false);
        ui->importButton->setEnabled(false);
        ui->signButton->setEnabled(false);
        ui->sendButton->setEnabled(false);
    }
}

void HardwareSignTxDialog::txChanged()
{
    QString psbt = ui->textEditTxData->toPlainText().trimmed();
    QString decoded;
    if(psbt != d->psbt)
    {
        // Decode psbt
        d->psbt = psbt;
        d->hexTx = "";
        d->complete = false;
        bool isOk = d->tool->decodePsbt(psbt, decoded);
        ui->textEditTxDetails->setText(decoded);
        ui->signButton->setEnabled(isOk);
        ui->sendButton->setEnabled(false);

        // Determine amount and fee
        if(isOk && !decoded.isEmpty())
        {
            CAmount amount = 0;
            CAmount fee;

            QJsonDocument doc = QJsonDocument::fromJson(decoded.toUtf8());
            QJsonObject jsonData = doc.object();

            QJsonObject tx = jsonData.value("tx").toObject();
            QJsonArray vouts = tx.value("vout").toArray();

            // Determine the amount
            for (int i = 0; i < vouts.count(); i++) {
                QJsonObject vout = vouts.at(i).toObject();
                QJsonObject scriptPubKey = vout.value("scriptPubKey").toObject();
                QJsonArray addresses = scriptPubKey.value("addresses").toArray();
                QString address = scriptPubKey.value("address").toString();
                addresses.push_back(address);
                bool sendToFound = false;
                for(int j = 0; j < addresses.count(); j++)
                {
                    std::string address = addresses.at(j).toString().toStdString();
                    if(!d->model->wallet().isMineAddress(address))
                    {
                        sendToFound = true;
                        break;
                    }
                }

                if(sendToFound)
                {
                    CAmount amountValue;
                    BitcoinUnits::parse(BitcoinUnits::BTC, vout.value("value").toVariant().toString(), &amountValue);
                    amount += amountValue;
                }
            }

            // Determine the fee
            BitcoinUnits::parse(BitcoinUnits::BTC, jsonData.value("fee").toVariant().toString(), &fee);

            ui->lineEditAmount->setValue(amount);
            ui->lineEditFee->setValue(fee);
        }
        else
        {
            // Cleat the fields
            ui->lineEditAmount->clear();
            ui->lineEditFee->clear();
        }
    }
}

void HardwareSignTxDialog::on_signButton_clicked()
{
    if(d->sign())
    {
        ui->sendButton->setEnabled(true);
        on_sendButton_clicked();
    }
}

void HardwareSignTxDialog::on_sendButton_clicked()
{
    // Send transaction
    QString questionString = tr("Are you sure you want to broadcast the transaction? <br />");
    SendConfirmationDialog confirmationDialog(tr("Confirm broadcast transaction."), questionString, "", "", SEND_CONFIRM_DELAY, true, false, this);
    confirmationDialog.exec();
    QMessageBox::StandardButton retval = (QMessageBox::StandardButton)confirmationDialog.result();
    if(retval == QMessageBox::Yes)
    {
        QVariantMap result;
        if(d->send(result))
        {
            QDialog::accept();
        }
    }
}

void HardwareSignTxDialog::on_importButton_clicked()
{
    // Import addresses and rescan
    bool rescan, importPKH, importP2SH, importBech32;
    QString pathPKH, pathP2SH, pathBech32;
    if(importAddressesData(rescan, importPKH, importP2SH, importBech32, pathPKH, pathP2SH, pathBech32))
    {
        d->model->importAddressesData(rescan, importPKH, importP2SH, importBech32, pathPKH, pathP2SH, pathBech32);
        QDialog::accept();
    }
}

bool HardwareSignTxDialog::importAddressesData(bool &rescan, bool &importPKH, bool &importP2SH, bool &importBech32, QString &pathPKH, QString &pathP2SH, QString &pathBech32)
{
    // Init import addresses data
    bool ret = true;
    rescan = false;
    importPKH = false;
    importP2SH = false;
    importBech32 = false;

    // Get list to import
    DerivationPathDialog dlg(this, d->model);
    ret &= dlg.exec() == QDialog::Accepted;
    if(ret) ret &= dlg.importAddressesData(rescan, importPKH, importP2SH, importBech32, pathPKH, pathP2SH, pathBech32);

    // Ask for device
    bool fDevice = importPKH || importP2SH || importBech32;
    if(fDevice) ret &= d->askDevice();

    return ret;
}

