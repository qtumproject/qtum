#include <qt/hardwaresigntxdialog.h>
#include <qt/forms/ui_hardwaresigntxdialog.h>
#include <qt/walletmodel.h>
#include <qt/qtumhwitool.h>

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
        bool isOk = d->tool->decodePsbt(psbt, decoded);
        ui->textEditTxDetails->setText(decoded);
        ui->signButton->setEnabled(isOk);
        ui->sendButton->setEnabled(false);
    }
}
