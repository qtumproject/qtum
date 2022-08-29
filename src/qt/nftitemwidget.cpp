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
    if(m_type == New)
    {
        ui->buttonCreate->setIcon(platformStyle->MultiStatesIcon(":/icons/plus_full", PlatformStyle::PushButtonIcon));
        updateLogo();
    }
    else
    {
        ui->buttonSend->setIcon(m_platfromStyle->MultiStatesIcon(":/icons/send", PlatformStyle::PushButton));
        ui->toolShow->setIcon(platformStyle->MultiStatesIcon(":/icons/hide", ":/icons/show"));
        ui->toolShow->setCheckable(true);
        ui->toolShow->setChecked(true);
    }
}

NftItemWidget::~NftItemWidget()
{
    delete ui;
}

void NftItemWidget::setData(const QString &nftName, const QString &nftBalance, const QString &nftOwner, const QString& nftDesc, const QString& nftUrl, const QString& thumbnail, bool showThumbnail, bool watch)
{
    QString textName;
    QString tooltipName;
    getLabelText(nftName, textName, tooltipName);
    if(textName != ui->nftName->text())
    {
        ui->nftName->setText(textName);
        ui->nftUrl->setNftName(textName);
    }
    if(tooltipName != ui->nftName->toolTip())
        ui->nftName->setToolTip(tooltipName);

    QString textDesc;
    QString tooltipDesc;
    getLabelText(nftDesc, textDesc, tooltipDesc);
    if(textDesc != ui->nftDesc->text())
        ui->nftDesc->setText(textDesc);
    if(tooltipDesc != ui->nftDesc->toolTip())
        ui->nftDesc->setToolTip(tooltipDesc);

    if(nftOwner != ui->nftOwner->text())
        ui->nftOwner->setText(nftOwner);

    if(nftBalance != ui->nftBalance->text())
        ui->nftBalance->setText(nftBalance);

    if(nftUrl != ui->nftUrl->getNftUrl())
        ui->nftUrl->setNftUrl(nftUrl);

    if(thumbnail != ui->nftUrl->getThumbnail())
        ui->nftUrl->setThumbnail(thumbnail);

    if(showThumbnail != ui->nftUrl->getShowThumbnail())
        ui->nftUrl->setShowThumbnail(showThumbnail);
    ui->toolShow->setChecked(showThumbnail);

    ui->buttonSend->setEnabled(!watch);
}

void NftItemWidget::setPosition(int position)
{
    m_position = position;
}

void NftItemWidget::on_buttonCreate_clicked()
{
    Q_EMIT clicked(m_position, Buttons::Create);
}

void NftItemWidget::on_buttonSend_clicked()
{
    Q_EMIT clicked(m_position, Buttons::Send);
}

void NftItemWidget::on_toolShow_clicked()
{
    Q_EMIT clicked(m_position, Buttons::ShowThumbnail);
    bool showThumbnail = ui->toolShow->isChecked();
    ui->nftUrl->setShowThumbnail(showThumbnail);
}

int NftItemWidget::position() const
{
    return m_position;
}

void NftItemWidget::updateLogo()
{
    QPixmap pixmap = m_platfromStyle->MultiStatesIcon(":/icons/token").pixmap(NFT_ITEM_ICONSIZE, NFT_ITEM_ICONSIZE);;
    ui->nftBalance->setPixmap(pixmap);
}

void NftItemWidget::getLabelText(const QString &itemName, QString &itemText, QString &itemTooltip)
{
    if(itemName.length() > 50)
    {
        itemText = itemName.mid(0, 47) + "...";
        itemTooltip = itemName;
    }
    else
    {
        itemText = itemName;
        itemTooltip = "";
    }
}
