#ifndef STYLESHEET_H
#define STYLESHEET_H

#include <QMap>
#include <QString>
#include <QStringList>
#include <QSettings>
#include <QPointer>

class QWidget;
class QApplication;

#define SetObjectStyleSheet(object, name) StyleSheet::instance().setStyleSheet(object, name)

#define GetStyleValue(key, defaultValue) StyleSheet::instance().getStyleValue(key, defaultValue)
#define GetStringStyleValue(key, defaultValue) GetStyleValue(key, defaultValue).toString()
#define GetIntStyleValue(key, defaultValue) GetStyleValue(key, defaultValue).toInt()
#define GetPercentStyleValue(key, defaultValue) GetIntStyleValue(key, defaultValue) / 100.0
#define GetColorStyleValue(key, defaultValue) GetStyleValue(key, defaultValue.name()).toString()

/** Names of the styles that will be used for the GUI components appearance
 */
namespace StyleSheetNames 
{
    static const QString App                         = "app";
    static const QString Invalid                     = "invalid";
    static const QString TableViewLight              = "tableviewlight";
    static const QString ButtonDark                  = "buttondark";
    static const QString ButtonLight                 = "buttonlight";
    static const QString ButtonGray                  = "buttongray";
    static const QString ButtonTransparent           = "buttontransparent";
    static const QString ButtonTransparentBordered   = "buttontransparentbordered";
    static const QString NavButton                   = "navbutton";
    static const QString NavGroupButton              = "navgroupbutton";
    static const QString NavSubGroupButton           = "navsubgroupbutton";
    static const QString TreeView                    = "treeview";
    static const QString ScrollBarLight              = "scrollbarlight";
    static const QString ScrollBarDark               = "scrollbardark";
}

/** Singleton class that manage the styles
 */
class StyleSheet
{
public:
    static StyleSheet& instance();
    void setStyleSheet(QWidget* widget, const QString& style_name);
    void setStyleSheet(QApplication* app, const QString& style_name);
    QVariant getStyleValue(const QString& key, const QVariant &defaultValue);

    QString getCurrentTheme();
    static QStringList getSupportedThemes();
    static QString getDefaultTheme();
    static QStringList getSupportedThemesNames();
    static bool setTheme(const QString& theme);

private:
    QString getStyleSheet(const QString& style_name);

    template<typename T>
    void setObjectStyleSheet(T* object, const QString& style_name);

    explicit StyleSheet();
    QMap<QString, QString> m_cacheStyles;
    QString m_theme;
    QPointer<QSettings> m_config;
};
#endif // STYLESHEET_H
