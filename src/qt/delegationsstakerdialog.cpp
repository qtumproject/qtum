#include "delegationsstakerdialog.h"
#include "qt/forms/ui_delegationsstakerdialog.h"

#include <QMenu>

DelegationsStakerDialog::DelegationsStakerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DelegationsStakerDialog),
    model(0)
{
    ui->setupUi(this);

    // Actions
    QAction *copyAddressAction = new QAction(tr("Copy address"), this);
    QAction *copyFeeAction = new QAction(tr("Copy fee"), this);
    QAction *copyPoDAction = new QAction(tr("Copy PoD"), this);

    setWindowTitle(tr("Delegate for super staker"));

    contextMenu = new QMenu(ui->delegationView);
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyFeeAction);
    contextMenu->addAction(copyPoDAction);

    connect(ui->delegationView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));

    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
    connect(copyFeeAction, SIGNAL(triggered()), this, SLOT(copyFee()));
    connect(copyPoDAction, SIGNAL(triggered()), this, SLOT(copyPoD()));
}

DelegationsStakerDialog::~DelegationsStakerDialog()
{
    delete ui;
}

void DelegationsStakerDialog::setModel(WalletModel *_model)
{
    this->model = _model;
    // TODO
}

void DelegationsStakerDialog::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->delegationView->indexAt(point);
    QModelIndexList selection = ui->delegationView->selectionModel()->selectedRows(0);
    if (selection.empty())
        return;

    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void DelegationsStakerDialog::copyAddress()
{
    // TODO
}

void DelegationsStakerDialog::copyFee()
{
    // TODO
}

void DelegationsStakerDialog::copyPoD()
{
    // TODO
}
