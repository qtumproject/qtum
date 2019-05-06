#include <qt/navigationbar.h>
#include <QActionGroup>
#include <QToolButton>
#include <QLayout>
#include <QStylePainter>
#include <QStyleOptionToolButton>
#include <QStyle>
#include <qt/styleSheet.h>

namespace NavigationBar_NS
{
static const int ToolButtonWidth = 220;
static const int ToolButtonHeight = 54;
static const int ToolButtonIconSize = 32;
static const int MarginLeft = 0;
static const int MarginRight = 0;
static const int MarginTop = 0;
static const int MarginBottom = 8;
static const int ButtonSpacing = 2;
static const int SubNavPaddingRight = 40;
}
using namespace NavigationBar_NS;

class NavToolButton : public QToolButton
{
public:
    explicit NavToolButton(QWidget * parent, bool subBar):
        QToolButton(parent),
        m_subBar(subBar),
        m_iconCached(false)
    {}

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
                color = 0x1a96ce;
            }
            else if(toolbutton->state & (QStyle::State_Sunken | QStyle::State_On))
            {
                color = 0xe5f3f9;
            }
            else if(toolbutton->state & QStyle::State_MouseOver)
            {
                color = 0xb3dcef;
            }
            else
            {
                color = 0x7fc4e3;
            }

            // Determine area
            QRect rect = toolbutton->rect;
            int w = rect.width() - SubNavPaddingRight;
            w = qMax(w, 0);
            rect.setWidth(w);
            int shiftX = 0;
            int shiftY = 0;
            if (toolbutton->state & (QStyle::State_Sunken | QStyle::State_On)) {
                shiftX = style()->pixelMetric(QStyle::PM_ButtonShiftHorizontal, toolbutton, this);
                shiftY = style()->pixelMetric(QStyle::PM_ButtonShiftVertical, toolbutton, this);
            }

            // Draw text
            if (!toolbutton->text.isEmpty()) {
                int alignment = Qt::AlignRight | Qt::AlignVCenter | Qt::TextShowMnemonic;
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
};

NavigationBar::NavigationBar(QWidget *parent) :
    QWidget(parent),
    m_toolStyle(Qt::ToolButtonTextBesideIcon),
    m_subBar(false),
    m_built(false)
{
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
                SetObjectStyleSheet(toolButton, StyleSheetNames::ToolSubBlack);
            }
            else
            {
                SetObjectStyleSheet(toolButton, StyleSheetNames::ToolBlack);
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
                    SetObjectStyleSheet(toolButton, StyleSheetNames::ToolGroupBlack);
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
                connect(action, SIGNAL(toggled(bool)), subNavBar, SLOT(onSubBarClick(bool)));
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
            vboxLayout->addStretch(1);
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

