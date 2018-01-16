#include "styleSheet.h"

#include <QFile>
#include <QWidget>
#include <QApplication>
#include <QStyleFactory>
#include <QProxyStyle>
#include <QListView>
#include <QComboBox>
#include <QMessageBox>
#include <QPushButton>
#include <QPainter>
#include <QLineEdit>
#include <QtGlobal>

static const QString STYLE_FORMAT = ":/styles/%1";
static const QColor LINK_COLOR = "#2d9ad0";

class QtumStyle : public QProxyStyle
{
public:

    void polish(QWidget *widget)
    {
        if(widget && widget->inherits("QComboBox"))
        {
            QComboBox* comboBox = (QComboBox*)widget;
            if(comboBox->view() && comboBox->view()->inherits("QComboBoxListView"))
            {
                comboBox->setView(new QListView());
                qApp->processEvents();
            }

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
                iconPixmap = QPixmap(":/styles/app-icons/message_info");
                break;
            case QMessageBox::Warning:
                iconPixmap = QPixmap(":/styles/app-icons/message_warning");
                break;
            case QMessageBox::Critical:
                iconPixmap = QPixmap(":/styles/app-icons/message_critical");
                break;
            case QMessageBox::Question:
                iconPixmap = QPixmap(":/styles/app-icons/message_question");
                break;
            default:
                QProxyStyle::polish(widget);
                return;
            }
            messageBox->setIconPixmap(iconPixmap.scaled(45,49));
        }
        if(widget && widget->inherits("QPushButton"))
        {
            QPushButton* button = (QPushButton*)widget;
            button->setText(button->text().toUpper());
        }
        if(widget && widget->inherits("QLineEdit"))
        {
            QLineEdit* lineEdit = (QLineEdit*)widget;
            if(lineEdit->isReadOnly())
            {
                lineEdit->setFocusPolicy(Qt::ClickFocus);
            }
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

    QPalette mainPalette(app->palette());
    mainPalette.setColor(QPalette::Link, LINK_COLOR);
    app->setPalette(mainPalette);

    // Increase the font size slightly for Windows and MAC
    QFont font = app->font();
    qreal fontSize = font.pointSizeF();
    qreal multiplier = 1;
#if defined(Q_OS_WIN) ||  defined(Q_OS_MAC)
    multiplier = 1.1;
#endif
    font.setPointSizeF(fontSize * multiplier);
    app->setFont(font);

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
