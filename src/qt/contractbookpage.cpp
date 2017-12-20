#include "contractbookpage.h"
#include "ui_contractbookpage.h"

#include "contracttablemodel.h"
#include "csvmodelwriter.h"
#include "guiutil.h"
#include "platformstyle.h"

#include <QIcon>
#include <QMenu>
#include <QSortFilterProxyModel>

ContractBookPage::ContractBookPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ContractBookPage),
    model(0)
{
    ui->setupUi(this);

    if (!platformStyle->getImagesOnButtons()) {
        ui->newContractInfo->setIcon(QIcon());
        ui->copyAddress->setIcon(QIcon());
        ui->deleteContractInfo->setIcon(QIcon());
        ui->exportButton->setIcon(QIcon());
    } else {
        ui->newContractInfo->setIcon(platformStyle->SingleColorIcon(":/icons/add"));
        ui->copyAddress->setIcon(platformStyle->SingleColorIcon(":/icons/editcopy"));
        ui->deleteContractInfo->setIcon(platformStyle->SingleColorIcon(":/icons/remove"));
        ui->exportButton->setIcon(platformStyle->SingleColorIcon(":/icons/export"));
    }

    // Context menu actions
    QAction *copyAddressAction = new QAction(tr("&Copy Address"), this);
    QAction *copyNameAction = new QAction(tr("&Copy Name"), this);
    QAction *editAction = new QAction(tr("&Edit"), this);
    QAction *deleteAction = new QAction(ui->deleteContractInfo->text(), this);

    // Build context menu
    contextMenu = new QMenu(this);
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyNameAction);
    contextMenu->addAction(editAction);
    contextMenu->addAction(deleteAction);
    contextMenu->addSeparator();

    // Connect signals for context menu actions
    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(on_copyAddress_clicked()));
    connect(copyNameAction, SIGNAL(triggered()), this, SLOT(onCopyNameAction()));
    connect(editAction, SIGNAL(triggered()), this, SLOT(onEditAction()));
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(on_deleteContractInfo_clicked()));

    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));

    connect(ui->chooseContractInfo, SIGNAL(clicked()), this, SLOT(accept()));
}

ContractBookPage::~ContractBookPage()
{
    delete ui;
}

void ContractBookPage::setModel(ContractTableModel *_model)
{
    this->model = _model;
    if(!_model)
        return;

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(_model);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    ui->tableView->setModel(proxyModel);
    ui->tableView->sortByColumn(0, Qt::AscendingOrder);

    // Set column widths
#if QT_VERSION < 0x050000
    ui->tableView->horizontalHeader()->setResizeMode(ContractTableModel::Label, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setResizeMode(ContractTableModel::Address, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setResizeMode(ContractTableModel::ABI, QHeaderView::Stretch);
#else
    ui->tableView->horizontalHeader()->setSectionResizeMode(ContractTableModel::Label, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(ContractTableModel::Address, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(ContractTableModel::ABI, QHeaderView::Stretch);
#endif

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        this, SLOT(selectionChanged()));

    // Select row for newly created contract info
    connect(_model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewContractInfo(QModelIndex,int,int)));

    selectionChanged();
}

void ContractBookPage::onCopyNameAction()
{
    GUIUtil::copyEntryData(ui->tableView, ContractTableModel::Label);
}

void ContractBookPage::on_copyAddress_clicked()
{
    GUIUtil::copyEntryData(ui->tableView, ContractTableModel::Address);
}

void ContractBookPage::onEditAction()
{
    // TODO
}

void ContractBookPage::on_newContractInfo_clicked()
{
    // TODO
}

void ContractBookPage::on_deleteContractInfo_clicked()
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    QModelIndexList indexes = table->selectionModel()->selectedRows();
    if(!indexes.isEmpty())
    {
        table->model()->removeRow(indexes.at(0).row());
    }
}

void ContractBookPage::selectionChanged()
{
    // Set button states based on selection
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        ui->deleteContractInfo->setEnabled(true);
        ui->copyAddress->setEnabled(true);
    }
    else
    {
        ui->deleteContractInfo->setEnabled(false);
        ui->copyAddress->setEnabled(false);
    }
}

void ContractBookPage::done(int retval)
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel() || !table->model())
        return;

    // Figure out which contract info was selected, and return it
    QModelIndexList indexes = table->selectionModel()->selectedRows(ContractTableModel::Address);

    Q_FOREACH (const QModelIndex& index, indexes) {
        QVariant address = table->model()->data(index);
        QVariant ABI = table->model()->index(index.row(), ContractTableModel::ABI).data();
        addressValue = address.toString();
        ABIValue = ABI.toString();
    }

    if(addressValue.isEmpty() || ABIValue.isEmpty())
    {
        // If no contract info entry selected, return rejected
        retval = Rejected;
    }

    QDialog::done(retval);
}

void ContractBookPage::on_exportButton_clicked()
{
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Export Contract List"), QString(),
        tr("Comma separated file (*.csv)"), NULL);

    if (filename.isNull())
        return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(proxyModel);
    writer.addColumn("Label", ContractTableModel::Label, Qt::EditRole);
    writer.addColumn("Address", ContractTableModel::Address, Qt::EditRole);
    writer.addColumn("ABI", ContractTableModel::ABI, Qt::EditRole);

    if(!writer.write()) {
        QMessageBox::critical(this, tr("Exporting Failed"),
            tr("There was an error trying to save the address list to %1. Please try again.").arg(filename));
    }
}

void ContractBookPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void ContractBookPage::selectNewContractInfo(const QModelIndex &parent, int begin, int)
{
    QModelIndex idx = proxyModel->mapFromSource(model->index(begin, ContractTableModel::Address, parent));
    if(idx.isValid() && (idx.data(Qt::EditRole).toString() == newContractInfoToSelect))
    {
        // Select row of newly created address, once
        ui->tableView->setFocus();
        ui->tableView->selectRow(idx.row());
        newContractInfoToSelect.clear();
    }
}
