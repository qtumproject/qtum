#include <qt/derivationpathdialog.h>
#include <qt/forms/ui_derivationpathdialog.h>

DerivationPathDialog::DerivationPathDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DerivationPathDialog)
{
    ui->setupUi(this);

    // Connect signal and slots
    QObject::connect(ui->cbRescan, &QCheckBox::clicked, this, &DerivationPathDialog::updateWidgets);
    QObject::connect(ui->cbLegacy, &QCheckBox::clicked, this, &DerivationPathDialog::updateWidgets);
    QObject::connect(ui->cbP2SH, &QCheckBox::clicked, this, &DerivationPathDialog::updateWidgets);
    QObject::connect(ui->cbSegWit, &QCheckBox::clicked, this, &DerivationPathDialog::updateWidgets);
    QObject::connect(ui->txtLegacy, &QValidatedLineEdit::textChanged, this, &DerivationPathDialog::updateWidgets);
    QObject::connect(ui->txtP2SH, &QValidatedLineEdit::textChanged, this, &DerivationPathDialog::updateWidgets);
    QObject::connect(ui->txtSegWit, &QValidatedLineEdit::textChanged, this, &DerivationPathDialog::updateWidgets);

    updateWidgets();
}

DerivationPathDialog::~DerivationPathDialog()
{
    delete ui;
}

void DerivationPathDialog::on_cancelButton_clicked()
{
    QDialog::reject();
}

void DerivationPathDialog::on_okButton_clicked()
{
    QDialog::accept();
}

bool DerivationPathDialog::importAddressesData(bool &rescan, bool &importPKH, bool &importP2SH, bool &importBech32)
{
    rescan = ui->cbRescan->isChecked();
    importPKH = ui->cbLegacy->isChecked();
    importP2SH = ui->cbP2SH->isChecked();
    importBech32 = ui->cbSegWit->isChecked();
    return rescan || importPKH || importP2SH || importBech32;
}

void DerivationPathDialog::updateWidgets()
{
    bool legacy = ui->cbLegacy->isChecked();
    bool p2sh = ui->cbP2SH->isChecked();
    bool segWit = ui->cbSegWit->isChecked();
    bool rescan = ui->cbRescan->isChecked();
    bool enabled = isDataValid() && isDataSelected(rescan, legacy, p2sh, segWit);

    widgetEnabled(ui->okButton, enabled);
    widgetEnabled(ui->txtLegacy, legacy);
    widgetEnabled(ui->txtP2SH, p2sh);
    widgetEnabled(ui->txtSegWit, segWit);
}

void DerivationPathDialog::widgetEnabled(QWidget *widget, bool enabled)
{
    if(widget && widget->isEnabled() != enabled)
    {
        widget->setEnabled(enabled);
    }
}

bool DerivationPathDialog::isDataValid()
{
    return true;
}

bool DerivationPathDialog::isDataSelected(bool rescan, bool importPKH, bool importP2SH, bool importBech32)
{
    return rescan || importPKH || importP2SH || importBech32;
}
