#include "abiparam.h"

#include <QHBoxLayout>

ABIParam::ABIParam(int ID, std::string name, QWidget *parent) :
    QWidget(parent),
    m_paramName(new QLabel(this)),
    m_ParamValue(new QLineEdit(this))
{
    // Set up layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(30);
    mainLayout->setContentsMargins(0,0,0,0);
    m_ParamID = ID;
    m_paramName->setText(QString::fromStdString(name));
    m_paramName->setMinimumWidth(110);
    m_ParamValue->setMinimumWidth(170);

    mainLayout->addWidget(m_paramName);
    mainLayout->addWidget(m_ParamValue);
    mainLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Fixed));
}

QString ABIParam::getValue()
{
    return m_ParamValue->text();
}
