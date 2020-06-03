#include <qt/navigationbar.h>
#include <QActionGroup>
#include <QToolButton>
#include <QLayout>
#include <QStylePainter>
#include <QStyleOptionToolButton>
#include <QStyle>
#include <QLabel>
#include <qt/styleSheet.h>
#include <qt/platformstyle.h>

namespace NavigationBar_NS
{
static const int ToolButtonWidth = 190;
static const int ToolButtonHeight = 54;
static const int ToolButtonIconSize = 28;
static const int MarginLeft = 0;
static const int MarginRight = 0;
static const int MarginTop = 0;
static const int MarginBottom = 8;
static const int ButtonSpacing = 2;
static const int SubNavPaddingRight = 40;
static const int LogoHeight = 60;
static const int LogoWidth = 90;
}
using namespace NavigationBar_NS;

class NavToolButton : public QToolButton
{
public:
    explicit NavToolButton(QWidget * parent, bool subBar):
        QToolButton(parent),
        m_subBar(subBar),
        m_iconCached(false)
    {
        m_colorEnabled = GetStringStyleValue("navtoolbutton/color-enabled", "#1a96ce");
        m_colorPressed = GetStringStyleValue("navtoolbutton/color-pressed", "#e5f3f9");
        m_colorHover = GetStringStyleValue("navtoolbutton/color-hover", "#b3dcef");
        m_colorDisabled = GetStringStyleValue("navtoolbutton/color-disabled", "#7fc4e3");
        m_subIcon = QImage(GetStringStyleValue("navtoolbutton/sub-icon", ""));
        m_subPaddingRight = GetIntStyleValue("navtoolbutton/sub-padding-right", SubNavPaddingRight);
        m_subPaddingLeft = GetIntStyleValue("navtoolbutton/sub-padding-left", 0);
        m_subAlignment = GetIntStyleValue("navtoolbutton/sub-alignment", Qt::AlignRight);
        m_subIconHeight = GetIntStyleValue("navtoolbutton/sub-icon-height", 6);
        m_subIconWidth = GetIntStyleValue("navtoolbutton/sub-icon-width", 3);
    }

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE
    {
        QStylePainter sp( this );
        QStyleOptionToolButton opt;
        initStyleOption( &opt );

        if(m_subBar)
        {
            const QString strText = opt.text;

            //draw background
            opt.text.clear();
            opt.icon = QIcon();
            sp.drawComplexControl( QStyle::CC_ToolButton, opt );
            opt.text = strText;

            //draw label
            drawLabel(&opt, &sp);
        }
        else
        {
            //update icon
            updateIcon(opt);

            //draw control
            sp.drawComplexControl( QStyle::CC_ToolButton, opt );
        }
    }

    void drawLabel(const QStyleOption *opt, QPainter *p)
    {
        if (const QStyleOptionToolButton *toolbutton
                = qstyleoption_cast<const QStyleOptionToolButton *>(opt)) {

            // Choose color
            QColor color;
            if(!(toolbutton->state & QStyle::State_Enabled))
            {
                color = m_colorEnabled;
            }
            else if(toolbutton->state & (QStyle::State_Sunken | QStyle::State_On))
            {
                color = m_colorPressed;
            }
            else if(toolbutton->state & QStyle::State_MouseOver)
            {
                color = m_colorHover;
            }
            else
            {
                color = m_colorDisabled;
            }

            // Determine area
            QRect rect = toolbutton->rect;
            int w = rect.width() - m_subPaddingRight;
            w = qMax(w, 0);
            rect.setWidth(w);
            rect.setLeft(m_subPaddingLeft);
            int shiftX = 0;
            int shiftY = 0;
            if (toolbutton->state & (QStyle::State_Sunken | QStyle::State_On)) {
                shiftX = style()->pixelMetric(QStyle::PM_ButtonShiftHorizontal, toolbutton, this);
                shiftY = style()->pixelMetric(QStyle::PM_ButtonShiftVertical, toolbutton, this);
            }

            // Draw icon
            if(!m_subIcon.isNull())
            {
                QImage image = m_subIcon;
                PlatformStyle::SingleColorImage(image, color);
                QRect rectImage(rect.left() -m_subIconHeight -2, rect.top() + (rect.height() - m_subIconHeight)/2, m_subIconWidth, m_subIconHeight);
                p->drawImage(rectImage, image);
            }

            // Draw text
            if (!toolbutton->text.isEmpty()) {
                int alignment = m_subAlignment | Qt::AlignVCenter | Qt::TextShowMnemonic;
                rect.translate(shiftX, shiftY);
                p->setFont(toolbutton->font);
                p->setPen(color);
                p->drawText(rect, alignment, toolbutton->text);
            }
        }
    }

    void updateIcon(QStyleOptionToolButton &toolbutton)
    {
        // Update mouse over icon
        if((toolbutton.state & QStyle::State_Enabled) &&
                !(toolbutton.state & QStyle::State_On) &&
                (toolbutton.state & QStyle::State_MouseOver))
        {
            if(!m_iconCached)
            {
                QIcon icon = toolbutton.icon;
                QPixmap pixmap = icon.pixmap(toolbutton.iconSize, QIcon::Selected, QIcon::On);
                m_hoverIcon = QIcon(pixmap);
                m_iconCached = true;
            }
            toolbutton.icon = m_hoverIcon;
        }
    }

private:
    bool m_subBar;
    bool m_iconCached;
    QIcon m_hoverIcon;
    QColor m_colorEnabled;
    QColor m_colorPressed;
    QColor m_colorHover;
    QColor m_colorDisabled;
    QImage m_subIcon;
    int m_subPaddingRight;
    int m_subPaddingLeft;
    int m_subAlignment;
    int m_subIconHeight;
    int m_subIconWidth;
};

NavigationBar::NavigationBar(QWidget *parent) :
    QWidget(parent),
    m_toolStyle(Qt::ToolButtonTextBesideIcon),
    m_subBar(false),
    m_built(false),
    m_logoSpace(0)
{
    m_logoSpace = GetIntStyleValue("navigationbar/logo-space", 0);
}

void NavigationBar::addAction(QAction *action)
{
    // Add action to the list
    m_actions.append(action);
}

QAction *NavigationBar::addGroup(QList<QAction *> list, const QIcon &icon, const QString &text)
{
    // Add new group
    QAction* action = new QAction(icon, text, this);
    mapGroup(action, list);
    return action;
}

QAction *NavigationBar::addGroup(QList<QAction *> list, const QString &text)
{
    // Add new group
    QAction* action = new QAction(text, this);
    mapGroup(action, list);
    return action;
}

void NavigationBar::buildUi()
{
    // Build the layout of the complex GUI component
    if(!m_built)
    {
        // Set it visible if main component
        setVisible(!m_subBar);

        // Create new layout for the bar
        QActionGroup* actionGroup = new QActionGroup(this);
        actionGroup->setExclusive(true);
        QVBoxLayout* vboxLayout = new QVBoxLayout(this);
        int defButtonWidth = ToolButtonWidth;
        vboxLayout->setContentsMargins(m_subBar ? 0 : MarginLeft,
                                       m_subBar ? 0 : MarginTop,
                                       m_subBar ? 0 : MarginRight,
                                       m_subBar ? 0 : MarginBottom);
        vboxLayout->setSpacing(m_subBar ? 0 : ButtonSpacing);

        if(!m_subBar)
        {
            QHBoxLayout *hLayout = new QHBoxLayout();
            hLayout->setContentsMargins(0,0,0,10);
            QLabel *labelLogo = new QLabel(this);
            labelLogo->setFixedSize(LogoHeight, LogoWidth);
            labelLogo->setObjectName("labelLogo");
            hLayout->addWidget(labelLogo);
            vboxLayout->addLayout(hLayout);

            if(m_logoSpace)
            {
                QFrame *line = new QFrame(this);
                line->setObjectName("hLineLogo");
                line->setFrameShape(QFrame::HLine);
                vboxLayout->addWidget(line);
                vboxLayout->addSpacerItem(new QSpacerItem(m_logoSpace, m_logoSpace, QSizePolicy::Fixed, QSizePolicy::Fixed));
            }
        }
        // List all actions
        for(int i = 0; i < m_actions.count(); i++)
        {
            // Add an action to the layout
            QAction* action = m_actions[i];
            action->setActionGroup(actionGroup);
            action->setCheckable(true);
            QToolButton* toolButton = new NavToolButton(this, m_subBar);
            toolButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            toolButton->setToolButtonStyle(m_toolStyle);
            toolButton->setDefaultAction(action);
            toolButton->setIconSize(QSize(ToolButtonIconSize, ToolButtonIconSize));
            if(m_subBar)
            {
                SetObjectStyleSheet(toolButton, StyleSheetNames::NavSubGroupButton);
            }
            else
            {
                SetObjectStyleSheet(toolButton, StyleSheetNames::NavButton);
            }

            if(m_groups.contains(action))
            {
                // Add the tool button
                QVBoxLayout* vboxLayout2 = new QVBoxLayout();
                vboxLayout->addLayout(vboxLayout2);
                vboxLayout2->addWidget(toolButton);
                vboxLayout2->setSpacing(0);
                if(!m_subBar)
                {
                    SetObjectStyleSheet(toolButton, StyleSheetNames::NavGroupButton);
                }

                // Add sub-navigation bar for the group of actions
                QList<QAction*> group = m_groups[action];
                NavigationBar* subNavBar = new NavigationBar(this);
                subNavBar->setSubBar(true);
                for(int j = 0; j < group.count(); j++)
                {
                    subNavBar->addAction(group[j]);
                }
                vboxLayout2->addWidget(subNavBar);
                subNavBar->buildUi();
                connect(action, &QAction::toggled, subNavBar, &NavigationBar::onSubBarClick);
            }
            else
            {

                vboxLayout->addWidget(toolButton);
            }
        }

        if(!m_subBar)
        {
            // Set specific parameters for for the main component
            if(m_actions.count())
            {
                m_actions[0]->setChecked(true);
            }
            setMinimumWidth(defButtonWidth + MarginLeft + MarginRight);
            QFrame *lineStatus = new QFrame(this);
            lineStatus->setObjectName("hLineStatus");
            lineStatus->setFrameShape(QFrame::HLine);
            vboxLayout->addStretch(1);
            vboxLayout->addWidget(lineStatus);
        }

        // The component is built
        m_built = true;
    }
}

void NavigationBar::onSubBarClick(bool clicked)
{
    // Expand/collapse the sub-navigation bar
    setVisible(clicked);


    if(clicked && m_actions.count() > 0)
    {
        // Activate the checked action
        bool haveChecked = false;
        for(int i = 0; i < m_actions.count(); i++)
        {
            QAction* action = m_actions[i];
            if(action->isChecked())
            {
                action->trigger();
                haveChecked = true;
                break;
            }
        }
        if(!haveChecked)
        {
            m_actions[0]->trigger();
        }
    }
}

void NavigationBar::setToolButtonStyle(Qt::ToolButtonStyle toolButtonStyle)
{
    // Set the tool button style
    m_toolStyle = toolButtonStyle;
}

void NavigationBar::resizeEvent(QResizeEvent *evt)
{
    QWidget::resizeEvent(evt);
    Q_EMIT resized(size());
}

void NavigationBar::mapGroup(QAction *action, QList<QAction *> list)
{
    // Map the group with the actions
    addAction(action);
    m_groups[action] = list;
}

void NavigationBar::setSubBar(bool subBar)
{
    // Set the component be sub-navigation bar
    m_subBar = subBar;
}

