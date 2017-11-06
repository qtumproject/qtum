#include "styleSheet.h"

#include <QFile>
#include <QWidget>
#include <QApplication>
#include <QStyleFactory>

static const QString STYLE_FORMAT = ":/styles/%1";

StyleSheet &StyleSheet::instance()
{
    static StyleSheet inst;
    return inst;
}

StyleSheet::StyleSheet()
{}

void StyleSheet::setStyleSheet(QWidget *widget, const QString &style_name)
{
    setObjectStyleSheet<QWidget>(widget, style_name);
}

void StyleSheet::setStyleSheet(QApplication *app, const QString& style_name)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    //Enable scaling for high resolution, the attribute is present from Qt 5.6 on
    app->setAttribute(Qt::AA_EnableHighDpiScaling);  
#endif

    QStyle* mainStyle = QStyleFactory::create("Fusion");
	if(mainStyle) app->setStyle(mainStyle);

    setObjectStyleSheet<QApplication>(app, style_name);
}

QString StyleSheet::getStyleSheet(const QString &style_name)
{
    QString style;
    QFile file(STYLE_FORMAT.arg(style_name));
    if(file.open(QIODevice::ReadOnly))
    {
        style = file.readAll();
        m_cacheStyles[style_name] = style;
    }
    return style;
}

template<typename T>
void StyleSheet::setObjectStyleSheet(T *object, const QString &style_name)
{
    QString style_value = m_cacheStyles.contains(style_name) ? m_cacheStyles[style_name] : getStyleSheet(style_name);
    object->setStyleSheet(style_value);
}
