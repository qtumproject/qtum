#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/nftlistwidget.h>
#include <qt/platformstyle.h>
#include <qt/nftitemwidget.h>
#include <qt/nftitemmodel.h>
#include <qt/walletmodel.h>
#include <qt/bitcoinunits.h>
#include <qt/nftfilterproxy.h>

#include <QAbstractItemModel>

#include <QObject>

NftListWidget::NftListWidget(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    m_mainLayout(new QVBoxLayout(this))
{
    m_platfromStyle = platformStyle;
    m_mainLayout->setSpacing(5);
    m_mainLayout->setContentsMargins(0,0,0,0);
    this->setLayout(m_mainLayout);
    NftItemWidget* item = new NftItemWidget(platformStyle, this, NftItemWidget::New);
    insertItem(0, item);
    m_mainLayout->addStretch();
}

void NftListWidget::setModel(WalletModel *_model)
{
    m_model = _model;
    if(m_model && m_model->getNftItemModel())
    {
        // Sort nfts by name
        NftFilterProxy *proxyModel = new NftFilterProxy(this);
        NftItemModel* nftModel = m_model->getNftItemModel();
        proxyModel->setSourceModel(nftModel);
        proxyModel->sort(0, Qt::AscendingOrder);
        m_nftModel = proxyModel;

        // Connect signals and slots
        connect(m_nftModel, SIGNAL(rowsInserted(QModelIndex,int,int)),this, SLOT(on_rowsInserted(QModelIndex,int,int)));
        connect(m_nftModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),this, SLOT(on_rowsRemoved(QModelIndex,int,int)));
        connect(m_nftModel, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),this, SLOT(on_rowsMoved(QModelIndex,int,int,QModelIndex,int)));
        connect(m_nftModel, SIGNAL(modelReset()),this, SLOT(on_modelReset()));
        connect(m_nftModel, SIGNAL(layoutChanged()), this, SLOT(on_layoutChanged()));
        connect(m_nftModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(on_dataChanged(QModelIndex, QModelIndex)));

        // Add items
        on_rowsInserted(QModelIndex(), 0, m_nftModel->rowCount() - 1);
    }
}

void NftListWidget::on_rowsInserted(const QModelIndex &, int start, int end)
{
    for(int i = start; i <= end; i++)
    {
        insertRow(m_nftModel->index(i, 0), i);
    }
}

void NftListWidget::on_rowsRemoved(const QModelIndex &, int start, int end)
{
    for(int i = end; i >= start; i--)
    {
        NftItemWidget* row = removeRow(i);
        if(row) delete row;
    }
}

void NftListWidget::on_rowsMoved(const QModelIndex &, int start, int end, const QModelIndex &, int row)
{
    QList<NftItemWidget*> movedRows;
    for(int i = end; i >= start; i--)
    {
        NftItemWidget* row = removeRow(i);
        movedRows.prepend(row);
    }

    for(int i = 0; i <movedRows.size(); i++)
    {
        int position = row + i;
        NftItemWidget* item = movedRows[i];
        m_rows.insert(position, item);
        m_mainLayout->insertWidget(position, item);
    }
}

void NftListWidget::on_modelReset()
{
    for(int i = 0; i < m_rows.size(); i++)
    {
        NftItemWidget* row = m_rows[i];
        m_mainLayout->removeWidget(row);
        row->deleteLater();
    }
    m_rows.clear();
}

void NftListWidget::insertRow(const QModelIndex &index, int position)
{
    NftItemWidget* item = new NftItemWidget(m_platfromStyle);
    m_rows.insert(position, item);
    insertItem(position, item);
    updateRow(index, position);
}

NftItemWidget *NftListWidget::removeRow(int position)
{
    NftItemWidget* row =  m_rows[position];
    m_rows.removeAt(position);
    m_mainLayout->removeWidget(row);
    return row;
}

void NftListWidget::on_layoutChanged()
{
    for(int i = 0; i < m_nftModel->rowCount(); i++)
    {
        updateRow(m_nftModel->index(i, 0), i);
    }
}

void NftListWidget::updateRow(const QModelIndex &index, int position)
{
    if(index.isValid())
    {
        QString name = m_nftModel->data(index, NftItemModel::NameRole).toString();
        QString owner = m_nftModel->data(index, NftItemModel::OwnerRole).toString();
        QString balance = m_nftModel->data(index, NftItemModel::BalanceRole).toString();
        QString desc = m_nftModel->data(index, NftItemModel::DescRole).toString();
        QString url = m_nftModel->data(index, NftItemModel::UrlRole).toString();
        NftItemWidget* item = m_rows[position];
        item->setPosition(position);
        item->setData(name, balance, owner, desc, url);

    }
}

void NftListWidget::on_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    for(int i = topLeft.row(); i <= bottomRight.row(); i++)
    {
        updateRow(m_nftModel->index(i, 0), i);
    }
}

QAbstractItemModel *NftListWidget::nftModel() const
{
    return m_nftModel;
}

void NftListWidget::insertItem(int position, NftItemWidget *item)
{
    m_mainLayout->insertWidget(position, item);
    connect(item, SIGNAL(clicked(int, int)), this, SLOT(on_clicked(int, int)));
}

void NftListWidget::on_clicked(int position, int button)
{
    QModelIndex index = indexAt(position);
    if(button == NftItemWidget::Create)
    {
        Q_EMIT createNft();
    }
    else if(button == NftItemWidget::Send)
    {
        Q_EMIT sendNft(index);
    }
}

QModelIndex NftListWidget::indexAt(const QPoint &p) const
{
    QModelIndex index;
    QWidget* child = childAt(p);
    while(child != 0)
    {
        if(child->inherits("NftItemWidget"))
        {
            NftItemWidget* item = (NftItemWidget*)child;
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

QModelIndex NftListWidget::indexAt(int position) const
{
    QModelIndex index;
    if(position >= 0 && position < m_nftModel->rowCount())
    {
        index = m_nftModel->index(position, 0);
    }
    return index;
}
