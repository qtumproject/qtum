#include "sendtocontract.h"
#include "ui_sendtocontract.h"
#include "platformstyle.h"
#include "guiconstants.h"

SendToContract::SendToContract(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SendToContract)
{
    ui->setupUi(this);
    ui->groupBoxOptional->setStyleSheet(STYLE_GROUPBOX);
    Q_UNUSED(platformStyle);

    connect(ui->pushButtonClearAll, SIGNAL(clicked()), SLOT(on_clearAll_clicked()));
}

SendToContract::~SendToContract()
{
    delete ui;
}

void SendToContract::on_clearAll_clicked()
{
    ui->lineEditContractAddress->clear();
    ui->lineEditDataHex->clear();
    ui->lineEditAmount->clear();
    ui->lineEditGasLimit->clear();
    ui->lineEditGasPrice->clear();
    ui->lineEditSenderAddress->clear();
    ui->comboBoxBroadcast->setCurrentIndex(0);
}
