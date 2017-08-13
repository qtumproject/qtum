#ifndef CREATECONTRACT_H
#define CREATECONTRACT_H

#include <QWidget>

class PlatformStyle;

namespace Ui {
class CreateContract;
}

class CreateContract : public QWidget
{
    Q_OBJECT

public:
    explicit CreateContract(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~CreateContract();

    void setLinkLabels();

Q_SIGNALS:

public Q_SLOTS:
    void on_clearAll_clicked();

private:
    Ui::CreateContract *ui;
};

#endif // CREATECONTRACT_H
