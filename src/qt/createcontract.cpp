#include "createcontract.h"
#include "ui_createcontract.h"
#include "platformstyle.h"
#include "guiconstants.h"
#include "rpcconsole.h"

CreateContract::CreateContract(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CreateContract)
{
    ui->setupUi(this);
    ui->groupBoxOptional->setStyleSheet(STYLE_GROUPBOX);
    setLinkLabels();
    Q_UNUSED(platformStyle);

    connect(ui->pushButtonClearAll, SIGNAL(clicked()), SLOT(on_clearAll_clicked()));
}

CreateContract::~CreateContract()
{
    delete ui;
}

void CreateContract::setLinkLabels()
{
    ui->labelSolidity->setOpenExternalLinks(true);
    ui->labelSolidity->setText("<a href=\"https://chriseth.github.io/browser-solidity/#version=soljson-latest.js\">Solidity</a>");

    ui->labelContractTemplate->setOpenExternalLinks(true);
    ui->labelContractTemplate->setText("<a href=\"https://www.qtum.org\">Contract Template</a>");

    ui->labelGenerateBytecode->setOpenExternalLinks(true);
    ui->labelGenerateBytecode->setText("<a href=\"https://www.qtum.org\">Generate Bytecode</a>");
}

void CreateContract::on_clearAll_clicked()
{
    ui->textEditBytecode->clear();
    ui->lineEditGasLimit->clear();
    ui->lineEditGasPrice->clear();
    ui->lineEditSenderAddress->clear();
    ui->comboBoxBroadcast->setCurrentIndex(0);
}
