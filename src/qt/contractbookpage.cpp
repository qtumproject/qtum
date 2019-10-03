#include <qt/contractbookpage.h>
#include <qt/forms/ui_contractbookpage.h>

#include <qt/contracttablemodel.h>
#include <qt/csvmodelwriter.h>
#include <qt/editcontractinfodialog.h>
#include <qt/guiutil.h>
#include <qt/platformstyle.h>
#include <qt/styleSheet.h>

#include <QIcon>
#include <QMenu>
#include <QSortFilterProxyModel>

ContractBookPage::ContractBookPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ContractBookPage),
    model(0)
{
    ui->setupUi(this);

    SetObjectStyleSheet(ui->tableView, StyleSheetNames::TableViewLight);

    if (!platformStyle->getImagesOnButtons()) {
        ui->newContractInfo->setIcon(QIcon());
        ui->copyAddress->setIcon(QIcon());
        ui->deleteContractInfo->setIcon(QIcon());
        ui->exportButton->setIcon(QIcon());
    } else {
        ui->newContractInfo->setIcon(platformStyle->MultiStatesIcon(":/icons/add", PlatformStyle::PushButtonLight));
        ui->copyAddress->setIcon(platformStyle->MultiStatesIcon(":/icons/editcopy", PlatformStyle::PushButtonLight));
        ui->deleteContractInfo->setIcon(platformStyle->MultiStatesIcon(":/icons/remove", PlatformStyle::PushButtonLight));
        ui->exportButton->setIcon(platformStyle->MultiStatesIcon(":/icons/export", PlatformStyle::PushButton));
    }

    SetObjectStyleSheet(ui->newContractInfo, StyleSheetNames::ButtonLight);
    SetObjectStyleSheet(ui->copyAddress, StyleSheetNames::ButtonLight);
    SetObjectStyleSheet(ui->deleteContractInfo, StyleSheetNames::ButtonLight);
    SetObjectStyleSheet(ui->exportButton, StyleSheetNames::ButtonGray);
    SetObjectStyleSheet(ui->chooseContractInfo, StyleSheetNames::ButtonGray);

    setWindowTitle(tr("Choose the contract for send/call"));
    ui->labelExplanation->setText(tr("These are your saved contracts. Always check the contract address and the ABI before sending/calling."));

    // Context menu actions
    QAction *copyAddressAction = new QAction(tr("Copy &Address"), this);
    QAction *copyNameAction = new QAction(tr("Copy &Name"), this);
    QAction *copyABIAction = new QAction(tr("Copy &Interface"), this);
    QAction *editAction = new QAction(tr("&Edit"), this);
    QAction *deleteAction = new QAction(tr("&Delete"), this);

    // Build context menu
    contextMenu = new QMenu(this);
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyNameAction);
    contextMenu->addAction(copyABIAction);
    contextMenu->addAction(editAction);
    contextMenu->addAction(deleteAction);
    contextMenu->addSeparator();

    // Connect signals for context menu actions
    connect(copyAddressAction, &QAction::triggered, this, &ContractBookPage::on_copyAddress_clicked);
    connect(copyNameAction, &QAction::triggered, this, &ContractBookPage::onCopyNameAction);
    connect(copyABIAction, &QAction::triggered, this, &ContractBookPage::onCopyABIAction);
    connect(editAction, &QAction::triggered, this, &ContractBookPage::onEditAction);
    connect(deleteAction, &QAction::triggered, this, &ContractBookPage::on_deleteContractInfo_clicked);

    connect(ui->tableView, &QTableView::doubleClicked, this, &ContractBookPage::accept);
    connect(ui->tableView, &QTableView::customContextMenuRequested, this, &ContractBookPage::contextualMenu);

    connect(ui->chooseContractInfo, &QPushButton::clicked, this, &ContractBookPage::accept);
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
    ui->tableView->setColumnWidth(ContractTableModel::Label, ColumnWidths::LABEL_COLUMN_WIDTH);
    ui->tableView->setColumnWidth(ContractTableModel::Address, ColumnWidths::ADDRESS_COLUMN_WIDTH);
#if QT_VERSION < 0x050000
    ui->tableView->horizontalHeader()->setResizeMode(ContractTableModel::Label, QHeaderView::Fixed);
    ui->tableView->horizontalHeader()->setResizeMode(ContractTableModel::Address, QHeaderView::Fixed);
    ui->tableView->horizontalHeader()->setResizeMode(ContractTableModel::ABI, QHeaderView::Stretch);
#else
    ui->tableView->horizontalHeader()->setSectionResizeMode(ContractTableModel::Label, QHeaderView::Fixed);
    ui->tableView->horizontalHeader()->setSectionResizeMode(ContractTableModel::Address, QHeaderView::Fixed);
    ui->tableView->horizontalHeader()->setSectionResizeMode(ContractTableModel::ABI, QHeaderView::Stretch);
#endif

    connect(ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContractBookPage::selectionChanged);

    // Select row for newly created contract info
    connect(_model, &ContractTableModel::rowsInserted, this, &ContractBookPage::selectNewContractInfo);

    selectionChanged();
}

void ContractBookPage::onCopyNameAction()
{
    GUIUtil::copyEntryData(ui->tableView, ContractTableModel::Label);
}

void ContractBookPage::onCopyABIAction()
{
    GUIUtil::copyEntryData(ui->tableView, ContractTableModel::ABI);
}

void ContractBookPage::on_copyAddress_clicked()
{
    GUIUtil::copyEntryData(ui->tableView, ContractTableModel::Address);
}

void ContractBookPage::onEditAction()
{
    if(!model)
        return;

    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList indexes = ui->tableView->selectionModel()->selectedRows();
    if(indexes.isEmpty())
        return;

    EditContractInfoDialog dlg(EditContractInfoDialog::EditContractInfo, this);
    dlg.setModel(model);
    QModelIndex origIndex = proxyModel->mapToSource(indexes.at(0));
    dlg.loadRow(origIndex.row());
    dlg.exec();
}

void ContractBookPage::on_newContractInfo_clicked()
{
    if(!model)
        return;

    EditContractInfoDialog dlg(EditContractInfoDialog::NewContractInfo, this);
    dlg.setModel(model);
    if(dlg.exec())
    {
        newContractInfoToSelect = dlg.getAddress();
    }
}

void ContractBookPage::on_deleteContractInfo_clicked()
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    QModelIndexList indexes = table->selectionModel()->selectedRows();
    if(!indexes.isEmpty())
    {
        int row = indexes.at(0).row();
        QModelIndex index = table->model()->index(row, ContractTableModel::Address);
        QString contractAddress = table->model()->data(index).toString();
        QString message = tr("Are you sure you want to delete the address \"%1\" from your contract address list?");
        if(QMessageBox::Yes == QMessageBox::question(this, tr("Delete contact address"), message.arg(contractAddress),
                                                     QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel))
        {
            table->model()->removeRow(row);
        }
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

    if(addressValue.isEmpty())
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
