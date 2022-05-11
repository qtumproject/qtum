#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/delegationlistwidget.h>
#include <qt/platformstyle.h>
#include <qt/delegationitemwidget.h>
#include <qt/delegationitemmodel.h>
#include <qt/walletmodel.h>
#include <qt/clientmodel.h>

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include <QObject>

DelegationListWidget::DelegationListWidget(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    m_mainLayout(new QVBoxLayout(this)),
    m_model(0),
    m_clientModel(0)
{
    m_platfromStyle = platformStyle;
    m_mainLayout->setSpacing(5);
    m_mainLayout->setContentsMargins(0,0,0,0);
    this->setLayout(m_mainLayout);
    DelegationItemWidget* item = new DelegationItemWidget(platformStyle, this, DelegationItemWidget::New);
    insertItem(0, item);
    m_mainLayout->addStretch();
}

void DelegationListWidget::setModel(WalletModel *_model)
{
    m_model = _model;
    if(m_model && m_model->getDelegationItemModel())
    {
        // Sort delegations
        QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
        DelegationItemModel* delegationModel = m_model->getDelegationItemModel();
        proxyModel->setSourceModel(delegationModel);
        proxyModel->sort(5, Qt::AscendingOrder);
        m_delegationModel = proxyModel;

        // Connect signals and slots
        connect(m_delegationModel, SIGNAL(rowsInserted(QModelIndex,int,int)),this, SLOT(on_rowsInserted(QModelIndex,int,int)));
        connect(m_delegationModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),this, SLOT(on_rowsRemoved(QModelIndex,int,int)));
        connect(m_delegationModel, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),this, SLOT(on_rowsMoved(QModelIndex,int,int,QModelIndex,int)));
        connect(m_delegationModel, SIGNAL(modelReset()),this, SLOT(on_modelReset()));
        connect(m_delegationModel, SIGNAL(layoutChanged()), this, SLOT(on_layoutChanged()));
        connect(m_delegationModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(on_dataChanged(QModelIndex, QModelIndex)));

        // Add items
        on_rowsInserted(QModelIndex(), 0, m_delegationModel->rowCount() - 1);
    }
}

void DelegationListWidget::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;
}

void DelegationListWidget::on_rowsInserted(const QModelIndex &, int start, int end)
{
    for(int i = start; i <= end; i++)
    {
        insertRow(m_delegationModel->index(i, 0), i);
    }
}

void DelegationListWidget::on_rowsRemoved(const QModelIndex &, int start, int end)
{
    for(int i = end; i >= start; i--)
    {
        DelegationItemWidget* row = removeRow(i);
        if(row) delete row;
    }
}

void DelegationListWidget::on_rowsMoved(const QModelIndex &, int start, int end, const QModelIndex &, int row)
{
    QList<DelegationItemWidget*> movedRows;
    for(int i = end; i >= start; i--)
    {
        DelegationItemWidget* row = removeRow(i);
        movedRows.prepend(row);
    }

    for(int i = 0; i <movedRows.size(); i++)
    {
        int position = row + i;
        DelegationItemWidget* item = movedRows[i];
        m_rows.insert(position, item);
        m_mainLayout->insertWidget(position, item);
    }
}

void DelegationListWidget::on_modelReset()
{
    for(int i = 0; i < m_rows.size(); i++)
    {
        DelegationItemWidget* row = m_rows[i];
        m_mainLayout->removeWidget(row);
        row->deleteLater();
    }
    m_rows.clear();
}

void DelegationListWidget::insertRow(const QModelIndex &index, int position)
{
    DelegationItemWidget* item = new DelegationItemWidget(m_platfromStyle);
    if(m_model) item->setModel(m_model);
    if(m_clientModel) item->setClientModel(m_clientModel);
    m_rows.insert(position, item);
    for(DelegationItemWidget* p_row : m_rows)
    {
        if(p_row != item)
        {
            int pos = p_row->position();
            if(pos >= position)
            {
                p_row->setPosition(pos + 1);
            }
        }
    }
    insertItem(position, item);
    updateRow(index, position);
}

DelegationItemWidget *DelegationListWidget::removeRow(int position)
{
    DelegationItemWidget* row =  m_rows[position];
    m_rows.removeAt(position);
    m_mainLayout->removeWidget(row);
    for(DelegationItemWidget* p_row : m_rows)
    {
        int pos = p_row->position();
        if(pos > position)
        {
            p_row->setPosition(pos - 1);
        }
    }
    return row;
}

void DelegationListWidget::on_layoutChanged()
{
    for(int i = 0; i < m_delegationModel->rowCount(); i++)
    {
        updateRow(m_delegationModel->index(i, 0), i);
    }
}

void DelegationListWidget::updateRow(const QModelIndex &index, int position)
{
    if(index.isValid())
    {
        QString fee = m_delegationModel->data(index, DelegationItemModel::FeeRole).toString() + " %";
        QString staker = m_delegationModel->data(index, DelegationItemModel::StakerNameRole).toString();
        QString address = m_delegationModel->data(index, DelegationItemModel::AddressRole).toString();
        int32_t blockHight = m_delegationModel->data(index, DelegationItemModel::BlockHeightRole).toInt();
        int64_t balance = m_delegationModel->data(index, DelegationItemModel::BalanceRole).toLongLong();
        int64_t stake = m_delegationModel->data(index, DelegationItemModel::StakeRole).toLongLong();
        int64_t weight = m_delegationModel->data(index, DelegationItemModel::WeightRole).toLongLong();
        int32_t status = m_delegationModel->data(index, DelegationItemModel::TxStatusRole).toInt();
        DelegationItemWidget* item = m_rows[position];
        item->setPosition(position);
        item->setData(fee, staker, address, blockHight, balance, stake, weight, status);
    }
}

void DelegationListWidget::on_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    for(int i = topLeft.row(); i <= bottomRight.row(); i++)
    {
        updateRow(m_delegationModel->index(i, 0), i);
    }
}

QAbstractItemModel *DelegationListWidget::delegationModel() const
{
    return m_delegationModel;
}

void DelegationListWidget::insertItem(int position, DelegationItemWidget *item)
{
    m_mainLayout->insertWidget(position, item);
    connect(item, SIGNAL(clicked(int, int)), this, SLOT(on_clicked(int, int)));
}

void DelegationListWidget::on_clicked(int position, int button)
{
    QModelIndex index = indexAt(position);
    if(button == DelegationItemWidget::Add)
    {
        Q_EMIT addDelegation();
    }
    else if(button == DelegationItemWidget::Remove)
    {
        Q_EMIT removeDelegation(index);
    }
    else if(button == DelegationItemWidget::Split)
    {
        Q_EMIT splitCoins(index);
    }
    else if(button == DelegationItemWidget::Restore)
    {
        Q_EMIT restoreDelegations();
    }
}

QModelIndex DelegationListWidget::indexAt(const QPoint &p) const
{
    QModelIndex index;
    QWidget* child = childAt(p);
    while(child != 0)
    {
        if(child->inherits("DelegationItemWidget"))
        {
            DelegationItemWidget* item = (DelegationItemWidget*)child;
            index = indexAt(item->position());
            child = 0;
        }
        else
        {
            child = child->parentWidget();
        }
    }

    return index;
}

QModelIndex DelegationListWidget::indexAt(int position) const
{
    QModelIndex index;
    if(position >= 0 && position < m_delegationModel->rowCount())
    {
        index = m_delegationModel->index(position, 0);
    }
    return index;
}
