#include <qt/derivationpathdialog.h>
#include <qt/forms/ui_derivationpathdialog.h>
#include <qt/qtumhwitool.h>
#include <outputtype.h>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

#define paternDerivationPath "^m/[0-9]{1,9}'/[0-9]{1,9}'/[0-9]{1,9}'$"
QString toHWIPath(const QString& path)
{
    if(path.isEmpty())
        return "";
    QString hwiPath = path;
    hwiPath.replace("'", "h");
    return hwiPath;
}

DerivationPathDialog::DerivationPathDialog(QWidget *parent, WalletModel* model, bool _create) :
    QDialog(parent),
    create(_create),
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

    // Set contract address validator
    QRegularExpression regEx;
    regEx.setPattern(paternDerivationPath);

    QRegularExpressionValidator *legacyValidator = new QRegularExpressionValidator(ui->txtLegacy);
    legacyValidator->setRegularExpression(regEx);
    ui->txtLegacy->setCheckValidator(legacyValidator);
    ui->txtLegacy->setText(QtumHwiTool::derivationPathPKH());
    ui->txtLegacy->setPlaceholderText(QtumHwiTool::derivationPathPKH());

    QRegularExpressionValidator *P2SHValidator = new QRegularExpressionValidator(ui->txtP2SH);
    P2SHValidator->setRegularExpression(regEx);
    ui->txtP2SH->setCheckValidator(P2SHValidator);
    ui->txtP2SH->setText(QtumHwiTool::derivationPathP2SH());
    ui->txtP2SH->setPlaceholderText(QtumHwiTool::derivationPathP2SH());

    QRegularExpressionValidator *segWitValidator = new QRegularExpressionValidator(ui->txtSegWit);
    segWitValidator->setRegularExpression(regEx);
    ui->txtSegWit->setCheckValidator(segWitValidator);
    ui->txtSegWit->setText(QtumHwiTool::derivationPathBech32());
    ui->txtSegWit->setPlaceholderText(QtumHwiTool::derivationPathBech32());

    if(model && create)
    {
        ui->cbRescan->setChecked(true);
        ui->cbRescan->setEnabled(false);

        OutputType type = model->wallet().getDefaultAddressType();
        switch (type) {
        case OutputType::LEGACY:
            ui->cbLegacy->setChecked(true);
            ui->cbLegacy->setEnabled(false);
            break;
        case OutputType::P2SH_SEGWIT:
            ui->cbP2SH->setChecked(true);
            ui->cbP2SH->setEnabled(false);
            break;
        case OutputType::BECH32:
            ui->cbSegWit->setChecked(true);
            ui->cbSegWit->setEnabled(false);
            break;
        default:
            break;
        }
    }

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

bool DerivationPathDialog::importAddressesData(bool &rescan, bool &importPKH, bool &importP2SH, bool &importBech32, QString& pathPKH, QString& pathP2SH, QString& pathBech32)
{
    rescan = ui->cbRescan->isChecked();
    importPKH = ui->cbLegacy->isChecked();
    importP2SH = ui->cbP2SH->isChecked();
    importBech32 = ui->cbSegWit->isChecked();
    pathPKH = toHWIPath(ui->txtLegacy->text());
    pathP2SH = toHWIPath(ui->txtP2SH->text());
    pathBech32 = toHWIPath(ui->txtSegWit->text());
    return isDataValid() && isDataSelected(rescan, importPKH, importP2SH, importBech32);
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
    ui->txtLegacy->checkValidity();
    ui->txtP2SH->checkValidity();
    ui->txtSegWit->checkValidity();
    return ui->txtLegacy->isValid() && ui->txtP2SH->isValid() && ui->txtSegWit->isValid();
}

bool DerivationPathDialog::isDataSelected(bool rescan, bool importPKH, bool importP2SH, bool importBech32)
{
    bool hasDerivation = importPKH || importP2SH || importBech32;
    if(create) return hasDerivation;
    return rescan || hasDerivation;
}
