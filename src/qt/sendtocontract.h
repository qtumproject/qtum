#ifndef SENDTOCONTRACT_H
#define SENDTOCONTRACT_H

#include <QWidget>

class PlatformStyle;

namespace Ui {
class SendToContract;
}

class SendToContract : public QWidget
{
    Q_OBJECT

public:
    explicit SendToContract(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~SendToContract();

private:
    Ui::SendToContract *ui;
};

#endif // SENDTOCONTRACT_H
