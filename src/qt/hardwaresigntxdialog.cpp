#include <qt/hardwaresigntxdialog.h>
#include <qt/forms/ui_hardwaresigntxdialog.h>
#include <qt/walletmodel.h>
#include <qt/qtumhwitool.h>
#include <qt/waitmessagebox.h>

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
}

void HardwareSignTxDialog::on_sendButton_clicked()
{
    if(d->tool->sendRawTransaction(d->hexTx))
        QDialog::accept();
}
