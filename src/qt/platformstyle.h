// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PLATFORMSTYLE_H
#define BITCOIN_QT_PLATFORMSTYLE_H

#include <QIcon>
#include <QPixmap>
#include <QString>

/* Coin network-specific GUI style information */
class PlatformStyle
{
public:
    /** Get style associated with provided platform name, or 0 if not known */
    static const PlatformStyle *instantiate(const QString &platformId);

    const QString &getName() const { return name; }

    bool getImagesOnButtons() const { return imagesOnButtons; }
    bool getUseExtraSpacing() const { return useExtraSpacing; }

    QColor TextColor() const { return textColor; }
    QColor SingleColor() const { return singleColor; }
    QColor MenuColor() const { return menuColor; }

    /** Colorize an image (given filename) with the icon color */
    QImage SingleColorImage(const QString& filename) const;

    /** Colorize an icon (given filename) with the icon color */
    QIcon SingleColorIcon(const QString& filename) const;

    /** Colorize an icon (given object) with the icon color */
    QIcon SingleColorIcon(const QIcon& icon) const;

    /** Colorize an icon (given filename) with the text color */
    QIcon TextColorIcon(const QString& filename) const;

    /** Colorize an icon (given object) with the text color */
    QIcon TextColorIcon(const QIcon& icon) const;

    /** Colorize an icon (given filename) with the menu text color */
    QIcon MenuColorIcon(const QString& filename) const;

    /** Colorize an icon (given object) with the menu text color */
    QIcon MenuColorIcon(const QIcon& icon) const;

    enum StateType{
        NavBar = 0,
        PushButton = 1
    };
    /** Get multi-states icon*/
    QIcon MultiStatesIcon(const QString& resourcename, StateType type = NavBar, QColor color = Qt::white, QColor colorAlt = 0x2d2d2d) const;

    enum TableColorType{
        Normal = 0,
        Input,
        Inout,
        Output,
        Error
    };
    QIcon TableColorIcon(const QString& resourcename, TableColorType type) const;
    QImage TableColorImage(const QString& resourcename, TableColorType type) const;
    void TableColor(TableColorType type, int& color, double& opacity) const;

private:
    PlatformStyle(const QString &name, bool imagesOnButtons, bool colorizeIcons, bool useExtraSpacing);

    QString name;
    bool imagesOnButtons;
    bool colorizeIcons;
    bool useExtraSpacing;
    QColor singleColor;
    QColor textColor;
    QColor menuColor;
    /* ... more to come later */
};

#endif // BITCOIN_QT_PLATFORMSTYLE_H

