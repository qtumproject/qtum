// Copyright (c) 2015-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/platformstyle.h>

#include <qt/guiconstants.h>

#include <QApplication>
#include <QColor>
#include <QImage>
#include <QPalette>

static const struct {
    const char *platformId;
    /** Show images on push buttons */
    const bool imagesOnButtons;
    /** Colorize single-color icons */
    const bool colorizeIcons;
    /** Extra padding/spacing in transactionview */
    const bool useExtraSpacing;
} platform_styles[] = {
    {"macosx", true, true, false},
    {"windows", true, true, false},
    /* Other: linux, unix, ... */
    {"other", true, true, false}
};
static const unsigned platform_styles_count = sizeof(platform_styles)/sizeof(*platform_styles);

namespace {
/* Local functions for colorizing single-color images */

void MakeSingleColorImage(QImage& img, const QColor& colorbase, double opacity = 1)
{
    //Opacity representation in percentage (0, 1) i.e. (0%, 100%)
    if(opacity > 1 && opacity < 0) opacity = 1;

    img = img.convertToFormat(QImage::Format_ARGB32);
    for (int x = img.width(); x--; )
    {
        for (int y = img.height(); y--; )
        {
            const QRgb rgb = img.pixel(x, y);
            img.setPixel(x, y, qRgba(colorbase.red(), colorbase.green(), colorbase.blue(), opacity * qAlpha(rgb)));
        }
    }
}
QPixmap MakeSingleColorPixmap(QImage& img, const QColor& colorbase, double opacity = 1)
{
    MakeSingleColorImage(img, colorbase, opacity);
    return QPixmap::fromImage(img);
}

QIcon ColorizeIcon(const QIcon& ico, const QColor& colorbase, double opacity = 1)
{
    QIcon new_ico;
    for (const QSize& sz : ico.availableSizes())
    {
        QImage img(ico.pixmap(sz).toImage());
        MakeSingleColorImage(img, colorbase, opacity);
        new_ico.addPixmap(QPixmap::fromImage(img));
    }
    return new_ico;
}

QImage ColorizeImage(const QString& filename, const QColor& colorbase, double opacity = 1)
{
    QImage img(filename);
    MakeSingleColorImage(img, colorbase, opacity);
    return img;
}

QIcon ColorizeIcon(const QString& filename, const QColor& colorbase, double opacity = 1)
{
    return QIcon(QPixmap::fromImage(ColorizeImage(filename, colorbase, opacity)));
}

}


PlatformStyle::PlatformStyle(const QString &_name, bool _imagesOnButtons, bool _colorizeIcons, bool _useExtraSpacing):
    name(_name),
    imagesOnButtons(_imagesOnButtons),
    colorizeIcons(_colorizeIcons),
    useExtraSpacing(_useExtraSpacing),
    singleColor(0,0,0),
    textColor(0,0,0),
    menuColor(0,0,0)
{
    // Determine icon highlighting color
    if (colorizeIcons) {
        singleColor = 0x008ac8;
    }
    // Determine text color
    textColor = 0xe6f0f0;
    menuColor = QColor(QApplication::palette().color(QPalette::WindowText));
}

QImage PlatformStyle::SingleColorImage(const QString& filename) const
{
    if (!colorizeIcons)
        return QImage(filename);
    return ColorizeImage(filename, SingleColor(), 0.8);
}

QIcon PlatformStyle::SingleColorIcon(const QString& filename) const
{
    if (!colorizeIcons)
        return QIcon(filename);
    return ColorizeIcon(filename, SingleColor());
}

QIcon PlatformStyle::SingleColorIcon(const QIcon& icon) const
{
    if (!colorizeIcons)
        return icon;
    return ColorizeIcon(icon, SingleColor());
}

QIcon PlatformStyle::TextColorIcon(const QString& filename) const
{
    return ColorizeIcon(filename, TextColor(), 0.6);
}

QIcon PlatformStyle::TextColorIcon(const QIcon& icon) const
{
    return ColorizeIcon(icon, TextColor(), 0.6);
}

QIcon PlatformStyle::MenuColorIcon(const QString &filename) const
{
    return ColorizeIcon(filename, MenuColor(), 0.8);
}

QIcon PlatformStyle::MenuColorIcon(const QIcon &icon) const
{
    return ColorizeIcon(icon, MenuColor(), 0.8);
}

QIcon PlatformStyle::MultiStatesIcon(const QString &resourcename, StateType type, QColor color, QColor colorAlt) const
{
    QIcon icon;
    switch (type) {
    case NavBar:
    {
        QImage img1(resourcename);
        QImage img2(img1);
        QImage img3(img1);
        QPixmap pix1 = MakeSingleColorPixmap(img1, color, 1);
        QPixmap pix2 = MakeSingleColorPixmap(img2, color, 0.8);
        QPixmap pix3 = MakeSingleColorPixmap(img3, colorAlt, 0.8);
        icon.addPixmap(pix1, QIcon::Normal, QIcon::On);
        icon.addPixmap(pix2, QIcon::Normal, QIcon::Off);
        icon.addPixmap(pix3, QIcon::Selected, QIcon::On);
        break;
    }
    case PushButton:
    {
        QImage img1(resourcename);
        QImage img2(img1);
        QPixmap pix1 = MakeSingleColorPixmap(img1, color, 1);
        QPixmap pix2 = MakeSingleColorPixmap(img2, color, 0.2);
        icon.addPixmap(pix1, QIcon::Normal, QIcon::Off);
        icon.addPixmap(pix2, QIcon::Disabled, QIcon::On);
        icon.addPixmap(pix2, QIcon::Disabled, QIcon::Off);
        break;
    }
    default:
        break;
    }
    return icon;
}

QIcon PlatformStyle::TableColorIcon(const QString &resourcename, TableColorType type) const
{
    // Initialize variables
    QIcon icon;
    QImage img1(resourcename);
    QImage img2(img1);
    double opacity = 1;
    double opacitySelected = 0.8;
    int color = 0xffffff;
    int colorSelected = 0xffffff;

    // Choose color
    TableColor(type, color, opacity);

    // Create pixmaps
    QPixmap pix1 = MakeSingleColorPixmap(img1, color, opacity);
    QPixmap pix2 = MakeSingleColorPixmap(img2, colorSelected, opacitySelected);

    // Create icon
    icon.addPixmap(pix1, QIcon::Normal, QIcon::On);
    icon.addPixmap(pix1, QIcon::Normal, QIcon::Off);
    icon.addPixmap(pix2, QIcon::Selected, QIcon::On);
    icon.addPixmap(pix2, QIcon::Selected, QIcon::Off);

    return icon;
}

QImage PlatformStyle::TableColorImage(const QString &resourcename, PlatformStyle::TableColorType type) const
{
    // Initialize variables
    QImage img(resourcename);
    double opacity = 1;
    int color = 0xffffff;

    // Choose color
    TableColor(type, color, opacity);

    // Create imtge
    MakeSingleColorImage(img, color, opacity);

    return img;
}

void PlatformStyle::TableColor(PlatformStyle::TableColorType type, int &color, double &opacity) const
{
    // Initialize variables
    opacity = 1;
    color = 0xffffff;

    // Choose color
    switch (type) {
    case Normal:
        opacity = 0.3;
        color = 0xffffff;
        break;
    case Input:
        opacity = 0.8;
        color = 0x2fa5df;
        break;
    case Inout:
        opacity = 0.8;
        color = 0x40bb00;
        break;
    case Output:
        opacity = 0.8;
        color = 0x40bb00;
        break;
    case Error:
        opacity = 0.8;
        color = 0xd02e49;
        break;
    default:
        break;
    }
}

const PlatformStyle *PlatformStyle::instantiate(const QString &platformId)
{
    for (unsigned x=0; x<platform_styles_count; ++x)
    {
        if (platformId == platform_styles[x].platformId)
        {
            return new PlatformStyle(
                    platform_styles[x].platformId,
                    platform_styles[x].imagesOnButtons,
                    platform_styles[x].colorizeIcons,
                    platform_styles[x].useExtraSpacing);
        }
    }
    return 0;
}

