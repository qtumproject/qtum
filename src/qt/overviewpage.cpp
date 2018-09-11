// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/overviewpage.h>
#include <qt/forms/ui_overviewpage.h>

#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/transactionfilterproxy.h>
#include <qt/transactiontablemodel.h>
#include <qt/walletmodel.h>
#include <qt/tokenitemmodel.h>
#include <interfaces/wallet.h>
#include <qt/transactiondescdialog.h>
#include <qt/styleSheet.h>

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QMessageBox>
#include <QTimer>

#include <QStandardItem>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>

#define NUM_ITEMS 5
#define TOKEN_SIZE 40
#define MARGIN 5
#define SYMBOL_WIDTH 80

#define TX_SIZE 40
#define DECORATION_SIZE 20
#define DATE_WIDTH 110
#define TYPE_WIDTH 140
#define AMOUNT_WIDTH 205

Q_DECLARE_METATYPE(interfaces::WalletBalances)

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    explicit TxViewDelegate(const PlatformStyle *_platformStyle, QObject *parent=nullptr):
        QAbstractItemDelegate(parent), unit(BitcoinUnits::BTC),
        platformStyle(_platformStyle)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();
        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QIcon icon = qvariant_cast<QIcon>(index.data(TransactionTableModel::RawDecorationRole));
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();

        QModelIndex ind = index.model()->index(index.row(), TransactionTableModel::Type, index.parent());
        QString typeString = ind.data(Qt::DisplayRole).toString();

        QRect mainRect = option.rect;
        QColor txColor = index.row() % 2 ? QColor("#393939") : QColor("#2e2e2e");
        painter->fillRect(mainRect, txColor);

        QPen pen;
        pen.setWidth(2);
        pen.setColor(QColor("#009ee5"));
        painter->setPen(pen);
        bool selected = option.state & QStyle::State_Selected;
        if(selected)
        {
            painter->drawRect(mainRect.x()+1, mainRect.y()+1, mainRect.width()-2, mainRect.height()-2);
        }

        QColor foreground("#dedede");
        painter->setPen(foreground);

        QRect dateRect(mainRect.left() + MARGIN, mainRect.top(), DATE_WIDTH, TX_SIZE);
        painter->drawText(dateRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        int topMargin = (TX_SIZE - DECORATION_SIZE) / 2;
        QRect decorationRect(dateRect.topRight() + QPoint(MARGIN, topMargin), QSize(DECORATION_SIZE, DECORATION_SIZE));
        icon.paint(painter, decorationRect);

        QRect typeRect(decorationRect.right() + MARGIN, mainRect.top(), TYPE_WIDTH, TX_SIZE);
        painter->drawText(typeRect, Qt::AlignLeft|Qt::AlignVCenter, typeString);

        bool watchOnly = index.data(TransactionTableModel::WatchonlyRole).toBool();

        if (watchOnly)
        {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            QRect watchonlyRect(typeRect.right() + MARGIN, mainRect.top() + topMargin, DECORATION_SIZE, DECORATION_SIZE);
            iconWatchonly.paint(painter, watchonlyRect);
        }

        int addressMargin = watchOnly ? MARGIN + 20 : MARGIN;
        int addressWidth = mainRect.width() - DATE_WIDTH - DECORATION_SIZE - TYPE_WIDTH - AMOUNT_WIDTH - 5*MARGIN;
        addressWidth = watchOnly ? addressWidth - 20 : addressWidth;

        QFont addressFont = option.font;
        addressFont.setPointSizeF(addressFont.pointSizeF() * 0.95);
        painter->setFont(addressFont);

        QFontMetrics fmName(painter->font());
        QString clippedAddress = fmName.elidedText(address, Qt::ElideRight, addressWidth);

        QRect addressRect(typeRect.right() + addressMargin, mainRect.top(), addressWidth, TX_SIZE);
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, clippedAddress);

        QFont amountFont = option.font;
        amountFont.setBold(true);
        painter->setFont(amountFont);

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = QColor("#ffffff");
        }
        painter->setPen(foreground);

        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true, BitcoinUnits::separatorAlways);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }

        QRect amountRect(addressRect.right() + MARGIN, addressRect.top(), AMOUNT_WIDTH, TX_SIZE);
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(TX_SIZE, TX_SIZE);
    }

    int unit;
    const PlatformStyle *platformStyle;
};

class TknViewDelegate : public QAbstractItemDelegate
{
public:
    TknViewDelegate(const PlatformStyle *_platformStyle, QObject *parent) :
        QAbstractItemDelegate(parent),
        platformStyle(_platformStyle)
    {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const
    {
        painter->save();

        QString tokenSymbol = index.data(TokenItemModel::SymbolRole).toString();
        QString tokenBalance = index.data(TokenItemModel::BalanceRole).toString();

        QRect mainRect = option.rect;
        mainRect.setWidth(option.rect.width());

        painter->fillRect(mainRect, QColor("#383938"));

        QRect hLineRect(mainRect.left(), mainRect.bottom(), mainRect.width(), 1);
        painter->fillRect(hLineRect, QColor("#2e2e2e"));

        QColor foreground("#dedede");
        painter->setPen(foreground);
        
        QFont font = option.font;

        QFontMetrics fmName(font);
        QString clippedSymbol = fmName.elidedText(tokenSymbol, Qt::ElideRight, SYMBOL_WIDTH);

        QRect symbolRect(mainRect.left() + MARGIN, mainRect.top(), SYMBOL_WIDTH, mainRect.height());
        painter->drawText(symbolRect, Qt::AlignLeft|Qt::AlignVCenter, clippedSymbol);

        int balanceWidth = mainRect.width() - symbolRect.width() - 3 * MARGIN;
        QRect balanceRect(symbolRect.right() + MARGIN, symbolRect.top(), balanceWidth, mainRect.height());
        painter->drawText(balanceRect, Qt::AlignRight|Qt::AlignVCenter, tokenBalance);

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QFont font = option.font;

        QString balanceString = index.data(TokenItemModel::BalanceRole).toString();
        QFontMetrics fm(font);
        int balanceWidth = fm.width(balanceString);

        int width = SYMBOL_WIDTH + balanceWidth + 3*MARGIN;
        return QSize(width, TOKEN_SIZE);
    }

    const PlatformStyle *platformStyle;
};
#include <qt/overviewpage.moc>

OverviewPage::OverviewPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    clientModel(0),
    walletModel(0),
    txdelegate(new TxViewDelegate(platformStyle, this)),
    tkndelegate(new TknViewDelegate(platformStyle, this))
{
    ui->setupUi(this);

    // Set stylesheet
    SetObjectStyleSheet(ui->labelWalletStatus, StyleSheetNames::ButtonTransparent);
    SetObjectStyleSheet(ui->labelTokenStatus, StyleSheetNames::ButtonTransparent);
    SetObjectStyleSheet(ui->labelTransactionsStatus, StyleSheetNames::ButtonTransparent);

    m_balances.balance = -1;

    if (!platformStyle->getImagesOnButtons()) {
        ui->buttonAddToken->setIcon(QIcon());
    } else {
        ui->buttonAddToken->setIcon(platformStyle->MultiStatesIcon(":/icons/add", PlatformStyle::PushButton));
    }


    // use a MultiStatesIcon for the "out of sync warning" icon
    QIcon icon = platformStyle->MultiStatesIcon(":/icons/warning", PlatformStyle::PushButton);

    ui->labelTransactionsStatus->setIcon(icon);
    ui->labelWalletStatus->setIcon(icon);
    ui->labelTokenStatus->setIcon(icon);

    QFont font = ui->labelTotal->font();
    font.setPointSizeF(font.pointSizeF() * 1.5);
    ui->labelTotal->setFont(font);

    QFont fontWatch = ui->labelWatchTotal->font();
    fontWatch.setPointSizeF(fontWatch.pointSizeF() * 1.5);
    ui->labelWatchTotal->setFont(fontWatch);

    ui->labelDate->setFixedWidth(DATE_WIDTH);
    ui->labelType->setFixedWidth(TYPE_WIDTH);
    ui->labelAmount->setFixedWidth(AMOUNT_WIDTH);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (TX_SIZE + 2));
    ui->listTransactions->setMinimumWidth(590);
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->listTransactions->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(ui->listTransactions, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(showDetails()));

    // Token list
    ui->listTokens->setItemDelegate(tkndelegate);
    ui->listTokens->setAttribute(Qt::WA_MacShowFocusRect, false);

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
    connect(ui->labelWalletStatus, SIGNAL(clicked()), this, SLOT(handleOutOfSyncWarningClicks()));
    connect(ui->labelTokenStatus, SIGNAL(clicked()), this, SLOT(handleOutOfSyncWarningClicks()));
    connect(ui->labelTransactionsStatus, SIGNAL(clicked()), this, SLOT(handleOutOfSyncWarningClicks()));
}

void OverviewPage::handleOutOfSyncWarningClicks()
{
    Q_EMIT outOfSyncWarningClicked();
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setBalance(const interfaces::WalletBalances& balances)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    m_balances = balances;
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balances.balance, false, BitcoinUnits::separatorAlways));
    ui->labelUnconfirmed->setText(BitcoinUnits::formatWithUnit(unit, balances.unconfirmed_balance, false, BitcoinUnits::separatorAlways));
    ui->labelImmature->setText(BitcoinUnits::formatWithUnit(unit, balances.immature_balance, false, BitcoinUnits::separatorAlways));
    ui->labelStake->setText(BitcoinUnits::formatWithUnit(unit, balances.stake, false, BitcoinUnits::separatorAlways));
    ui->labelTotal->setText(BitcoinUnits::formatWithUnit(unit, balances.balance + balances.unconfirmed_balance + balances.immature_balance + balances.stake, false, BitcoinUnits::separatorAlways));
    ui->labelWatchAvailable->setText(BitcoinUnits::formatWithUnit(unit, balances.watch_only_balance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchPending->setText(BitcoinUnits::formatWithUnit(unit, balances.unconfirmed_watch_only_balance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchImmature->setText(BitcoinUnits::formatWithUnit(unit, balances.immature_watch_only_balance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchStake->setText(BitcoinUnits::formatWithUnit(unit, balances.watch_only_stake, false, BitcoinUnits::separatorAlways));
    ui->labelWatchTotal->setText(BitcoinUnits::formatWithUnit(unit, balances.watch_only_balance + balances.unconfirmed_watch_only_balance + balances.immature_watch_only_balance + balances.watch_only_stake, false, BitcoinUnits::separatorAlways));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = balances.immature_balance != 0;
    bool showStake = balances.stake != 0;
    bool showWatchOnlyImmature = balances.immature_watch_only_balance != 0;
    bool showWatchOnlyStake = balances.watch_only_stake != 0;

    // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelWatchImmature->setVisible(showWatchOnlyImmature); // show watch-only immature balance
    ui->labelStake->setVisible(showStake || showWatchOnlyStake);
    ui->labelStakeText->setVisible(showStake || showWatchOnlyStake);
    ui->labelWatchStake->setVisible(showWatchOnlyStake); // show watch-only stake balance
}

void OverviewPage::checkForInvalidTokens()
{
    if(walletModel)
    {
        std::vector<interfaces::TokenInfo> invalidTokens = walletModel->wallet().getInvalidTokens();
        if(invalidTokens.size() > 0)
        {
            QString message;
            for(interfaces::TokenInfo& tokenInfo : invalidTokens)
            {
                QString symbol = QString::fromStdString(tokenInfo.token_symbol);
                QString address = QString::fromStdString(tokenInfo.sender_address);
                message += tr("The %1 address \"%2\" is not yours, please change it to new one.\n").arg(symbol, address);
            }
            QMessageBox::warning(this, tr("Invalid token address"), message);
        }
    }
}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{
    ui->labelSpendable->setVisible(showWatchOnly);      // show spendable label (only when watch-only is active)
    ui->labelWatchonly->setVisible(showWatchOnly);      // show watch-only label
    ui->labelWatchAvailable->setVisible(showWatchOnly); // show watch-only available balance
    ui->labelWatchPending->setVisible(showWatchOnly);   // show watch-only pending balance
    ui->labelWatchTotal->setVisible(showWatchOnly);     // show watch-only total balance

    if (!showWatchOnly)
    {
        ui->labelWatchImmature->hide();
        ui->labelWatchStake->hide();
    }
}

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
        // Show warning if this is a prerelease version
        connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
        updateAlerts(model->getStatusBarWarnings());
    }
}

void OverviewPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter.reset(new TransactionFilterProxy());
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter.get());
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        interfaces::Wallet& wallet = model->wallet();
        interfaces::WalletBalances balances = wallet.getBalances();
        setBalance(balances);
        connect(model, SIGNAL(balanceChanged(interfaces::WalletBalances)), this, SLOT(setBalance(interfaces::WalletBalances)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        updateWatchOnlyLabels(wallet.haveWatchOnly());
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));
    }

    if(model && model->getTokenItemModel())
    {
        // Sort tokens by name
        QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
        TokenItemModel* tokenModel = model->getTokenItemModel();
        proxyModel->setSourceModel(tokenModel);
        proxyModel->sort(0, Qt::AscendingOrder);

        // Set tokens model
        ui->listTokens->setModel(proxyModel);
    }

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();

    // check for presence of invalid tokens
    QTimer::singleShot(500, this, SLOT(checkForInvalidTokens()));
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if (m_balances.balance != -1) {
            setBalance(m_balances);
        }

        // Update txdelegate->unit with the current unit
        txdelegate->unit = walletModel->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();
    }
}

void OverviewPage::updateAlerts(const QString &warnings)
{
    this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
    ui->labelTokenStatus->setVisible(fShow);
}

void OverviewPage::on_buttonAddToken_clicked()
{
    Q_EMIT addTokenClicked();
}

void OverviewPage::on_showMoreButton_clicked()
{
    Q_EMIT showMoreClicked();
}

void OverviewPage::showDetails()
{
    if(!ui->listTransactions->selectionModel())
        return;
    QModelIndexList selection = ui->listTransactions->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        TransactionDescDialog *dlg = new TransactionDescDialog(selection.at(0));
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    }
}