#ifndef CONTRACTPARAMFIELD_H
#define CONTRACTPARAMFIELD_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>

class ParameterABI;
/**
 * @brief The ABIParam class ABI parameter widget
 */
class ABIParam : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief ABIParam Constructor
     * @param ID Id if the parameter
     * @param name Name of the parameter
     * @param parent Parent windows for the GUI control
     */
    explicit ABIParam(int ID, const ParameterABI &param, QWidget *parent = 0);

    /**
     * @brief getValue Get the value of the parameter
     * @return String representation of the value
     */
    QString getValue();

Q_SIGNALS:

public Q_SLOTS:

private:
    int m_ParamID;
    QLabel *m_paramName;
    QLineEdit *m_ParamValue;
};

#endif // CONTRACTPARAMFIELD_H
