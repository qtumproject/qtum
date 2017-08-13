#include "createcontract.h"
#include "ui_createcontract.h"
#include "platformstyle.h"
#include "guiconstants.h"

CreateContract::CreateContract(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CreateContract)
{
    ui->setupUi(this);
    ui->groupBoxOptional->setStyleSheet(STYLE_GROUPBOX);
    setLinkLabels();
    Q_UNUSED(platformStyle);
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
