#include <qt/nfttransactionview.h>

#include <qt/walletmodel.h>
#include <qt/platformstyle.h>
#include <qt/nfttransactiontablemodel.h>
#include <qt/nfttransactionrecord.h>
#include <qt/nfttxfilterproxy.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/nftitemmodel.h>
#include <qt/nftdescdialog.h>
#include <qt/styleSheet.h>

#include <QComboBox>
#include <QDateTimeEdit>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPoint>
#include <QScrollBar>
#include <QTableView>
#include <QVBoxLayout>
#include <QRegularExpressionValidator>

#define paternNftAmount "^([0-9]{1,1}|10)"

NftTransactionView::NftTransactionView(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    model(0),
    nftTxProxyModel(0),
    nftView(0),
    columnResizingFixer(0)
{
    // Build filter row
    setContentsMargins(0,0,0,0);

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(6,6,6,6);
    hlayout->setSpacing(10);
    hlayout->addSpacing(STATUS_COLUMN_WIDTH);

    dateWidget = new QComboBox(this);
    dateWidget->setFixedWidth(DATE_COLUMN_WIDTH -10);

    dateWidget->addItem(tr("All"), All);
    dateWidget->addItem(tr("Today"), Today);
    dateWidget->addItem(tr("This week"), ThisWeek);
    dateWidget->addItem(tr("This month"), ThisMonth);
    dateWidget->addItem(tr("Last month"), LastMonth);
    dateWidget->addItem(tr("This year"), ThisYear);
    dateWidget->addItem(tr("Range..."), Range);
    hlayout->addWidget(dateWidget);

    typeWidget = new QComboBox(this);
    typeWidget->setFixedWidth(TYPE_COLUMN_WIDTH -10);

    typeWidget->addItem(tr("All"), NftTxFilterProxy::ALL_TYPES);
    typeWidget->addItem(tr("Received with"), NftTxFilterProxy::TYPE(NftTransactionRecord::RecvWithAddress));
    typeWidget->addItem(tr("Sent to"), NftTxFilterProxy::TYPE(NftTransactionRecord::SendToAddress));
    typeWidget->addItem(tr("To yourself"), NftTxFilterProxy::TYPE(NftTransactionRecord::SendToSelf));
    hlayout->addWidget(typeWidget);

    addressWidget = new QLineEdit(this);
#if QT_VERSION >= 0x040700
    addressWidget->setPlaceholderText(tr("Enter address to search"));
#endif
    hlayout->addWidget(addressWidget);

    nameWidget = new QComboBox(this);
    nameWidget->setFixedWidth(NAME_COLUMN_WIDTH -10);
    nameWidget->addItem(tr("All"), "");

    hlayout->addWidget(nameWidget);

    amountWidget = new QLineEdit(this);
#if QT_VERSION >= 0x040700
    amountWidget->setPlaceholderText(tr("Min amount"));
#endif
    amountWidget->setFixedWidth(AMOUNT_COLUMN_WIDTH - 10);

    QRegularExpression regEx;
    regEx.setPattern(paternNftAmount);
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(amountWidget);
    validator->setRegularExpression(regEx);
    amountWidget->setValidator(validator);
    hlayout->addWidget(amountWidget);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0,0,0,0);
    vlayout->setSpacing(0);

    QTableView *view = new QTableView(this);
    vlayout->addLayout(hlayout);
    vlayout->addWidget(createDateRangeWidget());
    vlayout->addWidget(view);
    vlayout->setSpacing(0);
    int width = view->verticalScrollBar()->sizeHint().width();
    // Cover scroll bar width with spacing
    if (platformStyle->getUseExtraSpacing()) {
        hlayout->addSpacing(width+2);
    } else {
        hlayout->addSpacing(width);
    }
    // Always show scroll bar
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->setTabKeyNavigation(false);
    view->setContextMenuPolicy(Qt::CustomContextMenu);

    view->installEventFilter(this);
    nftView = view;

    // Actions
    QAction *copyAddressAction = new QAction(tr("Copy address"), this);
    QAction *copyAmountAction = new QAction(tr("Copy amount"), this);
    QAction *copyTxIDAction = new QAction(tr("Copy transaction ID"), this);
    QAction *copyTxPlainText = new QAction(tr("Copy full transaction details"), this);
    QAction *showDetailsAction = new QAction(tr("Show transaction details"), this);

    contextMenu = new QMenu(nftView);
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyAmountAction);
    contextMenu->addAction(copyTxIDAction);
    contextMenu->addAction(copyTxPlainText);
    contextMenu->addAction(showDetailsAction);

    connect(dateWidget, SIGNAL(activated(int)), this, SLOT(chooseDate(int)));
    connect(typeWidget, SIGNAL(activated(int)), this, SLOT(chooseType(int)));
    connect(addressWidget, SIGNAL(textChanged(QString)), this, SLOT(changedPrefix(QString)));
    connect(amountWidget, SIGNAL(textChanged(QString)), this, SLOT(changedAmount(QString)));
    connect(nameWidget, SIGNAL(activated(int)), this, SLOT(chooseName(int)));

    connect(view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));

    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
    connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyAmount()));
    connect(copyTxIDAction, SIGNAL(triggered()), this, SLOT(copyTxID()));
    connect(copyTxPlainText, SIGNAL(triggered()), this, SLOT(copyTxPlainText()));
    connect(showDetailsAction, SIGNAL(triggered()), this, SLOT(showDetails()));
    connect(nftView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(showDetails()));
}

void NftTransactionView::setModel(WalletModel *_model)
{
    this->model = _model;
    if(_model)
    {
        refreshNameWidget();
        connect(model->getNftItemModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),this, SLOT(addToNameWidget(QModelIndex,int,int)));
        connect(model->getNftItemModel(), SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),this, SLOT(removeFromNameWidget(QModelIndex,int,int)));

        nftTxProxyModel = new NftTxFilterProxy(this);
        nftTxProxyModel->setSourceModel(_model->getNftTransactionTableModel());
        nftTxProxyModel->setDynamicSortFilter(true);
        nftTxProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        nftTxProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

        nftTxProxyModel->setSortRole(Qt::EditRole);

        nftView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        nftView->setModel(nftTxProxyModel);
        nftView->setAlternatingRowColors(true);
        nftView->setSelectionBehavior(QAbstractItemView::SelectRows);
        nftView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        nftView->setSortingEnabled(true);
        nftView->sortByColumn(NftTransactionTableModel::Date, Qt::DescendingOrder);
        nftView->verticalHeader()->hide();

        nftView->setColumnWidth(NftTransactionTableModel::Status, STATUS_COLUMN_WIDTH);
        nftView->setColumnWidth(NftTransactionTableModel::Date, DATE_COLUMN_WIDTH);
        nftView->setColumnWidth(NftTransactionTableModel::Type, TYPE_COLUMN_WIDTH);
        nftView->setColumnWidth(NftTransactionTableModel::Name, NAME_COLUMN_WIDTH);
        nftView->setColumnWidth(NftTransactionTableModel::Amount, AMOUNT_COLUMN_WIDTH);

        columnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(nftView, AMOUNT_MINIMUM_COLUMN_WIDTH, MINIMUM_COLUMN_WIDTH, this, 3);
    }
}

QWidget *NftTransactionView::createDateRangeWidget()
{
    dateRangeWidget = new QFrame();
    dateRangeWidget->setFrameStyle(QFrame::Panel | QFrame::Raised);
    dateRangeWidget->setContentsMargins(1,1,1,8);
    QHBoxLayout *layout = new QHBoxLayout(dateRangeWidget);
    layout->setContentsMargins(0,0,0,0);
    layout->addSpacing(23);
    layout->addWidget(new QLabel(tr("Range:")));

    dateFrom = new QDateTimeEdit(this);
    dateFrom->setDisplayFormat("dd/MM/yy");
    dateFrom->setCalendarPopup(true);
    dateFrom->setMinimumWidth(100);
    dateFrom->setDate(QDate::currentDate().addDays(-7));
    layout->addWidget(dateFrom);
    layout->addWidget(new QLabel(tr("to")));

    dateTo = new QDateTimeEdit(this);
    dateTo->setDisplayFormat("dd/MM/yy");
    dateTo->setCalendarPopup(true);
    dateTo->setMinimumWidth(100);
    dateTo->setDate(QDate::currentDate());
    layout->addWidget(dateTo);
    layout->addStretch();

    // Hide by default
    dateRangeWidget->setVisible(false);

    // Notify on change
    connect(dateFrom, SIGNAL(dateChanged(QDate)), this, SLOT(dateRangeChanged()));
    connect(dateTo, SIGNAL(dateChanged(QDate)), this, SLOT(dateRangeChanged()));

    return dateRangeWidget;
}

void NftTransactionView::refreshNameWidget()
{
    if(model)
    {
        NftItemModel *tim = model->getNftItemModel();
        for(int i = 0; i < tim->rowCount(); i++)
        {
            QString name = tim->data(tim->index(i, 0), NftItemModel::NameRole).toString();
            if(nameWidget->findText(name) == -1)
                nameWidget->addItem(name, name);
        }
    }
}

void NftTransactionView::addToNameWidget(const QModelIndex &parent, int start, int /*end*/)
{
    if(model)
    {
        NftItemModel *tim = model->getNftItemModel();
        QString name = tim->index(start, NftItemModel::Name, parent).data().toString();
        if(nameWidget->findText(name) == -1)
            nameWidget->addItem(name, name);
    }
}

void NftTransactionView::removeFromNameWidget(const QModelIndex &parent, int start, int /*end*/)
{
    if(model)
    {
        NftItemModel *tim = model->getNftItemModel();
        QString name = tim->index(start, NftItemModel::Name, parent).data().toString();
        int nameCount = 0;

        for(int i = 0; i < tim->rowCount(); i++)
        {
            QString checkName = tim->index(i, NftItemModel::Name, parent).data().toString();
            if(name == checkName)
            {
                nameCount++;
            }
        }

        int nameIndex = nameWidget->findText(name);
        if(nameCount == 1 && nameIndex != -1)
            nameWidget->removeItem(nameIndex);
    }
}

void NftTransactionView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    columnResizingFixer->stretchColumnWidth(NftTransactionTableModel::ToAddress);
}

bool NftTransactionView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_C && ke->modifiers().testFlag(Qt::ControlModifier))
        {
            GUIUtil::copyEntryData(nftView, 0, NftTransactionTableModel::TxPlainTextRole);
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void NftTransactionView::contextualMenu(const QPoint &point)
{
    QModelIndex index = nftView->indexAt(point);
    QModelIndexList selection = nftView->selectionModel()->selectedRows(0);
    if (selection.empty())
        return;

    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void NftTransactionView::dateRangeChanged()
{
    if(!nftTxProxyModel)
        return;
    nftTxProxyModel->setDateRange(
                QDateTime(dateFrom->date()),
                QDateTime(dateTo->date()).addDays(1));
}

void NftTransactionView::showDetails()
{
    if(!nftView->selectionModel())
        return;
    QModelIndexList selection = nftView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        NftDescDialog *dlg = new NftDescDialog(selection.at(0));
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    }
}

void NftTransactionView::copyAddress()
{
    GUIUtil::copyEntryData(nftView, 0, NftTransactionTableModel::AddressRole);
}

void NftTransactionView::copyAmount()
{
    GUIUtil::copyEntryData(nftView, 0, NftTransactionTableModel::AmountRole);
}

void NftTransactionView::copyTxID()
{
    GUIUtil::copyEntryData(nftView, 0, NftTransactionTableModel::TxHashRole);
}

void NftTransactionView::copyTxPlainText()
{
    GUIUtil::copyEntryData(nftView, 0, NftTransactionTableModel::TxPlainTextRole);
}

void NftTransactionView::chooseDate(int idx)
{
    if(!nftTxProxyModel)
        return;
    QDate current = QDate::currentDate();
    dateRangeWidget->setVisible(false);
    switch(dateWidget->itemData(idx).toInt())
    {
    case All:
        nftTxProxyModel->setDateRange(
                    NftTxFilterProxy::MIN_DATE,
                    NftTxFilterProxy::MAX_DATE);
        break;
    case Today:
        nftTxProxyModel->setDateRange(
                    QDateTime(current),
                    NftTxFilterProxy::MAX_DATE);
        break;
    case ThisWeek: {
        // Find last Monday
        QDate startOfWeek = current.addDays(-(current.dayOfWeek()-1));
        nftTxProxyModel->setDateRange(
                    QDateTime(startOfWeek),
                    NftTxFilterProxy::MAX_DATE);

    } break;
    case ThisMonth:
        nftTxProxyModel->setDateRange(
                    QDateTime(QDate(current.year(), current.month(), 1)),
                    NftTxFilterProxy::MAX_DATE);
        break;
    case LastMonth:
        nftTxProxyModel->setDateRange(
                    QDateTime(QDate(current.year(), current.month(), 1).addMonths(-1)),
                    QDateTime(QDate(current.year(), current.month(), 1)));
        break;
    case ThisYear:
        nftTxProxyModel->setDateRange(
                    QDateTime(QDate(current.year(), 1, 1)),
                    NftTxFilterProxy::MAX_DATE);
        break;
    case Range:
        dateRangeWidget->setVisible(true);
        dateRangeChanged();
        break;
    }
}

void NftTransactionView::chooseType(int idx)
{
    if(!nftTxProxyModel)
        return;
    nftTxProxyModel->setTypeFilter(
                typeWidget->itemData(idx).toInt());
}

void NftTransactionView::chooseName(int idx)
{
    if(!nftTxProxyModel)
        return;
    nftTxProxyModel->setName(
                nameWidget->itemData(idx).toString());
}

void NftTransactionView::changedPrefix(const QString &prefix)
{
    if(!nftTxProxyModel)
        return;
    nftTxProxyModel->setAddressPrefix(prefix);
}

void NftTransactionView::changedAmount(const QString &amount)
{
    if(!nftTxProxyModel)
        return;
    qint32 amount_parsed = amount.toInt();
    nftTxProxyModel->setMinAmount(amount_parsed);
}
