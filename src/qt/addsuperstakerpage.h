#ifndef ADDSUPERSTAKERPAGE_H
#define ADDSUPERSTAKERPAGE_H

#include <QDialog>

class WalletModel;
class ClientModel;

namespace Ui {
class AddSuperStakerPage;
}

class AddSuperStakerPage : public QDialog
{
    Q_OBJECT

public:
    explicit AddSuperStakerPage(QWidget *parent = nullptr);
    ~AddSuperStakerPage();
    void setModel(WalletModel *_model);
    void setClientModel(ClientModel *_clientModel);
    void clearAll();

public Q_SLOTS:
    void accept();
    void reject();
    void show();

private Q_SLOTS:
    void on_cancelButton_clicked();
    void on_updateAddStakerButton();

private:
    Ui::AddSuperStakerPage *ui;
    WalletModel* m_model;
    ClientModel* m_clientModel;
};

#endif // ADDSUPERSTAKERPAGE_H
