#include <qt/hardwaresigntxdialog.h>
#include <qt/forms/ui_hardwaresigntxdialog.h>
#include <qt/walletmodel.h>
#include <qt/qtumhwitool.h>
#include <qt/waitmessagebox.h>
#include <qt/hardwarekeystoredialog.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>

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
    if(d->tool->sendRawTransaction(d->hexTx))
        QDialog::accept();
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
