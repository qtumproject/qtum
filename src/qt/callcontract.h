#ifndef CALLCONTRACT_H
#define CALLCONTRACT_H

#include <QWidget>

class PlatformStyle;

namespace Ui {
class CallContract;
}

class CallContract : public QWidget
{
    Q_OBJECT

public:
    explicit CallContract(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~CallContract();

private:
    Ui::CallContract *ui;
};

#endif // CALLCONTRACT_H
