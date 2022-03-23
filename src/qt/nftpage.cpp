#include <qt/nftpage.h>
#include <qt/forms/ui_nftpage.h>
#include <qt/nftitemmodel.h>
#include <qt/walletmodel.h>
#include <qt/nfttransactionview.h>
#include <qt/platformstyle.h>
#include <qt/styleSheet.h>
#include <qt/nftlistwidget.h>
#include <qt/guiutil.h>
#include <validation.h>

#include <QPainter>
#include <QAbstractItemDelegate>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QSizePolicy>
#include <QMenu>

NftPage::NftPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NftPage),
    m_model(0),
    m_clientModel(0),
    m_nftTransactionView(0)
{
    ui->setupUi(this);

    m_platformStyle = platformStyle;

    m_sendNftDialog = new SendNftDialog(this);
    m_createNftDialog = new CreateNftDialog(this);

    m_sendNftDialog->setEnabled(false);

    m_nftTransactionView = new NftTransactionView(m_platformStyle, this);
    m_nftTransactionView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->nftViewLayout->addWidget(m_nftTransactionView);

    QAction *copyOwnerAction = new QAction(tr("Copy owner address"), this);
    QAction *copyBalanceAction = new QAction(tr("Copy NFT balance"), this);
    QAction *copyNameAction = new QAction(tr("Copy NFT name"), this);
    QAction *copyUrlAction = new QAction(tr("Copy NFT URL"), this);
    QAction *copyDescAction = new QAction(tr("Copy NFT description"), this);

    m_nftList = new NftListWidget(platformStyle, this);
    m_nftList->setContextMenuPolicy(Qt::CustomContextMenu);
    new QVBoxLayout(ui->scrollArea);
    ui->scrollArea->setWidget(m_nftList);
    ui->scrollArea->setWidgetResizable(true);
    connect(m_nftList, &NftListWidget::sendNft, this, &NftPage::on_sendNft);
    connect(m_nftList, &NftListWidget::createNft, this, &NftPage::on_createNft);

    contextMenu = new QMenu(m_nftList);
    contextMenu->addAction(copyOwnerAction);
    contextMenu->addAction(copyBalanceAction);
    contextMenu->addAction(copyNameAction);
    contextMenu->addAction(copyUrlAction);
    contextMenu->addAction(copyDescAction);

    connect(copyBalanceAction, &QAction::triggered, this, &NftPage::copyBalance);
    connect(copyNameAction, &QAction::triggered, this, &NftPage::copyName);
    connect(copyOwnerAction, &QAction::triggered, this, &NftPage::copyOwnerAddress);
    connect(copyUrlAction, &QAction::triggered, this, &NftPage::copyUrl);
    connect(copyDescAction, &QAction::triggered, this, &NftPage::copyDesc);

    connect(m_nftList, &NftListWidget::customContextMenuRequested, this, &NftPage::contextualMenu);

    connect(m_sendNftDialog, &SendNftDialog::message, this, &NftPage::message);
}

NftPage::~NftPage()
{
    delete ui;
}

void NftPage::setModel(WalletModel *_model)
{
    m_model = _model;
    m_createNftDialog->setModel(m_model);
    m_sendNftDialog->setModel(m_model);
    m_nftList->setModel(m_model);
    m_nftTransactionView->setModel(_model);
    if(m_model && m_model->getNftItemModel())
    {
        // Set current nft
        connect(m_nftList->nftModel(), &QAbstractItemModel::dataChanged, this, &NftPage::on_dataChanged);
        connect(m_nftList->nftModel(), &QAbstractItemModel::rowsInserted, this, &NftPage::on_rowsInserted);
        if(m_nftList->nftModel()->rowCount() > 0)
        {
            QModelIndex currentNft(m_nftList->nftModel()->index(0, 0));
            on_currentNftChanged(currentNft);
        }
    }
}

void NftPage::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;
    m_sendNftDialog->setClientModel(_clientModel);
    m_createNftDialog->setClientModel(_clientModel);
}

void NftPage::on_goToSendNftDialog()
{
    if(checkLogEvents())
    {
        m_sendNftDialog->show();
    }
}

void NftPage::on_goToCreateNftDialog()
{
    if(checkLogEvents())
    {
        m_createNftDialog->show();
    }
}

void NftPage::on_currentNftChanged(QModelIndex index)
{
    if(m_nftList->nftModel())
    {
        if(index.isValid())
        {
            m_selectedNftHash = m_nftList->nftModel()->data(index, NftItemModel::HashRole).toString();
            std::string owner = m_nftList->nftModel()->data(index, NftItemModel::OwnerRole).toString().toStdString();
            std::string id = m_nftList->nftModel()->data(index, NftItemModel::IdRole).toString().toStdString();
            std::string balance = m_nftList->nftModel()->data(index, NftItemModel::RawBalanceRole).toString().toStdString();
            m_sendNftDialog->setNftData(owner, id, balance);

            if(!m_sendNftDialog->isEnabled())
                m_sendNftDialog->setEnabled(true);
        }
        else
        {
            m_sendNftDialog->setEnabled(false);
        }
    }
}

void NftPage::on_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    Q_UNUSED(bottomRight);
    Q_UNUSED(roles);

    if(m_nftList->nftModel())
    {
        QString nftHash = m_nftList->nftModel()->data(topLeft, NftItemModel::HashRole).toString();
        if(m_selectedNftHash.isEmpty() ||
                nftHash == m_selectedNftHash)
        {
            on_currentNftChanged(topLeft);
        }
    }
}

void NftPage::on_currentChanged(QModelIndex current, QModelIndex previous)
{
    Q_UNUSED(previous);

    on_currentNftChanged(current);
}

void NftPage::on_rowsInserted(QModelIndex index, int first, int last)
{
    Q_UNUSED(index);
    Q_UNUSED(first);
    Q_UNUSED(last);

    if(m_nftList->nftModel()->rowCount() == 1)
    {
        QModelIndex currentNft(m_nftList->nftModel()->index(0, 0));
        on_currentNftChanged(currentNft);
    }
}

void NftPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = m_nftList->indexAt(point);
    if(index.isValid())
    {
        indexMenu = index;
        contextMenu->exec(QCursor::pos());
    }
}

void NftPage::copyBalance()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(NftItemModel::BalanceRole).toString());
        indexMenu = QModelIndex();
    }
}

void NftPage::copyName()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(NftItemModel::NameRole).toString());
        indexMenu = QModelIndex();
    }
}

void NftPage::copyOwnerAddress()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(NftItemModel::OwnerRole).toString());
        indexMenu = QModelIndex();
    }
}

void NftPage::copyUrl()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(NftItemModel::UrlRole).toString());
        indexMenu = QModelIndex();
    }
}

void NftPage::copyDesc()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(NftItemModel::DescRole).toString());
        indexMenu = QModelIndex();
    }
}

void NftPage::on_sendNft(const QModelIndex &index)
{
    on_currentNftChanged(index);
    on_goToSendNftDialog();
}

void NftPage::on_createNft()
{
    on_goToCreateNftDialog();
}

bool NftPage::checkLogEvents()
{
    if(!fLogEvents)
    {
        QMessageBox::information(this, tr("Log events"), tr("Enable log events from the option menu in order to receive NFT transactions."));
    }
    return fLogEvents;
}
