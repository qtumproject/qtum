#ifndef RESTOREDIALOG_H
#define RESTOREDIALOG_H

#include <QDialog>

class WalletModel;

namespace Ui {
class RestoreDialog;
}

/**
 * @brief The RestoreDialog class Restore dialog class
 */
class RestoreDialog : public QDialog
{
    Q_OBJECT

public:

    /**
     * @brief RestoreDialog Constructor
     * @param parent Parent windows
     */
    explicit RestoreDialog(QWidget *parent = 0);

    /**
     * @brief ~RestoreDialog Destructor
     */
    ~RestoreDialog();

    /**
     * @brief getParam Get the command line param for restart of the wallet
     * @return Command line param
     */
    QString getParam();

    /**
     * @brief getFileName Get the restore wallet name
     * @return Wallet name path
     */
    QString getFileName();

    /**
     * @brief setModel Set wallet model
     * @param model Wallet model
     */
    void setModel(WalletModel *model);

private Q_SLOTS:
    /**
     * @brief on_btnReset_clicked Reset button click slot
     */
    void on_btnReset_clicked();

    /**
     * @brief on_btnBoxRestore_accepted Ok button click slot
     */
    void on_btnBoxRestore_accepted();

    /**
     * @brief on_btnBoxRestore_rejected Cancel button click slot
     */
    void on_btnBoxRestore_rejected();

    /**
     * @brief on_toolWalletPath_clicked Choose wallet button path slot
     */
    void on_toolWalletPath_clicked();

private:
    Ui::RestoreDialog *ui;
    WalletModel *model;

};

#endif // RESTOREDIALOG_H
