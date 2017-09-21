#ifndef RECEIVETOKENPAGE_H
#define RECEIVETOKENPAGE_H

#include <QWidget>

namespace Ui {
class ReceiveTokenPage;
}

class ReceiveTokenPage : public QWidget
{
    Q_OBJECT

public:
    explicit ReceiveTokenPage(QWidget *parent = 0);
    ~ReceiveTokenPage();

private:
    Ui::ReceiveTokenPage *ui;
};

#endif // RECEIVETOKENPAGE_H
