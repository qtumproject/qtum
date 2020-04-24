#ifndef SPLITUTXOPAGE_H
#define SPLITUTXOPAGE_H

#include <QDialog>

class WalletModel;
class ExecRPCCommand;

namespace Ui {
class SplitUTXOPage;
}

class SplitUTXOPage : public QDialog
{
    Q_OBJECT

public:
    explicit SplitUTXOPage(QWidget *parent = nullptr);
    ~SplitUTXOPage();
    void setModel(WalletModel *_model);
    void setAddress(const QString& address);
    bool isDataValid();
    void clearAll();

public Q_SLOTS:
    void accept();
    void reject();

private Q_SLOTS:
    void updateDisplayUnit();
    void on_updateSplitCoinsButton();
    void on_splitCoinsClicked();
    void on_cancelButtonClicked();

private:
    Ui::SplitUTXOPage *ui;
    WalletModel* m_model;
    ExecRPCCommand *m_execRPCCommand;
};

#endif // SPLITUTXOPAGE_H
