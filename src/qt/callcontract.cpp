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
}

CallContract::~CallContract()
{
    delete ui;
}
