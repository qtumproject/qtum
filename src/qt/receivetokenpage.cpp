#include <qt/receivetokenpage.h>
#include <qt/forms/ui_receivetokenpage.h>

#include <qt/guiutil.h>
#include <qt/guiconstants.h>
#include <qt/receiverequestdialog.h>
#include <qt/platformstyle.h>

ReceiveTokenPage::ReceiveTokenPage(const PlatformStyle *_platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReceiveTokenPage),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);
    connect(ui->copyAddressButton, &QToolButton::clicked, this, &ReceiveTokenPage::on_copyAddressClicked);
    ui->copyAddressButton->setVisible(false);
    setAddress("");
}

ReceiveTokenPage::~ReceiveTokenPage()
{
    delete ui;
}

void ReceiveTokenPage::setAddress(QString address)
{
    m_address = address;
    createQRCode();
}

void ReceiveTokenPage::setSymbol(QString symbol)
{
    QString addressText = symbol.isEmpty() ? "" : (QString("%1 %2:").arg(symbol, tr("Address")));
    ui->labelTokenAddressText->setText(addressText);
    setWindowTitle(QString("%1 %2").arg(symbol, tr("Receive")));
}

void ReceiveTokenPage::on_copyAddressClicked()
{
    if(!m_address.isEmpty())
        GUIUtil::setClipboard(m_address);
}

void ReceiveTokenPage::createQRCode()
{
    SendCoinsRecipient info;
    if(!m_address.isEmpty())
    {
        info.address = m_address;
        QString uri = GUIUtil::formatBitcoinURI(info);
        if(ui->lblQRCode->setQR(uri))
        {
            ui->widgetQRMargin->setVisible(true);
            ui->lblQRCode->setScaledContents(true);
        }
        else
        {
            ui->widgetQRMargin->setVisible(false);
        }
        ui->labelTokenAddress->setText(m_address);
        ui->copyAddressButton->setVisible(true);
    }
    else
    {
        ui->lblQRCode->clear();
        ui->labelTokenAddress->setText("");
        ui->labelTokenAddressText->setText("");
        ui->copyAddressButton->setVisible(false);
    }
}
