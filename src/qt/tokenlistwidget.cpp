#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/tokenlistwidget.h>
#include <qt/platformstyle.h>
#include <qt/tokenitemwidget.h>
#include <qt/tokenitemmodel.h>
#include <qt/walletmodel.h>
#include <qt/bitcoinunits.h>

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include <QObject>

static const QString TOKEN_ICON_FORMAT = ":/tokens/%1";

TokenListWidget::TokenListWidget(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    m_mainLayout(new QVBoxLayout(this))
{
    m_platfromStyle = platformStyle;
    m_mainLayout->setSpacing(5);
    m_mainLayout->setContentsMargins(0,0,0,0);
    this->setLayout(m_mainLayout);
    TokenItemWidget* item = new TokenItemWidget(platformStyle, this, TokenItemWidget::New);
    insertItem(0, item);
    m_mainLayout->addStretch();
}

void TokenListWidget::setModel(WalletModel *_model)
{
    m_model = _model;
    if(m_model && m_model->getTokenItemModel())
    {
        // Sort tokens by symbol
        QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
        TokenItemModel* tokenModel = m_model->getTokenItemModel();
        proxyModel->setSourceModel(tokenModel);
        proxyModel->sort(1, Qt::AscendingOrder);
        m_tokenModel = proxyModel;

        // Connect signals and slots
        connect(m_tokenModel, SIGNAL(rowsInserted(QModelIndex,int,int)),this, SLOT(on_rowsInserted(QModelIndex,int,int)));
        connect(m_tokenModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),this, SLOT(on_rowsRemoved(QModelIndex,int,int)));
        connect(m_tokenModel, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),this, SLOT(on_rowsMoved(QModelIndex,int,int,QModelIndex,int)));
        connect(m_tokenModel, SIGNAL(modelReset()),this, SLOT(on_modelReset()));
        connect(m_tokenModel, SIGNAL(layoutChanged()), this, SLOT(on_layoutChanged()));
        connect(m_tokenModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(on_dataChanged(QModelIndex, QModelIndex)));

        // Add items
        on_rowsInserted(QModelIndex(), 0, m_tokenModel->rowCount() - 1);
    }
}

void TokenListWidget::on_rowsInserted(const QModelIndex &, int start, int end)
{
    for(int i = start; i <= end; i++)
    {
        insertRow(m_tokenModel->index(i, 0), i);
    }
}

void TokenListWidget::on_rowsRemoved(const QModelIndex &, int start, int end)
{
    for(int i = end; i >= start; i--)
    {
        TokenItemWidget* row = removeRow(i);
        if(row) delete row;
    }
}

void TokenListWidget::on_rowsMoved(const QModelIndex &, int start, int end, const QModelIndex &, int row)
{
    QList<TokenItemWidget*> movedRows;
    for(int i = end; i >= start; i--)
    {
        TokenItemWidget* row = removeRow(i);
        movedRows.prepend(row);
    }

    for(int i = 0; i <movedRows.size(); i++)
    {
        int position = row + i;
        TokenItemWidget* item = movedRows[i];
        m_rows.insert(position, item);
        m_mainLayout->insertWidget(position, item);
    }
}

void TokenListWidget::on_modelReset()
{
    for(int i = 0; i < m_rows.size(); i++)
    {
        TokenItemWidget* row = m_rows[i];
        m_mainLayout->removeWidget(row);
        row->deleteLater();
    }
    m_rows.clear();
}

void TokenListWidget::insertRow(const QModelIndex &index, int position)
{
    TokenItemWidget* item = new TokenItemWidget(m_platfromStyle);
    m_rows.insert(position, item);
    insertItem(position, item);
    updateRow(index, position);
}

TokenItemWidget *TokenListWidget::removeRow(int position)
{
    TokenItemWidget* row =  m_rows[position];
    m_rows.removeAt(position);
    m_mainLayout->removeWidget(row);
    return row;
}

void TokenListWidget::on_layoutChanged()
{
    for(int i = 0; i < m_tokenModel->rowCount(); i++)
    {
        updateRow(m_tokenModel->index(i, 0), i);
    }
}

void TokenListWidget::updateRow(const QModelIndex &index, int position)
{
    if(index.isValid())
    {
        std::string name = m_tokenModel->data(index, TokenItemModel::NameRole).toString().toStdString();
        std::string symbol = m_tokenModel->data(index, TokenItemModel::SymbolRole).toString().toStdString();
        std::string sender = m_tokenModel->data(index, TokenItemModel::SenderRole).toString().toStdString();
        int8_t decimals = m_tokenModel->data(index, TokenItemModel::DecimalsRole).toInt();
        std::string balance = m_tokenModel->data(index, TokenItemModel::RawBalanceRole).toString().toStdString();
        std::string address = m_tokenModel->data(index, TokenItemModel::AddressRole).toString().toStdString();
        QString tokenIconPath = TOKEN_ICON_FORMAT.arg(QString::fromStdString(address));
        int256_t totalSupply(balance);
        TokenItemWidget* item = m_rows[position];
        item->setPosition(position);
        item->setData(QString::fromStdString(name), BitcoinUnits::formatTokenWithUnit(QString::fromStdString(symbol), decimals, totalSupply, false, BitcoinUnits::separatorAlways), QString::fromStdString(sender), tokenIconPath);

    }
}

void TokenListWidget::on_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    for(int i = topLeft.row(); i <= bottomRight.row(); i++)
    {
        updateRow(m_tokenModel->index(i, 0), i);
    }
}

QAbstractItemModel *TokenListWidget::tokenModel() const
{
    return m_tokenModel;
}

void TokenListWidget::insertItem(int position, TokenItemWidget *item)
{
    m_mainLayout->insertWidget(position, item);
    connect(item, SIGNAL(clicked(int, int)), this, SLOT(on_clicked(int, int)));
}

void TokenListWidget::on_clicked(int position, int button)
{
    QModelIndex index = indexAt(position);
    if(button == TokenItemWidget::Add)
    {
        Q_EMIT addToken();
    }
    else if(button == TokenItemWidget::Send)
    {
        Q_EMIT sendToken(index);
    }
    else if(button == TokenItemWidget::Receive)
    {
        Q_EMIT receiveToken(index);
    }
}

QModelIndex TokenListWidget::indexAt(const QPoint &p) const
{
    QModelIndex index;
    QWidget* child = childAt(p);
    while(child != 0)
    {
        if(child->inherits("TokenItemWidget"))
        {
            TokenItemWidget* item = (TokenItemWidget*)child;
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

QModelIndex TokenListWidget::indexAt(int position) const
{
    QModelIndex index;
    if(position >= 0 && position < m_tokenModel->rowCount())
    {
        index = m_tokenModel->index(position, 0);
    }
    return index;
}
