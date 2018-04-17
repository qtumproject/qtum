#ifndef NAVIGATIONBAR_H
#define NAVIGATIONBAR_H

#include <QWidget>
#include <QAction>
#include <QList>
#include <QMap>
#include <QSize>

/**
 * @brief The NavigationBar class Custom control for navigation bar
 */
class NavigationBar : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief NavigationBar Constructor
     * @param parent Parent widget
     */
    explicit NavigationBar(QWidget *parent = 0);

    /**
     * @brief addAction Add action to the navigation bar
     * @param action Action to add
     */
    void addAction(QAction *action);

    /**
     * @brief addGroup Add group of actions
     * @param list List of actions
     * @param icon Icon for the group of actions
     * @param text Text for the group of actions
     * @return New action associated with the group
     */
    QAction *addGroup(QList<QAction*> list, const QIcon &icon, const QString &text);

    /**
     * @brief addGroup Add group of actions
     * @param list List of actions
     * @param text Text for the group of actions
     * @return New action associated with the group
     */
    QAction *addGroup(QList<QAction*> list, const QString &text);

    /**
     * @brief mapGroup Map the action with group
     * @param action Action result of group creation
     * @param list List of group actions
     */
    void mapGroup(QAction* action, QList<QAction*> list);

    /**
     * @brief buildUi Construct the layout of the composite GUI control
     */
    void buildUi();

Q_SIGNALS:
    /**
     * @brief resized Signal that the size is changed
     */
    void resized(const QSize&);

public Q_SLOTS:
    /**
     * @brief onSubBarClick Activate the sub-navigation bar
     */
    void onSubBarClick(bool);

    /**
     * @brief setToolButtonStyle Set the style for the tool buttons
     * @param toolButtonStyle Tool button style
     */
    void setToolButtonStyle(Qt::ToolButtonStyle toolButtonStyle);

protected:
    void resizeEvent(QResizeEvent * evt);

private:

    /**
     * @brief setSubBar Set the component be sub-navigation bar
     * @param subBar Is the component sub-navigation bar
     */
    void setSubBar(bool subBar);

    QList<QAction*> m_actions;
    QMap<QAction*, QList<QAction*>> m_groups;
    Qt::ToolButtonStyle m_toolStyle;
    bool m_subBar;
    bool m_built;
};

#endif // NAVIGATIONBAR_H

