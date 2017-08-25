#include "abiparam.h"

#include <QHBoxLayout>

ABIParam::ABIParam(int ID, std::string name, QWidget *parent) :
    QWidget(parent),
    m_paramName(new QLabel(this)),
    m_ParamValue(new QLineEdit(this))
{
    // Set up layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    m_ParamID = ID;
    m_paramName->setText(QString::fromStdString(name));

    mainLayout->addWidget(m_paramName);
    mainLayout->addSpacerItem(new QSpacerItem(30, 20, QSizePolicy::Fixed, QSizePolicy::Fixed));
    mainLayout->addWidget(m_ParamValue);
}

QString ABIParam::getValue()
{
    return m_ParamValue->text();
}
