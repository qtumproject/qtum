#include <qt/qrctoken.h>
#include <qt/forms/ui_qrctoken.h>
#include <qt/tokenitemmodel.h>
#include <qt/walletmodel.h>
#include <qt/tokentransactionview.h>
#include <qt/platformstyle.h>
#include <qt/styleSheet.h>
#include <qt/tokenlistwidget.h>

#include <QPainter>
#include <QAbstractItemDelegate>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QSizePolicy>
#include <QMenu>

#define TOKEN_SIZE 54
#define SYMBOL_WIDTH 60
#define MARGIN 5

class TokenViewDelegate : public QAbstractItemDelegate
{
public:

    TokenViewDelegate(const PlatformStyle *_platformStyle, QObject *parent) :
        QAbstractItemDelegate(parent),
        platformStyle(_platformStyle)
    {
        token_size = GetIntStyleValue("tokenviewdelegate/token-size", TOKEN_SIZE);
        symbol_width = GetIntStyleValue("tokenviewdelegate/symbol-width", SYMBOL_WIDTH);
        margin = GetIntStyleValue("tokenviewdelegate/margin", MARGIN);
        background_color_selected = GetStringStyleValue("tokenviewdelegate/background-color-selected", "#009ee5");
        background_color = GetStringStyleValue("tokenviewdelegate/background-color", "#383938");
        hline_color = GetStringStyleValue("tokenviewdelegate/hline-color", "#2e2e2e");
        foreground_color = GetStringStyleValue("tokenviewdelegate/foreground-color", "#dddddd");
        amount_color = GetStringStyleValue("tokenviewdelegate/amount-color", "#ffffff");
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const
    {
        painter->save();

        QString tokenSymbol = index.data(TokenItemModel::SymbolRole).toString();
        QString tokenBalance = index.data(TokenItemModel::BalanceRole).toString();
        QString receiveAddress = index.data(TokenItemModel::SenderRole).toString();

        QRect mainRect = option.rect;

        bool selected = option.state & QStyle::State_Selected;
        if(selected)
        {
            painter->fillRect(mainRect, background_color_selected);
        }
        else
        {
            painter->fillRect(mainRect, background_color);
        }

        QRect hLineRect(mainRect.left(), mainRect.bottom(), mainRect.width(), 1);
        painter->fillRect(hLineRect, hline_color);

        QColor foreground = foreground_color;
        painter->setPen(foreground);

        QFont font = option.font;
        font.setPointSizeF(option.font.pointSizeF() * 1.1);
        font.setBold(true);
        painter->setFont(font);
        QColor amountColor = amount_color;
        painter->setPen(amountColor);

        QFontMetrics fmName(option.font);
        QString clippedSymbol = fmName.elidedText(tokenSymbol, Qt::ElideRight, symbol_width);
        QRect tokenSymbolRect(mainRect.left() + margin, mainRect.top() + margin, symbol_width, mainRect.height() / 2 - margin);
        painter->drawText(tokenSymbolRect, Qt::AlignLeft|Qt::AlignVCenter, clippedSymbol);

        int amountWidth = (mainRect.width() - 4 * margin - tokenSymbolRect.width());
        QFontMetrics fmAmount(font);
        QString clippedAmount = fmAmount.elidedText(tokenBalance, Qt::ElideRight, amountWidth);
        QRect tokenBalanceRect(tokenSymbolRect.right() + 2 * margin, tokenSymbolRect.top(), amountWidth, tokenSymbolRect.height());
        painter->drawText(tokenBalanceRect, Qt::AlignLeft|Qt::AlignVCenter, clippedAmount);

        QFont addressFont = option.font;
        addressFont.setPointSizeF(option.font.pointSizeF() * 0.8);
        painter->setFont(addressFont);
        painter->setPen(foreground);
        QRect receiveAddressRect(mainRect.left() + margin, tokenSymbolRect.bottom(), mainRect.width() - 2 * margin, mainRect.height() / 2 - 2 * margin);
        painter->drawText(receiveAddressRect, Qt::AlignLeft|Qt::AlignVCenter, receiveAddress);

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(token_size, token_size);
    }

    const PlatformStyle *platformStyle;

private:
    int token_size;
    int symbol_width;
    int margin;
    QColor background_color_selected;
    QColor background_color;
    QColor hline_color;
    QColor foreground_color;
    QColor amount_color;
};

QRCToken::QRCToken(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QRCToken),
    m_model(0),
    m_clientModel(0),
    m_tokenModel(0),
    m_tokenDelegate(0),
    m_tokenTransactionView(0)
{
    ui->setupUi(this);

    m_platformStyle = platformStyle;

    m_sendTokenPage = new SendTokenPage(this);
    m_receiveTokenPage = new ReceiveTokenPage(platformStyle, this);
    m_addTokenPage = new AddTokenPage(this);
    m_tokenDelegate = new TokenViewDelegate(platformStyle, this);

    m_sendTokenPage->setEnabled(false);
    m_receiveTokenPage->setEnabled(false);

    m_tokenTransactionView = new TokenTransactionView(m_platformStyle, this);
    m_tokenTransactionView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->tokenViewLayout->addWidget(m_tokenTransactionView);

    ui->tokensList->setItemDelegate(m_tokenDelegate);
    ui->tokensList->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tokensList->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->tokensList->hide();

    QAction *copySenderAction = new QAction(tr("Copy receive address"), this);
    QAction *copyTokenBalanceAction = new QAction(tr("Copy token balance"), this);
    QAction *copyTokenNameAction = new QAction(tr("Copy token name"), this);
    QAction *copyTokenAddressAction = new QAction(tr("Copy contract address"), this);
    QAction *removeTokenAction = new QAction(tr("Remove token"), this);

    m_tokenList = new TokenListWidget(platformStyle, this);
    m_tokenList->setContextMenuPolicy(Qt::CustomContextMenu);
    new QVBoxLayout(ui->scrollArea);
    ui->scrollArea->setWidget(m_tokenList);
    ui->scrollArea->setWidgetResizable(true);
    connect(m_tokenList, &TokenListWidget::sendToken, this, &QRCToken::on_sendToken);
    connect(m_tokenList, &TokenListWidget::receiveToken, this, &QRCToken::on_receiveToken);
    connect(m_tokenList, &TokenListWidget::addToken, this, &QRCToken::on_addToken);

    contextMenu = new QMenu(m_tokenList);
    contextMenu->addAction(copySenderAction);
    contextMenu->addAction(copyTokenBalanceAction);
    contextMenu->addAction(copyTokenNameAction);
    contextMenu->addAction(copyTokenAddressAction);
    contextMenu->addAction(removeTokenAction);

    connect(copyTokenAddressAction, &QAction::triggered, this, &QRCToken::copyTokenAddress);
    connect(copyTokenBalanceAction, &QAction::triggered, this, &QRCToken::copyTokenBalance);
    connect(copyTokenNameAction, &QAction::triggered, this, &QRCToken::copyTokenName);
    connect(copySenderAction, &QAction::triggered, this, &QRCToken::copySenderAddress);
    connect(removeTokenAction, &QAction::triggered, this, &QRCToken::removeToken);

    connect(ui->tokensList, &QListView::clicked, this, &QRCToken::on_currentTokenChanged);
    connect(m_tokenList, &TokenListWidget::customContextMenuRequested, this, &QRCToken::contextualMenu);
}

QRCToken::~QRCToken()
{
    delete ui;
}

void QRCToken::setModel(WalletModel *_model)
{
    m_model = _model;
    m_addTokenPage->setModel(m_model);
    m_sendTokenPage->setModel(m_model);
    m_tokenList->setModel(m_model);
    m_tokenTransactionView->setModel(_model);
    if(m_model && m_model->getTokenItemModel())
    {
        // Sort tokens by symbol
        QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
        TokenItemModel* tokenModel = m_model->getTokenItemModel();
        proxyModel->setSourceModel(tokenModel);
        proxyModel->sort(1, Qt::AscendingOrder);
        m_tokenModel = proxyModel;

        // Set tokens model
        ui->tokensList->setModel(m_tokenList->tokenModel());
        connect(ui->tokensList->selectionModel(), &QItemSelectionModel::currentChanged, this, &QRCToken::on_currentChanged);

        // Set current token
        connect(m_tokenList->tokenModel(), &QAbstractItemModel::dataChanged, this, &QRCToken::on_dataChanged);
        connect(m_tokenList->tokenModel(), &QAbstractItemModel::rowsInserted, this, &QRCToken::on_rowsInserted);
        if(m_tokenList->tokenModel()->rowCount() > 0)
        {
            QModelIndex currentToken(m_tokenList->tokenModel()->index(0, 0));
            ui->tokensList->setCurrentIndex(currentToken);
            on_currentTokenChanged(currentToken);
        }
    }
}

void QRCToken::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;
    m_sendTokenPage->setClientModel(_clientModel);
    m_addTokenPage->setClientModel(_clientModel);
}

void QRCToken::on_goToSendTokenPage()
{
    m_sendTokenPage->show();
}

void QRCToken::on_goToReceiveTokenPage()
{
    m_receiveTokenPage->show();
}

void QRCToken::on_goToAddTokenPage()
{
    m_addTokenPage->show();
}

void QRCToken::on_currentTokenChanged(QModelIndex index)
{
    if(m_tokenList->tokenModel())
    {
        if(index.isValid())
        {
            m_selectedTokenHash = m_tokenList->tokenModel()->data(index, TokenItemModel::HashRole).toString();
            std::string address = m_tokenList->tokenModel()->data(index, TokenItemModel::AddressRole).toString().toStdString();
            std::string symbol = m_tokenList->tokenModel()->data(index, TokenItemModel::SymbolRole).toString().toStdString();
            std::string sender = m_tokenList->tokenModel()->data(index, TokenItemModel::SenderRole).toString().toStdString();
            int8_t decimals = m_tokenList->tokenModel()->data(index, TokenItemModel::DecimalsRole).toInt();
            std::string balance = m_tokenList->tokenModel()->data(index, TokenItemModel::RawBalanceRole).toString().toStdString();
            m_sendTokenPage->setTokenData(address, sender, symbol, decimals, balance);
            m_receiveTokenPage->setAddress(QString::fromStdString(sender));
            m_receiveTokenPage->setSymbol(QString::fromStdString(symbol));

            if(!m_sendTokenPage->isEnabled())
                m_sendTokenPage->setEnabled(true);
            if(!m_receiveTokenPage->isEnabled())
                m_receiveTokenPage->setEnabled(true);
        }
        else
        {
            m_sendTokenPage->setEnabled(false);
            m_receiveTokenPage->setEnabled(false);
            m_receiveTokenPage->setAddress(QString::fromStdString(""));
            m_receiveTokenPage->setSymbol(QString::fromStdString(""));
        }
    }
}

void QRCToken::on_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    Q_UNUSED(bottomRight);
    Q_UNUSED(roles);

    if(m_tokenList->tokenModel())
    {
        QString tokenHash = m_tokenList->tokenModel()->data(topLeft, TokenItemModel::HashRole).toString();
        if(m_selectedTokenHash.isEmpty() ||
                tokenHash == m_selectedTokenHash)
        {
            on_currentTokenChanged(topLeft);
        }
    }
}

void QRCToken::on_currentChanged(QModelIndex current, QModelIndex previous)
{
    Q_UNUSED(previous);

    on_currentTokenChanged(current);
}

void QRCToken::on_rowsInserted(QModelIndex index, int first, int last)
{
    Q_UNUSED(index);
    Q_UNUSED(first);
    Q_UNUSED(last);

    if(m_tokenList->tokenModel()->rowCount() == 1)
    {
        QModelIndex currentToken(m_tokenList->tokenModel()->index(0, 0));
        ui->tokensList->setCurrentIndex(currentToken);
        on_currentTokenChanged(currentToken);
    }
}

void QRCToken::contextualMenu(const QPoint &point)
{
    QModelIndex index = m_tokenList->indexAt(point);
    if(index.isValid())
    {
        indexMenu = index;
        contextMenu->exec(QCursor::pos());
    }
}

void QRCToken::copyTokenAddress()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(TokenItemModel::AddressRole).toString());
        indexMenu = QModelIndex();
    }
}

void QRCToken::copyTokenBalance()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(TokenItemModel::BalanceRole).toString());
        indexMenu = QModelIndex();
    }
}

void QRCToken::copyTokenName()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(TokenItemModel::NameRole).toString());
        indexMenu = QModelIndex();
    }
}

void QRCToken::copySenderAddress()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(TokenItemModel::SenderRole).toString());
        indexMenu = QModelIndex();
    }
}

void QRCToken::removeToken()
{
    QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, tr("Confirm token remove"), tr("The selected token will be removed from the list. Are you sure?"),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

    if(btnRetVal == QMessageBox::Yes)
    {
        QModelIndex index = indexMenu;
        std::string sHash = index.data(TokenItemModel::HashRole).toString().toStdString();
        m_model->wallet().removeTokenEntry(sHash);
        indexMenu = QModelIndex();
    }
}

void QRCToken::on_sendToken(const QModelIndex &index)
{
    on_currentTokenChanged(index);
    on_goToSendTokenPage();
}

void QRCToken::on_receiveToken(const QModelIndex &index)
{
    on_currentTokenChanged(index);
    on_goToReceiveTokenPage();
}

void QRCToken::on_addToken()
{
    on_goToAddTokenPage();
}
