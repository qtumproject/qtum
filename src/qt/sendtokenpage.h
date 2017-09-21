#ifndef SENDTOKENPAGE_H
#define SENDTOKENPAGE_H

#include <QWidget>

namespace Ui {
class SendTokenPage;
}

class SendTokenPage : public QWidget
{
    Q_OBJECT

public:
    explicit SendTokenPage(QWidget *parent = 0);
    ~SendTokenPage();

    void clearAll();

private Q_SLOTS:
    void on_clearButton_clicked();

private:
    Ui::SendTokenPage *ui;
};

#endif // SENDTOKENPAGE_H
