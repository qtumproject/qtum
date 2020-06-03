#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/superstakerlistwidget.h>
#include <qt/platformstyle.h>
#include <qt/superstakeritemwidget.h>
#include <qt/superstakeritemmodel.h>
#include <qt/walletmodel.h>
#include <qt/clientmodel.h>

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include <QObject>

SuperStakerListWidget::SuperStakerListWidget(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    m_mainLayout(new QVBoxLayout(this)),
    m_model(0),
    m_clientModel(0)
{
    m_platfromStyle = platformStyle;
    m_mainLayout->setSpacing(5);
    m_mainLayout->setContentsMargins(0,0,0,0);
    this->setLayout(m_mainLayout);
    SuperStakerItemWidget* item = new SuperStakerItemWidget(platformStyle, this, SuperStakerItemWidget::New);
    insertItem(0, item);
    m_mainLayout->addStretch();
}

void SuperStakerListWidget::setModel(WalletModel *_model)
{
    m_model = _model;
    if(m_model && m_model->getSuperStakerItemModel())
    {
        // Sort super stakers
        QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
        SuperStakerItemModel* superStakerModel = m_model->getSuperStakerItemModel();
        proxyModel->setSourceModel(superStakerModel);
        proxyModel->sort(4, Qt::AscendingOrder);
        m_superStakerModel = proxyModel;

        // Connect signals and slots
        connect(m_superStakerModel, SIGNAL(rowsInserted(QModelIndex,int,int)),this, SLOT(on_rowsInserted(QModelIndex,int,int)));
        connect(m_superStakerModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),this, SLOT(on_rowsRemoved(QModelIndex,int,int)));
        connect(m_superStakerModel, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),this, SLOT(on_rowsMoved(QModelIndex,int,int,QModelIndex,int)));
        connect(m_superStakerModel, SIGNAL(modelReset()),this, SLOT(on_modelReset()));
        connect(m_superStakerModel, SIGNAL(layoutChanged()), this, SLOT(on_layoutChanged()));
        connect(m_superStakerModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(on_dataChanged(QModelIndex, QModelIndex)));

        // Add items
        on_rowsInserted(QModelIndex(), 0, m_superStakerModel->rowCount() - 1);
    }
}

void SuperStakerListWidget::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;
}

void SuperStakerListWidget::on_rowsInserted(const QModelIndex &, int start, int end)
{
    for(int i = start; i <= end; i++)
    {
        insertRow(m_superStakerModel->index(i, 0), i);
    }
}

void SuperStakerListWidget::on_rowsRemoved(const QModelIndex &, int start, int end)
{
    for(int i = end; i >= start; i--)
    {
        SuperStakerItemWidget* row = removeRow(i);
        if(row) delete row;
    }
}

void SuperStakerListWidget::on_rowsMoved(const QModelIndex &, int start, int end, const QModelIndex &, int row)
{
    QList<SuperStakerItemWidget*> movedRows;
    for(int i = end; i >= start; i--)
    {
        SuperStakerItemWidget* row = removeRow(i);
        movedRows.prepend(row);
    }

    for(int i = 0; i <movedRows.size(); i++)
    {
        int position = row + i;
        SuperStakerItemWidget* item = movedRows[i];
        m_rows.insert(position, item);
        m_mainLayout->insertWidget(position, item);
    }
}

void SuperStakerListWidget::on_modelReset()
{
    for(int i = 0; i < m_rows.size(); i++)
    {
        SuperStakerItemWidget* row = m_rows[i];
        m_mainLayout->removeWidget(row);
        row->deleteLater();
    }
    m_rows.clear();
}

void SuperStakerListWidget::insertRow(const QModelIndex &index, int position)
{
    SuperStakerItemWidget* item = new SuperStakerItemWidget(m_platfromStyle);
    if(m_model) item->setModel(m_model);
    if(m_clientModel) item->setClientModel(m_clientModel);
    m_rows.insert(position, item);
    for(SuperStakerItemWidget* p_row : m_rows)
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

SuperStakerItemWidget *SuperStakerListWidget::removeRow(int position)
{
    SuperStakerItemWidget* row =  m_rows[position];
    m_rows.removeAt(position);
    m_mainLayout->removeWidget(row);
    for(SuperStakerItemWidget* p_row : m_rows)
    {
        int pos = p_row->position();
        if(pos > position)
        {
            p_row->setPosition(pos - 1);
        }
    }
    return row;
}

void SuperStakerListWidget::on_layoutChanged()
{
    for(int i = 0; i < m_superStakerModel->rowCount(); i++)
    {
        updateRow(m_superStakerModel->index(i, 0), i);
    }
}

void SuperStakerListWidget::updateRow(const QModelIndex &index, int position)
{
    if(index.isValid())
    {
        QString minFee = m_superStakerModel->data(index, SuperStakerItemModel::MinFeeRole).toString() + " %";
        QString staker = m_superStakerModel->data(index, SuperStakerItemModel::StakerNameRole).toString();
        QString address = m_superStakerModel->data(index, SuperStakerItemModel::StakerAddressRole).toString();
        bool staking_on = m_superStakerModel->data(index, SuperStakerItemModel::StakingRole).toBool();
        int64_t balance = m_superStakerModel->data(index, SuperStakerItemModel::BalanceRole).toLongLong();
        int64_t stake = m_superStakerModel->data(index, SuperStakerItemModel::StakeRole).toLongLong();
        int64_t weight = m_superStakerModel->data(index, SuperStakerItemModel::WeightRole).toLongLong();
        int64_t delegationsWeight = m_superStakerModel->data(index, SuperStakerItemModel::DelegationsWeightRole).toLongLong();
        SuperStakerItemWidget* item = m_rows[position];
        item->setPosition(position);
        item->setData(minFee, staker, address, staking_on, balance, stake, weight, delegationsWeight);
    }
}

void SuperStakerListWidget::on_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    for(int i = topLeft.row(); i <= bottomRight.row(); i++)
    {
        updateRow(m_superStakerModel->index(i, 0), i);
    }
}

QAbstractItemModel *SuperStakerListWidget::superStakerModel() const
{
    return m_superStakerModel;
}

void SuperStakerListWidget::insertItem(int position, SuperStakerItemWidget *item)
{
    m_mainLayout->insertWidget(position, item);
    connect(item, SIGNAL(clicked(int, int)), this, SLOT(on_clicked(int, int)));
}

void SuperStakerListWidget::on_clicked(int position, int button)
{
    QModelIndex index = indexAt(position);
    if(button == SuperStakerItemWidget::Add)
    {
        Q_EMIT addSuperStaker();
    }
    else if(button == SuperStakerItemWidget::Remove)
    {
        Q_EMIT removeSuperStaker(index);
    }
    else if(button == SuperStakerItemWidget::Config)
    {
        Q_EMIT configSuperStaker(index);
    }
    else if(button == SuperStakerItemWidget::Delegations)
    {
        Q_EMIT delegationsSuperStaker(index);
    }
    else if(button == SuperStakerItemWidget::Split)
    {
        Q_EMIT splitCoins(index);
    }
    else if(button == SuperStakerItemWidget::Restore)
    {
        Q_EMIT restoreSuperStakers();
    }
}

QModelIndex SuperStakerListWidget::indexAt(const QPoint &p) const
{
    QModelIndex index;
    QWidget* child = childAt(p);
    while(child != 0)
    {
        if(child->inherits("SuperStakerItemWidget"))
        {
            SuperStakerItemWidget* item = (SuperStakerItemWidget*)child;
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

QModelIndex SuperStakerListWidget::indexAt(int position) const
{
    QModelIndex index;
    if(position >= 0 && position < m_superStakerModel->rowCount())
    {
        index = m_superStakerModel->index(position, 0);
    }
    return index;
}
