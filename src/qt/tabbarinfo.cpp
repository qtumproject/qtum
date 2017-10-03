#include "tabbarinfo.h"

TabBarInfo::TabBarInfo(QStackedWidget *parent) :
    QObject(parent),
    m_current(0),
    m_stack(parent),
    m_tabBar(0),
    m_attached(false)
{}

bool TabBarInfo::addTab(int index, const QString &name)
{
    if(m_stack->count() <= index)
    {
        return false;
    }
    m_mapName[index] = name;
    m_mapVisible[index] = true;
    update();
    return true;
}

bool TabBarInfo::removeTab(int index)
{
    if(m_stack->count() < index)
    {
        return false;
    }
    m_mapName.remove(index);
    m_mapVisible.remove(index);
    update();
    return true;
}

void TabBarInfo::setTabVisible(int index, bool visible)
{
    if(m_stack->count() > index)
    {
        m_mapVisible[index] = visible;
    }
    update();
}

void TabBarInfo::attach(QTabBar *tabBar)
{
    m_tabBar = tabBar;
    if(m_tabBar)
    {
        connect(m_tabBar, SIGNAL(currentChanged(int)), SLOT(on_currentChanged(int)));
    }
    update();
    m_attached = true;
}

void TabBarInfo::detach()
{
    m_attached = false;
    if(m_tabBar)
    {
        disconnect(m_tabBar, 0, 0, 0);
        int count = m_tabBar->count();
        for(int i = count - 1; i >= 0; i--)
        {
            m_tabBar->removeTab(i);
        }
        m_tabBar = 0;
    }
}

void TabBarInfo::setCurrent(int index)
{
    m_current = index;
    update();
}

void TabBarInfo::on_currentChanged(int index)
{
    if(m_attached && index < m_mapTabInfo.keys().size())
    {
        int tab = m_mapTabInfo[index];
        if(tab < m_stack->count())
        {
            m_stack->setCurrentIndex(tab);
        }
        m_current = tab;
    }
}

void TabBarInfo::update()
{
    if(m_tabBar)
    {
        // Populate the tab bar
        QMap<int, int> mapTabInfo;
        int currentTab = 0;
        int numberTabs = m_mapName.keys().size();
        for(int i = 0; i < numberTabs; i++)
        {
            bool visible = m_mapVisible[i];
            if(visible)
            {
                if(m_tabBar->count() > currentTab)
                {
                    m_tabBar->setTabText(currentTab, m_mapName[i]);
                }
                else
                {
                    m_tabBar->addTab(m_mapName[i]);
                }
                mapTabInfo[currentTab] = i;
                currentTab++;
            }
        }
        int count = m_tabBar->count();
        if(currentTab < count)
        {
            for(int i = count - 1; i >= currentTab; i--)
            {
                m_tabBar->removeTab(i);
            }
        }
        m_mapTabInfo = mapTabInfo;

        // Set the current tab
        int tabCurrent = m_mapTabInfo[m_tabBar->currentIndex()];
        if(tabCurrent != m_current)
        {
            for(int i = 0; i < m_tabBar->count(); i++)
            {
                tabCurrent = m_mapTabInfo[i];
                if(tabCurrent == m_current)
                {
                    m_tabBar->setCurrentIndex(tabCurrent);
                }
            }
        }
    }
}
