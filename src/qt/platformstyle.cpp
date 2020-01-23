// Copyright (c) 2015-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/platformstyle.h>

#include <qt/guiconstants.h>
#include "styleSheet.h"

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
    version(1),
    imagesOnButtons(_imagesOnButtons),
    colorizeIcons(_colorizeIcons),
    useExtraSpacing(_useExtraSpacing),
    singleColor(0,0,0),
    textColor(0,0,0),
    menuColor(0,0,0)
{
    // Get version
    version = GetIntStyleValue("platformstyle/version", version);

    // Determine icon highlighting color
    if (colorizeIcons) {
        singleColor = GetStringStyleValue("platformstyle/single-color", "#008ac8");
    }
    // Determine text color
    textColor = GetStringStyleValue("platformstyle/text-color", "#e6f0f0");
    menuColor = GetColorStyleValue("platformstyle/menu-color", QColor(QApplication::palette().color(QPalette::WindowText)));

    // Determine table color
    tableColorNormal = GetStringStyleValue("platformstyle/table-color-normal", "#ffffff");
    tableColorInput = GetStringStyleValue("platformstyle/table-color-input", "#2fa5df");
    tableColorInout = GetStringStyleValue("platformstyle/table-color-inout", "#40bb00");
    tableColorOutput = GetStringStyleValue("platformstyle/table-color-output", "#40bb00");
    tableColorError = GetStringStyleValue("platformstyle/table-color-error", "#d02e49");

    // Determine multi states icon colors
    multiStatesIconColor1 = GetStringStyleValue("platformstyle/multi-states-icon-color1", "#ffffff");
    multiStatesIconColor2 = GetStringStyleValue("platformstyle/multi-states-icon-color2", "#2d2d2d");
    multiStatesIconColor3 = GetStringStyleValue("platformstyle/multi-states-icon-color3", "#5a5a5d");
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

QIcon PlatformStyle::TextColorIcon(const QIcon& icon) const
{
    return ColorizeIcon(icon, TextColor(), 0.6);
}

QIcon PlatformStyle::MenuColorIcon(const QString &filename) const
{
    if(version == 2 || version == 3)
        return MenuColorIconV2(filename);

    return MenuColorIconV1(filename);
}

QIcon PlatformStyle::MultiStatesIcon(const QString &resourcename, StateType type) const
{
    if(version == 2)
        return MultiStatesIconV2(resourcename, type);

    if(version == 3)
        return MultiStatesIconV3(resourcename, type);

    return MultiStatesIconV1(resourcename, type);
}

QIcon PlatformStyle::TableColorIcon(const QString &resourcename, TableColorType type) const
{
    // Initialize variables
    QIcon icon;
    QImage img1(resourcename);
    QImage img2(img1);
    double opacity = 1;
    double opacitySelected = 0.8;
    QColor color = 0xffffff;
    QColor colorSelected = 0xffffff;

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
    QColor color = 0xffffff;

    // Choose color
    TableColor(type, color, opacity);

    // Create imtge
    MakeSingleColorImage(img, color, opacity);

    return img;
}

void PlatformStyle::TableColor(PlatformStyle::TableColorType type, QColor &color, double &opacity) const
{
    // Initialize variables
    color = 0xffffff;

    // Choose color
    switch (type) {
    case Normal:
        opacity = 0.4;
        color = tableColorNormal;
        break;
    case Input:
        opacity = 0.8;
        color = tableColorInput;
        break;
    case Inout:
        opacity = 0.8;
        color = tableColorInout;
        break;
    case Output:
        opacity = 0.8;
        color = tableColorOutput;
        break;
    case Error:
        opacity = 0.8;
        color = tableColorError;
        break;
    default:
        opacity = 1;
        break;
    }

    if(version > 1)
        opacity = 1;
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
    return nullptr;
}


QIcon PlatformStyle::MultiStatesIconV1(const QString &resourcename, PlatformStyle::StateType type) const
{
    QColor color = multiStatesIconColor1;
    QColor colorAlt = multiStatesIconColor2;
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
    case PushButtonLight:
    {
        color = multiStatesIconColor3;
    }
    case PushButtonIcon:
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

QIcon PlatformStyle::MultiStatesIconV2(const QString &resourcename, PlatformStyle::StateType type) const
{
    QColor color = multiStatesIconColor1;
    QColor colorAlt = multiStatesIconColor2;
    QIcon icon;
    switch (type) {
    case NavBar:
    {
        QImage img1(resourcename);
        QImage img2(img1);
        QPixmap pix1 = MakeSingleColorPixmap(img1, color, 1);
        QPixmap pix2 = MakeSingleColorPixmap(img2, colorAlt, 1);
        icon.addPixmap(pix1, QIcon::Normal, QIcon::On);
        icon.addPixmap(pix2, QIcon::Normal, QIcon::Off);
        icon.addPixmap(pix1, QIcon::Selected, QIcon::Off);
        icon.addPixmap(pix2, QIcon::Selected, QIcon::On);
        break;
    }
    case PushButtonIcon:
    {
        QImage img1(resourcename);
        QImage img2(img1);
        QImage img3(img1);
        QPixmap pix1 = MakeSingleColorPixmap(img1, color, 1);
        QPixmap pix2 = MakeSingleColorPixmap(img2, colorAlt, 1);
        QPixmap pix3 = MakeSingleColorPixmap(img3, colorAlt, 0.2);
        icon.addPixmap(pix1, QIcon::Normal, QIcon::On);
        icon.addPixmap(pix2, QIcon::Normal, QIcon::Off);
        icon.addPixmap(pix1, QIcon::Selected, QIcon::Off);
        icon.addPixmap(pix1, QIcon::Selected, QIcon::On);
        icon.addPixmap(pix3, QIcon::Disabled, QIcon::On);
        icon.addPixmap(pix3, QIcon::Disabled, QIcon::Off);
        break;
    }
    case PushButtonLight:
    case PushButton:
    {
        colorAlt = multiStatesIconColor3;
        QImage img1(resourcename);
        QImage img2(img1);
        QImage img3(img1);
        QPixmap pix1 = MakeSingleColorPixmap(img1, color, 1);
        QPixmap pix2 = MakeSingleColorPixmap(img2, colorAlt, 1);
        QPixmap pix3 = MakeSingleColorPixmap(img3, color, 0.2);
        icon.addPixmap(pix2, QIcon::Normal, QIcon::On);
        icon.addPixmap(pix1, QIcon::Normal, QIcon::Off);
        icon.addPixmap(pix2, QIcon::Selected, QIcon::Off);
        icon.addPixmap(pix2, QIcon::Selected, QIcon::On);
        icon.addPixmap(pix3, QIcon::Disabled, QIcon::On);
        icon.addPixmap(pix3, QIcon::Disabled, QIcon::Off);
        break;
    }
    default:
        break;
    }
    return icon;
}

void PlatformStyle::SingleColorImage(QImage &img, const QColor &colorbase, double opacity)
{
    MakeSingleColorImage(img, colorbase, opacity);
}

QIcon PlatformStyle::SingleColorIcon(const QString &resourcename, const QColor &colorbase, double opacity)
{
    QImage img(resourcename);
    QPixmap pix = MakeSingleColorPixmap(img, colorbase, opacity);
    QIcon icon;
    icon.addPixmap(pix, QIcon::Normal, QIcon::On);
    icon.addPixmap(pix, QIcon::Normal, QIcon::Off);
    icon.addPixmap(pix, QIcon::Disabled, QIcon::On);
    icon.addPixmap(pix, QIcon::Disabled, QIcon::Off);

    return icon;
}

QIcon PlatformStyle::SingleColorIcon(const QIcon &icon, const QColor &colorbase, double opacity)
{
    return ColorizeIcon(icon, colorbase, opacity);
}

QIcon PlatformStyle::MultiStatesIconV3(const QString &resourcename, PlatformStyle::StateType type) const
{
    if(type == NavBar)
    {
        QColor color = multiStatesIconColor3;
        QIcon icon;
        QImage img1(resourcename);
        QImage img2(img1);
        QPixmap pix1 = MakeSingleColorPixmap(img1, color, 1);
        QPixmap pix2 = MakeSingleColorPixmap(img2, color, 1);
        icon.addPixmap(pix1, QIcon::Normal, QIcon::On);
        icon.addPixmap(pix2, QIcon::Normal, QIcon::Off);
        icon.addPixmap(pix1, QIcon::Selected, QIcon::Off);
        icon.addPixmap(pix2, QIcon::Selected, QIcon::On);

        return icon;
    }

    return MultiStatesIconV2(resourcename, type);
}

QIcon PlatformStyle::MenuColorIconV1(const QString &filename) const
{
    return ColorizeIcon(filename, MenuColor(), 0.8);
}

QIcon PlatformStyle::MenuColorIconV2(const QString &filename) const
{
    QIcon icon;
    QImage img1(filename);
    QImage img2(img1);
    QColor color = MenuColor();
    QPixmap pix1 = MakeSingleColorPixmap(img1, color, 1);
    QPixmap pix2 = MakeSingleColorPixmap(img2, color, 0.3);
    icon.addPixmap(pix1, QIcon::Normal, QIcon::On);
    icon.addPixmap(pix1, QIcon::Normal, QIcon::Off);
    icon.addPixmap(pix2, QIcon::Disabled, QIcon::Off);
    icon.addPixmap(pix2, QIcon::Disabled, QIcon::On);
    return icon;
}
