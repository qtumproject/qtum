#include "sendtocontract.h"
#include "ui_sendtocontract.h"
#include "platformstyle.h"

SendToContract::SendToContract(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SendToContract)
{
    ui->setupUi(this);
    Q_UNUSED(platformStyle);
}

SendToContract::~SendToContract()
{
    delete ui;
}
