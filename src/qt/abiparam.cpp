#include "abiparam.h"
#include "contractabi.h"

#include <QHBoxLayout>
#include <QRegularExpressionValidator>

ABIParam::ABIParam(int ID, const ParameterABI &param, QWidget *parent) :
    QWidget(parent),
    m_paramName(new QLabel(this)),
    m_ParamValue(new QValidatedLineEdit(this))
{
    // Set up layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(0,0,0,0);
    m_ParamID = ID;
    m_paramName->setToolTip(tr("Parameter %1 %2").arg(ID + 1).arg(QString::fromStdString(param.name)));
    m_paramName->setFixedWidth(160);
    m_ParamValue->setFixedWidth(370);

    QRegularExpression regEx;
    if(ParameterABI::getRegularExpession(param.decodeType(), regEx))
    {
        QRegularExpressionValidator *validator = new QRegularExpressionValidator(m_ParamValue);
        validator->setRegularExpression(regEx);
        m_ParamValue->setEmptyIsValid(false);
        m_ParamValue->setCheckValidator(validator);
    }

    mainLayout->addWidget(m_paramName);
    mainLayout->addWidget(m_ParamValue);
    mainLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Fixed));

    QFontMetrics metrix(m_paramName->font());
    int width = m_paramName->width() + 10;
    QString text(QString("%2 <b>%1").arg(QString::fromStdString(param.name)).arg(QString::fromStdString(param.type)));
    QString clippedText = metrix.elidedText(text, Qt::ElideRight, width);
    m_paramName->setText(clippedText);
}

QString ABIParam::getValue()
{
    return m_ParamValue->text();
}

bool ABIParam::isValid()
{
    m_ParamValue->checkValidity();
    return m_ParamValue->isValid();
}
