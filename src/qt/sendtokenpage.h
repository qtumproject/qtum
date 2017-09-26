#ifndef SENDTOKENPAGE_H
#define SENDTOKENPAGE_H

#include <QWidget>

class ClientModel;

namespace Ui {
class SendTokenPage;
}

class SendTokenPage : public QWidget
{
    Q_OBJECT

public:
    explicit SendTokenPage(QWidget *parent = 0);
    ~SendTokenPage();

    void setClientModel(ClientModel *clientModel);
    void clearAll();

private Q_SLOTS:
    void on_clearButton_clicked();
    void on_numBlocksChanged();

private:
    Ui::SendTokenPage *ui;
    ClientModel* m_clientModel;
};

#endif // SENDTOKENPAGE_H
