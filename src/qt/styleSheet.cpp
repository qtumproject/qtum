#include <qt/styleSheet.h>

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
#include <QSettings>

static const QString STYLE_FORMAT = ":/styles/%1/%2";
static const QString STYLE_CONFIG_FORMAT = ":/styles/%1/config";
static const QColor LINK_COLOR = "#2d9ad0";

class QtumStyle : public QProxyStyle
{
public:
    QtumStyle()
    {
        message_info_path = GetStringStyleValue("appstyle/message-info-icon", ":/styles/theme1/app-icons/message_info");
        message_warning_path = GetStringStyleValue("appstyle/message-warning-icon", ":/styles/theme1/app-icons/message_warning");
        message_critical_path = GetStringStyleValue("appstyle/message-critical-icon", ":/styles/theme1/app-icons/message_critical");
        message_question_path = GetStringStyleValue("appstyle/message-question-icon", ":/styles/theme1/app-icons/message_question");
        message_icon_weight = GetIntStyleValue("appstyle/message-icon-weight", 45);
        message_icon_height = GetIntStyleValue("appstyle/message-icon-height", 49);
        button_text_upper = GetIntStyleValue("appstyle/button_text_upper", true);
    }

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
                iconPixmap = QPixmap(message_info_path);
                break;
            case QMessageBox::Warning:
                iconPixmap = QPixmap(message_warning_path);
                break;
            case QMessageBox::Critical:
                iconPixmap = QPixmap(message_critical_path);
                break;
            case QMessageBox::Question:
                iconPixmap = QPixmap(message_question_path);
                break;
            default:
                QProxyStyle::polish(widget);
                return;
            }
            messageBox->setIconPixmap(iconPixmap.scaled(message_icon_weight, message_icon_height));
        }
        if(widget && widget->inherits("QPushButton") && button_text_upper)
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

private:
    QString message_info_path;
    QString message_warning_path;
    QString message_critical_path;
    QString message_question_path;
    int message_icon_weight;
    int message_icon_height;
    bool button_text_upper;
};

StyleSheet &StyleSheet::instance()
{
    static StyleSheet inst;
    return inst;
}

StyleSheet::StyleSheet()
{
    QSettings settings;
    m_theme = settings.value("Theme", getDefaultTheme()).toString();
    QStringList supportedThemes = getSupportedThemes();
    if(!supportedThemes.contains(m_theme))
        m_theme = getDefaultTheme();
    m_config = new QSettings(STYLE_CONFIG_FORMAT.arg(m_theme), QSettings::IniFormat);
}

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
    mainPalette.setColor(QPalette::Link, GetStyleValue("appstyle/link-color", LINK_COLOR).toString());
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
    QFile file(STYLE_FORMAT.arg(m_theme, style_name));
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

QString StyleSheet::getCurrentTheme()
{
    return m_theme;
}

QStringList StyleSheet::getSupportedThemes()
{
    return QStringList() << "theme3" << "theme2" << "theme1";
}

QStringList StyleSheet::getSupportedThemesNames()
{
    return QStringList() << "Light blue theme" << "Dark blue theme" << "Dark theme";
}


QString StyleSheet::getDefaultTheme()
{
    return "theme3";
}

bool StyleSheet::setTheme(const QString &theme)
{
    if(getSupportedThemes().contains(theme))
    {
        QSettings settings;
        settings.setValue("Theme", theme);
        return true;
    }
    return false;
}

QVariant StyleSheet::getStyleValue(const QString &key, const QVariant &defaultValue)
{
    if(m_config)
    {
        return m_config->value(key, defaultValue);
    }

    return defaultValue;
}
