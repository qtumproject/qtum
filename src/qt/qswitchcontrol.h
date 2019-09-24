#ifndef QSWITCHCONTROL_H
#define QSWITCHCONTROL_H

#include <QFrame>

class QPushButton;
class QPropertyAnimation;

class QSwitchControl : public QFrame
{
    Q_OBJECT

public:
    QSwitchControl(QWidget *parent = nullptr);

    void setSwitchedOn(bool on);
    bool getSwitchedOn();

private slots:
    void onStatusChanged();

signals:
    void mouseClicked();
    void statusChanged(bool isOn);

protected:
    void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);

private:
    bool isOn;
    QPushButton *pbSwitch;
    QPropertyAnimation *animation;
};

#endif // QSWITCHCONTROL_H
