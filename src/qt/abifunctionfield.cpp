#include "abifunctionfield.h"
#include "abiparamsfield.h"
#include "contractabi.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStringListModel>

#include <iostream>
ABIFunctionField::ABIFunctionField(FunctionType type, QWidget *parent) :
    QWidget(parent),
    m_contractABI(0),
    m_func(new QWidget(this)),
    m_comboBoxFunc(new QComboBox(this)),
    m_paramsField(new QStackedWidget(this)),
    m_functionType(type)
{
    // Setup layouts
    m_comboBoxFunc->setFixedWidth(370);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *topLayout = new QHBoxLayout(this);
    topLayout->setSpacing(30);
    topLayout->setContentsMargins(0, 0, 0, 0);

    m_labelFunction = new QLabel(tr("Function:"));
    m_labelFunction->setMinimumWidth(110);
    topLayout->addWidget(m_labelFunction);

    topLayout->addWidget(m_comboBoxFunc);
    topLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Fixed));

    m_func->setLayout(topLayout);
    mainLayout->addWidget(m_func);
    mainLayout->addWidget(m_paramsField);
    mainLayout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Fixed, QSizePolicy::Expanding));
    connect(m_comboBoxFunc, SIGNAL(currentIndexChanged(int)), m_paramsField, SLOT(setCurrentIndex(int)));

    m_func->setVisible(false);
}

void ABIFunctionField::updateABIFunctionField()
{
    // Clear the content
    clear();

    if(m_contractABI != NULL)
    {
        // Populate the control with functions
        std::vector<FunctionABI> functions = m_contractABI->functions;
        QStringList functionList;
        QStringListModel *functionModel = new QStringListModel(this);
        for (int func = 0; func < (int)functions.size(); ++func)
        {
            const FunctionABI &function = functions[func];
            if((m_functionType == Constructor && function.type != "constructor") ||
                    (m_functionType == Function && function.type == "constructor"))
            {
                continue;
            }

            ABIParamsField *abiParamsField = new ABIParamsField(this);
            abiParamsField->updateParamsField(function);

            m_paramsField->addWidget(abiParamsField);
            QString funcName = QString::fromStdString(function.name);
            QString funcSelector = QString::fromStdString(function.selector());
            functionList.append(QString(funcName + "(" + funcSelector + ")"));

            m_abiFunctionList.append(func);
        }
        functionModel->setStringList(functionList);
        m_comboBoxFunc->setModel(functionModel);

        if(m_functionType == Function)
        {
            bool visible = m_abiFunctionList.size() > 0;
            m_func->setVisible(visible);
        }
    }
}

void ABIFunctionField::clear()
{
    m_comboBoxFunc->clear();
    m_abiFunctionList.clear();
    for(int i = m_paramsField->count() - 1; i >= 0; i--)
    {
        QWidget* widget = m_paramsField->widget(i);
        m_paramsField->removeWidget(widget);
        widget->deleteLater();
    }
}

void ABIFunctionField::setContractABI(ContractABI *contractABI)
{
    m_contractABI = contractABI;
    updateABIFunctionField();
}

QString ABIFunctionField::getParamValue(int paramID)
{
    return ((ABIParamsField*)m_paramsField->currentWidget())->getParamValue(paramID);
}

QStringList ABIFunctionField::getParamsValues()
{
    return ((ABIParamsField*)m_paramsField->currentWidget())->getParamsValues();
}

std::vector<std::string> ABIFunctionField::getValuesVector()
{
    QStringList qlist = getParamsValues();
    std::vector<std::string> result(qlist.size());
    for (int i=0; i<qlist.size(); i++)
    {
      result[i] = qlist.at(i).toUtf8().data();
    }
    return result;
}

int ABIFunctionField::getSelectedFunction() const
{
    // Get the currently selected function
    int currentFunc = m_comboBoxFunc->currentIndex();
    if(currentFunc == -1)
        return -1;

    return m_abiFunctionList[currentFunc];
}

