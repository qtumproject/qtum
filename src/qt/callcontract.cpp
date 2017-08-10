#include "callcontract.h"
#include "ui_callcontract.h"
#include "platformstyle.h"

CallContract::CallContract(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CallContract)
{
    ui->setupUi(this);
    Q_UNUSED(platformStyle);
}

CallContract::~CallContract()
{
    delete ui;
}
