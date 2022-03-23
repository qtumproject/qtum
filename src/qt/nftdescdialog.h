#ifndef NFTDESCDIALOG_H
#define NFTDESCDIALOG_H

#include <QDialog>

namespace Ui {
class NftDescDialog;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Dialog showing token details. */
class NftDescDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NftDescDialog(const QModelIndex &idx, QWidget *parent = 0);
    ~NftDescDialog();

private:
    Ui::NftDescDialog *ui;
};

#endif // NFTDESCDIALOG_H
