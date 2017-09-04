#include "abiparam.h"
#include "contractabi.h"

#include <QHBoxLayout>

ABIParam::ABIParam(int ID, const ParameterABI &param, QWidget *parent) :
    QWidget(parent),
    m_paramName(new QLabel(this)),
    m_ParamValue(new QLineEdit(this))
{
    // Set up layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(30);
    mainLayout->setContentsMargins(0,0,0,0);
    m_ParamID = ID;
    m_paramName->setText(QString("Parameter %1").arg(ID + 1));
    m_paramName->setMinimumWidth(110);
    m_ParamValue->setFixedWidth(370);

    mainLayout->addWidget(m_paramName);
    mainLayout->addWidget(m_ParamValue);
    mainLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Fixed));
    m_paramName->setToolTip(QString("%2 %1").arg(QString::fromStdString(param.name)).arg(QString::fromStdString(param.type)));
}

QString ABIParam::getValue()
{
    return m_ParamValue->text();
}
