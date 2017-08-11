#include "sendtocontract.h"
#include "ui_sendtocontract.h"
#include "platformstyle.h"
#include "guiconstants.h"

SendToContract::SendToContract(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SendToContract)
{
    ui->setupUi(this);
    ui->groupBox->setStyleSheet(STYLE_GROUPBOX);
    Q_UNUSED(platformStyle);
}

SendToContract::~SendToContract()
{
    delete ui;
}
