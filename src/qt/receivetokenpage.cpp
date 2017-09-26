#include "receivetokenpage.h"
#include "ui_receivetokenpage.h"

#include "guiconstants.h"
#include "receiverequestdialog.h"

ReceiveTokenPage::ReceiveTokenPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ReceiveTokenPage)
{
    ui->setupUi(this);
    ui->lineEditAmount->setStyleSheet(STYLE_UNDERLINE);

    SendCoinsRecipient info;
    info.address = "qXVCgU7rTAz9fnBmr62wVrLH3M4258bySt";
    if(ReceiveRequestDialog::createQRCode(ui->lblQRCode, info))
    {
        ui->lblQRCode->setScaledContents(true);
    }
}

ReceiveTokenPage::~ReceiveTokenPage()
{
    delete ui;
}
