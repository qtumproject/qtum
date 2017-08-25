#ifndef CONTRACTPARAMFIELD_H
#define CONTRACTPARAMFIELD_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>

class ABIParam : public QWidget
{
    Q_OBJECT
public:
    explicit ABIParam(int ID, std::string name, QWidget *parent = 0);
    QString getValue();

Q_SIGNALS:

public Q_SLOTS:

private:
    int m_ParamID;
    QLabel *m_paramName;
    QLineEdit *m_ParamValue;
};

#endif // CONTRACTPARAMFIELD_H
