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

#include <QMessageBox>

class HardwareSignTxDialogPriv
{
public:
    HardwareSignTxDialogPriv(QObject *parent)
    {
        tool = new QtumHwiTool(parent);
    }

    WalletModel* model = 0;
    QtumHwiTool* tool = 0;
    QString psbt;
    QString hexTx;
    bool complete = false;
};

HardwareSignTxDialog::HardwareSignTxDialog(const QString &tx, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HardwareSignTxDialog)
{
    // Init variables
    ui->setupUi(this);
    d = new HardwareSignTxDialogPriv(this);
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
    delete d;
}

void HardwareSignTxDialog::on_cancelButton_clicked()
{
    QDialog::reject();
}

void HardwareSignTxDialog::setModel(WalletModel *model)
{
    d->model = model;
    d->tool->setModel(model);
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
        d->psbt = psbt;
        d->hexTx = "";
        d->complete = false;
        bool isOk = d->tool->decodePsbt(psbt, decoded);
        ui->textEditTxDetails->setText(decoded);
        ui->signButton->setEnabled(isOk);
        ui->sendButton->setEnabled(false);
    }
}

void HardwareSignTxDialog::on_signButton_clicked()
{
    if(askDevice())
    {
        WaitMessageBox dlg(tr("Ledger Status"), tr("Confirm Transaction on your Ledger device..."), [this]() {
            QString fingerprint = d->model->getFingerprint();
            QString psbt = d->psbt;
            d->hexTx = "";
            d->complete = false;
            bool ret = d->tool->signTx(fingerprint, psbt);
            if(ret) ret &= d->tool->finalizePsbt(psbt, d->hexTx, d->complete);
            if(d->complete)
            {
                ui->sendButton->setEnabled(true);
            }
        }, this);

        dlg.exec();

        if(!d->complete)
        {
            QMessageBox::warning(this, tr("Sign failed"), tr("The transaction has no a complete set of signatures."));
        }
    }
}

void HardwareSignTxDialog::on_sendButton_clicked()
{
    QString questionString = tr("Are you sure you want to broadcast the transaction? <br />");
    SendConfirmationDialog confirmationDialog(tr("Confirm broadcast transaction."), questionString, "", "", SEND_CONFIRM_DELAY, tr("Send"), this);
    confirmationDialog.exec();
    QMessageBox::StandardButton retval = (QMessageBox::StandardButton)confirmationDialog.result();
    if(retval == QMessageBox::Yes)
    {
        if(d->tool->sendRawTransaction(d->hexTx))
        {
            QDialog::accept();
        }
        else
        {
            QString errorMessage = d->tool->errorMessage();
            if(errorMessage.isEmpty()) errorMessage = tr("Unknown transaction error");
            QMessageBox::warning(this, tr("Broadcast transaction"), errorMessage);
        }
    }
}

bool HardwareSignTxDialog::askDevice()
{
    // Check if the HWI tool exist
    QString hwiToolPath = GUIUtil::getHwiToolPath();
    if(!QFile::exists(hwiToolPath))
    {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("HWI tool not found"));
        msgBox.setTextFormat(Qt::RichText);
        msgBox.setText(tr("HWI tool not found at path \"%1\".<br>Please download it from %2 and add the path to the settings.").arg(hwiToolPath, QTUM_HWI_TOOL));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        return false;
    }

    // Ask for ledger
    QString fingerprint = d->model->getFingerprint();
    QString title = tr("Connect Ledger");
    QString message = tr("Please insert your Ledger (%1). Verify the cable is connected and that no other application is using it.\n\nTry to connect again?");
    if(HardwareKeystoreDialog::AskDevice(fingerprint, title, message.arg(fingerprint), this))
    {
        d->model->setFingerprint(fingerprint);
        return true;
    }

    d->model->setFingerprint("");
    return false;
}

void HardwareSignTxDialog::on_importButton_clicked()
{
    bool rescan, importPKH, importP2SH, importBech32;
    if(importAddressesData(rescan, importPKH, importP2SH, importBech32))
    {
        d->model->importAddressesData(rescan, importPKH, importP2SH, importBech32);
        QDialog::accept();
    }
}

bool HardwareSignTxDialog::importAddressesData(bool &rescan, bool &importPKH, bool &importP2SH, bool &importBech32)
{
    // Init import addresses data
    bool ret = true;
    rescan = false;
    importPKH = false;
    importP2SH = false;
    importBech32 = false;

    // Get list to import
    DerivationPathDialog dlg(this);
    ret &= dlg.exec() == QDialog::Accepted;
    if(ret) ret &= dlg.importAddressesData(rescan, importPKH, importP2SH, importBech32);

    // Ask for device
    bool fDevice = importPKH || importP2SH || importBech32;
    if(fDevice) ret &= askDevice();

    return ret;
}
