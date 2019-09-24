#include "qswitchcontrol.h"

#include <QPushButton>
#include <QPropertyAnimation>
#include <QStyleOption>
#include <QPainter>

static const QSize FrameSize = QSize(68, 30);
static const QSize SwitchSize = QSize (26, 26);
static const int SwitchOffset = (FrameSize.height() - SwitchSize.height()) / 2;

static const QString CustomFrameOnStlye = QString("QFrame { border: none; border-radius: %1; background-color: #4697D9;}").arg(FrameSize.height() / 2);
static const QString CustomFrameOffStlye = QString("QFrame { border: none; border-radius: %1; background-color: #6f80ab;}").arg(FrameSize.height() / 2);
static const QString CustomButtonStlye = QString("QPushButton { border-radius: %1; background-color: white;}").arg(SwitchSize.height() / 2);

QSwitchControl::QSwitchControl(QWidget *parent):
    QFrame(parent),
    isOn(false)
{
    this->setFixedSize(FrameSize);

    pbSwitch = new QPushButton(this);
    pbSwitch->setFixedSize(SwitchSize);
    pbSwitch->setStyleSheet(CustomButtonStlye);

    animation = new QPropertyAnimation(pbSwitch, "geometry", this);
    animation->setDuration(200);

    setSwitchedOn(true);

    connect(this, &QSwitchControl::mouseClicked, this, &QSwitchControl::onStatusChanged);
    connect(pbSwitch, &QPushButton::clicked, this, &QSwitchControl::onStatusChanged);
}

void QSwitchControl::setSwitchedOn(bool on)
{
    isOn = on;

    if(on)
    {
        pbSwitch->move(this->width() - pbSwitch->width() - SwitchOffset, this->y() + SwitchOffset);
        this->setStyleSheet(CustomFrameOnStlye);
    }
    else
    {
        pbSwitch->move(this->x() + SwitchOffset, this->y() + SwitchOffset);
        this->setStyleSheet(CustomFrameOffStlye);
    }
}

bool QSwitchControl::getSwitchedOn()
{
    return isOn;
}

void QSwitchControl::onStatusChanged()
{
    isOn = !isOn;

    QRect currentGeometry(pbSwitch->x(), pbSwitch->y(), pbSwitch->width(), pbSwitch->height());

    if(animation->state() == QAbstractAnimation::Running)
        animation->stop();

    if(isOn)
    {
        this->setStyleSheet(CustomFrameOnStlye);

        animation->setStartValue(currentGeometry);
        animation->setEndValue(QRect(this->width() - pbSwitch->width() - SwitchOffset, pbSwitch->y(), pbSwitch->width(), pbSwitch->height()));
    }
    else
    {
        this->setStyleSheet(CustomFrameOffStlye);

        animation->setStartValue(currentGeometry);
        animation->setEndValue(QRect(SwitchOffset, pbSwitch->y(), pbSwitch->width(), pbSwitch->height()));
    }
    animation->start();

    emit statusChanged(isOn);
}

void QSwitchControl::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)

    emit mouseClicked();
}

void QSwitchControl::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    QFrame::paintEvent(event);
}
