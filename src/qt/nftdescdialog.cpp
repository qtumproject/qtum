#include <qt/nftdescdialog.h>
#include <qt/forms/ui_nftdescdialog.h>
#include <qt/nfttransactiontablemodel.h>
#include <qt/styleSheet.h>

#include <QModelIndex>

NftDescDialog::NftDescDialog(const QModelIndex &idx, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NftDescDialog)
{
    ui->setupUi(this);

    // Set stylesheet
    SetObjectStyleSheet(this, StyleSheetNames::ScrollBarDark);

    setWindowTitle(tr("Details for %1").arg(idx.data(NftTransactionTableModel::TxHashRole).toString()));
    QString desc = idx.data(NftTransactionTableModel::LongDescriptionRole).toString();
    ui->detailText->setHtml(desc);
}

NftDescDialog::~NftDescDialog()
{
    delete ui;
}
