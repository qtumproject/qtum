#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/nftitemwidget.h>
#include <qt/platformstyle.h>
#include <qt/forms/ui_nftitemwidget.h>

#include <QFile>

#define NFT_ITEM_ICONSIZE 24
NftItemWidget::NftItemWidget(const PlatformStyle *platformStyle, QWidget *parent, ItemType type) :
    QWidget(parent),
    ui(new Ui::NftItemWidget),
    m_platfromStyle(platformStyle),
    m_type(type),
    m_position(-1)

{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(type);
    ui->buttonSend->setIcon(m_platfromStyle->MultiStatesIcon(":/icons/send", PlatformStyle::PushButton));
    ui->buttonAdd->setIcon(platformStyle->MultiStatesIcon(":/icons/plus_full", PlatformStyle::PushButtonIcon));
    ui->nftLogo->setPixmap(platformStyle->MultiStatesIcon(m_type == New ? ":/icons/add_token" : ":/icons/token").pixmap(NFT_ITEM_ICONSIZE, NFT_ITEM_ICONSIZE));
}

NftItemWidget::~NftItemWidget()
{
    delete ui;
}

void NftItemWidget::setData(const QString &nftName, const QString &nftBalance, const QString &senderAddress, const QString &filename)
{
    if(nftName != ui->nftName->text())
        ui->nftName->setText(nftName);
    if(nftBalance != ui->nftBalance->text())
        ui->nftBalance->setText(nftBalance);
    if(senderAddress != ui->senderAddress->text())
        ui->senderAddress->setText(senderAddress);
    if(m_filename != filename)
    {
        m_filename = filename;
        updateLogo();
    }
}

void NftItemWidget::setPosition(int position)
{
    m_position = position;
}

void NftItemWidget::on_buttonAdd_clicked()
{
    Q_EMIT clicked(m_position, Buttons::Add);
}

void NftItemWidget::on_buttonSend_clicked()
{
    Q_EMIT clicked(m_position, Buttons::Send);
}

int NftItemWidget::position() const
{
    return m_position;
}

void NftItemWidget::updateLogo()
{
    QPixmap pixmap;
    if(QFile::exists(m_filename))
    {
        QIcon icon(m_filename);
        pixmap = icon.pixmap(ui->nftLogo->width(), ui->nftLogo->height());
    }
    else
    {
        pixmap = m_platfromStyle->MultiStatesIcon(":/icons/token").pixmap(NFT_ITEM_ICONSIZE, NFT_ITEM_ICONSIZE);
    }
    ui->nftLogo->setPixmap(pixmap);
}
