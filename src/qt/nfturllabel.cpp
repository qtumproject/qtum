#include <qt/nfturllabel.h>

NftUrlLabel::NftUrlLabel(QWidget* parent)
    : QLabel(parent)
{}

NftUrlLabel::~NftUrlLabel() {}

void NftUrlLabel::mousePressEvent(QMouseEvent* event) {
    Q_UNUSED(event);
    Q_EMIT clicked();
}

QString NftUrlLabel::getNftUrl() const
{
    return nftUrl;
}

void NftUrlLabel::setNftUrl(const QString &value)
{
    nftUrl = value;
}
