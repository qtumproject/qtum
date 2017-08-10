#include "navigationbar.h"
#include <QActionGroup>
#include <QToolButton>
#include <QLayout>

#define BUTTON_WIDTH 128
#define BUTTON_HEIGHT 64

NavigationBar::NavigationBar(QWidget *parent) :
    QWidget(parent),
    m_toolStyle(Qt::ToolButtonTextUnderIcon),
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
        int defButtonWidth = m_subBar ? BUTTON_WIDTH / 2 : BUTTON_WIDTH;
        int defButtonHeight = m_subBar ? BUTTON_HEIGHT / 2 : BUTTON_HEIGHT;
        vboxLayout->setContentsMargins(m_subBar ? defButtonWidth : 6, 0, 0, 0);
        vboxLayout->setSpacing(3);

        // List all actions
        for(int i = 0; i < m_actions.count(); i++)
        {
            // Add an action to the layout
            QAction* action = m_actions[i];
            action->setActionGroup(actionGroup);
            action->setCheckable(true);
            QToolButton* toolButton = new QToolButton(this);
            toolButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            toolButton->setMinimumHeight(defButtonHeight);
            toolButton->setToolButtonStyle(m_toolStyle);
            toolButton->setDefaultAction(action);
            toolButton->setIconSize(QSize(32, 32));
            vboxLayout->addWidget(toolButton);

            if(m_groups.contains(action))
            {
                // Add sub-navigation bar for the group of actions
                QList<QAction*> group = m_groups[action];
                NavigationBar* subNavBar = new NavigationBar(this);
                subNavBar->setSubBar(true);
                for(int j = 0; j < group.count(); j++)
                {
                    subNavBar->addAction(group[j]);
                }
                vboxLayout->addWidget(subNavBar);
                subNavBar->buildUi();
                connect(action, SIGNAL(toggled(bool)), subNavBar, SLOT(onSubBarClick(bool)));
            }
        }

        if(!m_subBar)
        {
            // Set specific parameters for for the main component
            if(m_actions.count())
            {
                m_actions[0]->setChecked(true);
            }
            setMinimumWidth(defButtonWidth + 6);
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

