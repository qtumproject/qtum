#ifndef QTUMPUSHBUTTON_H
#define QTUMPUSHBUTTON_H
#include <QPushButton>
#include <QStyleOptionButton>
#include <QIcon>

class QtumPushButton : public QPushButton
{
public:
    explicit QtumPushButton(QWidget * parent = Q_NULLPTR);
    explicit QtumPushButton(const QString &text, QWidget *parent = Q_NULLPTR);

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

private:
    void updateIcon(QStyleOptionButton &pushbutton);

private:
    bool m_iconCached;
    QIcon m_downIcon;
};

#endif // QTUMPUSHBUTTON_H
