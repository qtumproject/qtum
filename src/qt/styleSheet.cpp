#include "styleSheet.h"

#include <QFile>
#include <QWidget>
#include <QApplication>
#include <QStyleFactory>
#include <QProxyStyle>
#include <QListView>
#include <QComboBox>
#include <QMessageBox>

static const QString STYLE_FORMAT = ":/styles/%1";

class QtumStyle : public QProxyStyle
{
public:

    void polish(QWidget *widget)
    {
        if(widget && widget->inherits("QComboBox"))
        {
            QComboBox* comboBox = (QComboBox*)widget;
            //comboBox->setView(new QListView());
            //qApp->processEvents();

            if(comboBox->view() && comboBox->view()->parentWidget())
            {
                QWidget* parent = comboBox->view()->parentWidget();
                parent->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
                parent->setAttribute(Qt::WA_TranslucentBackground);
            }
        }
        if(widget && widget->inherits("QMessageBox"))
        {
            QMessageBox* messageBox = (QMessageBox*)widget;
            QPixmap iconPixmap;
            QMessageBox::Icon icon = messageBox->icon();
            switch (icon)
            {
            case QMessageBox::Information:
            case QMessageBox::Question:
                iconPixmap = QPixmap(":/styles/app-icons/message_info");
                break;
            case QMessageBox::Warning:
                iconPixmap = QPixmap(":/styles/app-icons/message_critical");
                break;
            case QMessageBox::Critical:
                iconPixmap = QPixmap(":/styles/app-icons/message_critical");
                break;
            default:
                break;
            }
            messageBox->setIconPixmap(iconPixmap.scaled(45,49));
        }

        QProxyStyle::polish(widget);
    }
};

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
    QStyle* mainStyle = QStyleFactory::create("fusion");
    QtumStyle* qtumStyle = new QtumStyle;
    qtumStyle->setBaseStyle(mainStyle);
    app->setStyle(qtumStyle);

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
