#ifndef EDITCONTRACTINFODIALOG_H
#define EDITCONTRACTINFODIALOG_H

#include <QDialog>

class ContractTableModel;

namespace Ui {
class EditContractInfoDialog;
}

QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
QT_END_NAMESPACE

/** Dialog for editing a contract information.
 */
class EditContractInfoDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        NewContractInfo,
        EditContractInfo
    };

    explicit EditContractInfoDialog(Mode mode, QWidget *parent = 0);
    ~EditContractInfoDialog();

    void setModel(ContractTableModel *model);
    void loadRow(int row);

    QString getAddress() const;
    void setAddress(const QString &address);
    QString getABI() const;
    void setABI(const QString &ABI);

public Q_SLOTS:
    void accept();

private:
    bool saveCurrentRow();

    Ui::EditContractInfoDialog *ui;
    QDataWidgetMapper *mapper;
    Mode mode;
    ContractTableModel *model;

    QString address;
    QString ABI;
};

#endif // EDITCONTRACTINFODIALOG_H
