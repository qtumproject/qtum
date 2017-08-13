#include "callcontract.h"
#include "ui_callcontract.h"
#include "platformstyle.h"
#include "guiconstants.h"

CallContract::CallContract(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CallContract)
{
    ui->setupUi(this);
    ui->groupBoxOptional->setStyleSheet(STYLE_GROUPBOX);
    Q_UNUSED(platformStyle);

    connect(ui->pushButtonClearAll, SIGNAL(clicked()), SLOT(on_clearAll_clicked()));
}

CallContract::~CallContract()
{
    delete ui;
}

void CallContract::on_clearAll_clicked()
{
    ui->lineEditContractAddress->clear();
    ui->lineEditDataHex->clear();
    ui->lineEditSenderAddress->clear();
}
