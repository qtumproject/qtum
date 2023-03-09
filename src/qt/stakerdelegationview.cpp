#include "stakerdelegationview.h"

#include <qt/bitcoinunits.h>
#include <qt/delegationfilterproxy.h>
#include <qt/delegationstakeritemmodel.h>
#include <qt/optionsmodel.h>
#include <qt/walletmodel.h>

#include <QComboBox>
#include <QDateTimeEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QScrollBar>
#include <QSpinBox>
#include <QTableView>

StakerDelegationView::StakerDelegationView(QWidget *parent) :
    QWidget(parent),
    model(0),
    delegationProxyModel(0),
    delegationView(0),
    columnResizingFixer(0)
{
    // Build filter row
    setContentsMargins(0,0,0,0);

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(0,6,0,6);
    hlayout->setSpacing(10);

    dateWidget = new QComboBox(this);
    dateWidget->setFixedWidth(DATE_COLUMN_WIDTH - 5);

    dateWidget->addItem(tr("All"), All);
    dateWidget->addItem(tr("Today"), Today);
    dateWidget->addItem(tr("This week"), ThisWeek);
    dateWidget->addItem(tr("This month"), ThisMonth);
    dateWidget->addItem(tr("Last month"), LastMonth);
    dateWidget->addItem(tr("This year"), ThisYear);
    dateWidget->addItem(tr("Range..."), Range);
    hlayout->addWidget(dateWidget);

    addressWidget = new QLineEdit(this);
#if QT_VERSION >= 0x040700
    addressWidget->setPlaceholderText(tr("Enter address to search"));
#endif
    hlayout->addWidget(addressWidget);

    feeWidget = new QSpinBox(this);
    feeWidget->setMinimum(0);
    feeWidget->setMaximum(100);
    feeWidget->setValue(0);
    feeWidget->setSuffix("%");
    feeWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    feeWidget->setFixedWidth(FEE_COLUMN_WIDTH - 10);
    hlayout->addWidget(feeWidget);

    amountWidget = new QLineEdit(this);
    amountWidget->setFixedWidth(AMOUNT_COLUMN_WIDTH - 5);
#if QT_VERSION >= 0x040700
    amountWidget->setPlaceholderText(tr("Min amount"));
#endif
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
    hlayout->addSpacing(width);

    // Always show scroll bar
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->setTabKeyNavigation(false);
    view->setContextMenuPolicy(Qt::CustomContextMenu);

    delegationView = view;

    // Actions
    QAction *copyAddressAction = new QAction(tr("Copy address"), this);
    QAction *copyFeeAction = new QAction(tr("Copy fee"), this);
    QAction *copyAmount = new QAction(tr("Copy amount"), this);

    contextMenu = new QMenu(delegationView);
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyFeeAction);
    contextMenu->addAction(copyAmount);

    connect(delegationView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));

    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
    connect(copyFeeAction, SIGNAL(triggered()), this, SLOT(copyFee()));
    connect(copyAmount, SIGNAL(triggered()), this, SLOT(copyAmount()));

    connect(dateWidget, SIGNAL(activated(int)), this, SLOT(chooseDate(int)));
    connect(addressWidget, SIGNAL(textChanged(QString)), this, SLOT(changedPrefix(QString)));
    connect(feeWidget, SIGNAL(valueChanged(int)), this, SLOT(changedFee(int)));
    connect(amountWidget, SIGNAL(textChanged(QString)), this, SLOT(changedAmount()));
}

void StakerDelegationView::setModel(WalletModel *_model)
{
    this->model = _model;

    if(_model)
    {
        delegationProxyModel = new DelegationFilterProxy(this);
        delegationProxyModel->setSourceModel(_model->getDelegationStakerItemModel());
        delegationProxyModel->setDynamicSortFilter(true);
        delegationProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        delegationProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        delegationProxyModel->setSortRole(Qt::EditRole);

        delegationView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        delegationView->setModel(delegationProxyModel);
        delegationView->setAlternatingRowColors(true);
        delegationView->setSelectionBehavior(QAbstractItemView::SelectRows);
        delegationView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        delegationView->setSortingEnabled(true);
        delegationView->sortByColumn(DelegationStakerItemModel::Date, Qt::DescendingOrder);
        delegationView->verticalHeader()->hide();

        delegationView->setColumnWidth(DelegationStakerItemModel::Date, DATE_COLUMN_WIDTH);
        delegationView->setColumnWidth(DelegationStakerItemModel::Fee, FEE_COLUMN_WIDTH);
        delegationView->setColumnWidth(DelegationStakerItemModel::Weight, AMOUNT_COLUMN_WIDTH);

        columnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(delegationView, AMOUNT_MINIMUM_COLUMN_WIDTH, MINIMUM_COLUMN_WIDTH, this, 3);
    }
}

void StakerDelegationView::setSuperStakerData(const QString &staker, const int &fee)
{
    if(!delegationProxyModel)
        return;

    delegationProxyModel->setStaker(staker);

    dateWidget->setCurrentIndex(0);
    addressWidget->setText("");
    feeWidget->setValue(fee);
    amountWidget->setText("");

    chooseDate(0);
    changedPrefix("");
    changedFee(fee);
    changedAmount();
}

QWidget *StakerDelegationView::createDateRangeWidget()
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

void StakerDelegationView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    columnResizingFixer->stretchColumnWidth(DelegationStakerItemModel::Delegate);
}

void StakerDelegationView::dateRangeChanged()
{
    if(!delegationProxyModel)
        return;
    delegationProxyModel->setDateRange(
                QDateTime(dateFrom->date()),
                QDateTime(dateTo->date()).addDays(1));
}

void StakerDelegationView::contextualMenu(const QPoint &point)
{
    QModelIndex index = delegationView->indexAt(point);
    QModelIndexList selection = delegationView->selectionModel()->selectedRows(0);
    if (selection.empty())
        return;

    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void StakerDelegationView::copyAddress()
{
    GUIUtil::copyEntryData(delegationView, 0, DelegationStakerItemModel::DelegateRole);
}

void StakerDelegationView::copyFee()
{
    GUIUtil::copyEntryData(delegationView, 0, DelegationStakerItemModel::FormattedFeeRole);
}

void StakerDelegationView::copyAmount()
{
    GUIUtil::copyEntryData(delegationView, 0, DelegationStakerItemModel::FormattedWeightRole);
}

void StakerDelegationView::chooseDate(int idx)
{
    if(!delegationProxyModel)
        return;
    QDate current = QDate::currentDate();
    dateRangeWidget->setVisible(false);
    switch(dateWidget->itemData(idx).toInt())
    {
    case All:
        delegationProxyModel->setDateRange(
                    DelegationFilterProxy::MIN_DATE,
                    DelegationFilterProxy::MAX_DATE);
        break;
    case Today:
        delegationProxyModel->setDateRange(
                    QDateTime(current),
                    DelegationFilterProxy::MAX_DATE);
        break;
    case ThisWeek: {
        // Find last Monday
        QDate startOfWeek = current.addDays(-(current.dayOfWeek()-1));
        delegationProxyModel->setDateRange(
                    QDateTime(startOfWeek),
                    DelegationFilterProxy::MAX_DATE);

    } break;
    case ThisMonth:
        delegationProxyModel->setDateRange(
                    QDateTime(QDate(current.year(), current.month(), 1)),
                    DelegationFilterProxy::MAX_DATE);
        break;
    case LastMonth:
        delegationProxyModel->setDateRange(
                    QDateTime(QDate(current.year(), current.month(), 1).addMonths(-1)),
                    QDateTime(QDate(current.year(), current.month(), 1)));
        break;
    case ThisYear:
        delegationProxyModel->setDateRange(
                    QDateTime(QDate(current.year(), 1, 1)),
                    DelegationFilterProxy::MAX_DATE);
        break;
    case Range:
        dateRangeWidget->setVisible(true);
        dateRangeChanged();
        break;
    }
}

void StakerDelegationView::changedPrefix(const QString &prefix)
{
    if(!delegationProxyModel)
        return;
    delegationProxyModel->setAddrPrefix(prefix);
}

void StakerDelegationView::changedFee(int fee)
{
    if(!delegationProxyModel)
        return;

    delegationProxyModel->setMinFee(fee);
}

void StakerDelegationView::changedAmount()
{
    if(!delegationProxyModel)
        return;
    CAmount amount_parsed = 0;
    if (BitcoinUnits::parse(model->getOptionsModel()->getDisplayUnit(), amountWidget->text(), &amount_parsed)) {
        delegationProxyModel->setMinAmount(amount_parsed);
    }
    else
    {
        delegationProxyModel->setMinAmount(0);
    }
}
