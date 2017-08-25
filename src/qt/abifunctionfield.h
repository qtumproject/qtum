#ifndef ABIFUNCTIONFIELD_H
#define ABIFUNCTIONFIELD_H

#include <QWidget>
#include <QStackedWidget>
#include <QComboBox>
#include <QVector>
#include "contractabi.h"

class ABIParamsField;

/**
 * @brief The ABIFunctionField class ABI functions widget
 */
class ABIFunctionField : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief ABIFunctionField Constructor
     * @param parent Parent windows for the GUI control
     */
    ABIFunctionField(QWidget *parent = 0);

    /**
     * @brief setContractABI Set the contract ABI (list of functions from the contract)
     * @param contractABI Contract ABI
     */
    void setContractABI(ContractABI *contractABI);

    /**
     * @brief getParamValue Get the value of the parameter with the id from the currently selected function
     * @param paramID Id of the input parameter
     * @return Parameter value
     */
    QString getParamValue(int paramID);

    /**
     * @brief getParamsValues Get the values of the whole list of input parameters for the selected function
     * @return Values of the parameters
     */
    QStringList getParamsValues();

    /**
     * @brief getSelectedFunction Get the ABI for the selected function from the contract
     * @return Selected function ABI
     */
    const FunctionABI* getSelectedFunction() const;

Q_SIGNALS:

public Q_SLOTS:

private:
    /**
     * @brief updateABIFunctionField Populate the GUI control with functions
     */
    void updateABIFunctionField();

private:
    ContractABI *m_contractABI;
    QComboBox *m_comboBoxFunc;
    QStackedWidget *m_paramsField;
    QVector<const FunctionABI*> m_abiFunctionList;
};

#endif // ABIFUNCTIONFIELD_H
