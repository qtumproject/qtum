#ifndef ADDSUPERSTAKERPAGE_H
#define ADDSUPERSTAKERPAGE_H

#include <QDialog>

class WalletModel;

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
    void clearAll();

public Q_SLOTS:
    void accept();
    void reject();
    void show();

private Q_SLOTS:
    void on_cancelButton_clicked();
    void on_addSuperStakerButton_clicked();
    void on_updateAddStakerButton();

private:
    Ui::AddSuperStakerPage *ui;
    WalletModel* m_model;
};

#endif // ADDSUPERSTAKERPAGE_H
