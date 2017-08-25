#include "abifunctionfield.h"
#include "abiparamsfield.h"
#include "contractabi.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStringListModel>

#include <iostream>
ABIFunctionField::ABIFunctionField(QWidget *parent) :
    QWidget(parent),
    m_contractABI(0),
    m_comboBoxFunc(new QComboBox(this)),
    m_paramsField(new QStackedWidget(this))
{
    // Setup layouts
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *topLayout = new QHBoxLayout(this);
    QLabel *labelFunction = new QLabel(tr("Function:"));
    topLayout->addWidget(labelFunction);
    topLayout->addSpacerItem(new QSpacerItem(30, 20, QSizePolicy::Fixed, QSizePolicy::Fixed));
    topLayout->addWidget(m_comboBoxFunc);

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_paramsField);
    connect(m_comboBoxFunc, SIGNAL(currentIndexChanged(int)), m_paramsField, SLOT(setCurrentIndex(int)));
}

void ABIFunctionField::updateABIFunctionField()
{
    if(m_contractABI != NULL)
    {
        // Populate the control with functions
        std::vector<FunctionABI> functions = m_contractABI->functions;
        QStringList functionList;
        QStringListModel *functionModel = new QStringListModel(this);
        for (std::vector<FunctionABI>::const_iterator func = functions.begin() ; func != functions.end(); ++func)
        {
            ABIParamsField *abiParamsField = new ABIParamsField(this);
            abiParamsField->updateParamsField(*func);

            m_paramsField->addWidget(abiParamsField);
            QString funcName = QString::fromStdString((*func).name);
            QString funcSelector = QString::fromStdString((*func).selector());
            functionList.append(QString(funcName + "(" + funcSelector + ")"));

            m_abiFunctionList.append(&(*func));
        }
        functionModel->setStringList(functionList);
        m_comboBoxFunc->setModel(functionModel);
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

const FunctionABI *ABIFunctionField::getSelectedFunction() const
{
    // Get the currently selected function
    int currentFunc = m_comboBoxFunc->currentIndex();
    if(currentFunc == -1)
        return NULL;

    return m_abiFunctionList[currentFunc];
}

