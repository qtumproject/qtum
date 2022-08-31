#include <qt/nfturllabel.h>
#include <qt/platformstyle.h>
#include <qt/styleSheet.h>
#include <qt/guiutil.h>
#include <qtum/nftconfig.h>

#include <QDesktopServices>
#include <QUrl>
#include <QIcon>
#include <QMessageBox>

#define NFT_URI_ITEM_WIDTH 20
#define NFT_URI_ITEM_HEIGHT 30
#define NFT_HIDDEN_ITEM_WIDTH 30
#define NFT_HIDDEN_ITEM_HEIGHT 30

NftUrlLabel::NftUrlLabel(QWidget* parent)
    : QLabel(parent)
{
    connect(this, &NftUrlLabel::clicked, this, &NftUrlLabel::on_clicked);
    QColor colorIcon = GetStringStyleValue("nfturllabel/color-icon", "#575757");
    m_defPixmap = PlatformStyle::SingleColorIcon(QStringLiteral(":/icons/play"), colorIcon).pixmap(NFT_URI_ITEM_WIDTH, NFT_URI_ITEM_HEIGHT);
    m_hiddenPixmap = PlatformStyle::SingleColorIcon(QStringLiteral(":/icons/hidden"), colorIcon).pixmap(NFT_HIDDEN_ITEM_WIDTH, NFT_HIDDEN_ITEM_HEIGHT);
    updateThumbnail();
}

NftUrlLabel::~NftUrlLabel() {}

void NftUrlLabel::mouseReleaseEvent(QMouseEvent* event)
{
    if(event && event->button() == Qt::LeftButton)
        Q_EMIT clicked();
}

QString NftUrlLabel::getNftUrl() const
{
    return m_nftUrl;
}

void NftUrlLabel::setNftUrl(const QString &value)
{
    m_nftUrl = value;
}

void NftUrlLabel::on_clicked()
{
    if(m_showThumbnail)
    {
        if(NftConfig::Instance().IsUrlValid(m_nftUrl.toStdString()))
        {
            QString message = tr("You are about to open %1 NFT url:\n"
                                 "%2\nMake sure you trust the source of this NFT as opening untrusted URLs can be dangerous."
                                 "\n\nDo you want to open this NFT url?").arg(m_nftName, m_nftUrl);
            if(QMessageBox::question(this, tr("Open NFT url"), message) == QMessageBox::Yes)
            {
                QDesktopServices::openUrl(QUrl(m_nftUrl, QUrl::TolerantMode));
            }
        }
        else
        {
            QString message = tr("This NFT has an invalid URL and cannot be opened:\n%1.").arg(m_nftUrl);
            QMessageBox::warning(this, tr("Open NFT url"), message);
        }
    }
}

QString NftUrlLabel::getThumbnail() const
{
    return m_thumbnail;
}

void NftUrlLabel::setThumbnail(const QString &thumbnail)
{
    m_thumbnail = thumbnail;
    updateThumbnail();
}

QString NftUrlLabel::getNftName() const
{
    return m_nftName;
}

void NftUrlLabel::setNftName(const QString &nftName)
{
    m_nftName = nftName;
}

bool NftUrlLabel::getShowThumbnail() const
{
    return m_showThumbnail;
}

void NftUrlLabel::setShowThumbnail(bool showThumbnail)
{
    m_showThumbnail = showThumbnail;
    updateThumbnail();
}

void NftUrlLabel::updateThumbnail()
{
    QPixmap pixmap;
    if(!m_showThumbnail)
    {
        setPixmap(m_hiddenPixmap);
    }
    else if(!m_thumbnail.isEmpty() &&
            GUIUtil::Base64ToThumbnail(m_thumbnail, pixmap))
    {
        setPixmap(pixmap);
    }
    else
    {
        setPixmap(m_defPixmap);
    }
}
