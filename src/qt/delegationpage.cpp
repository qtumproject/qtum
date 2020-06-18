#include <qt/delegationpage.h>
#include <qt/forms/ui_delegationpage.h>
#include <qt/delegationitemmodel.h>
#include <qt/walletmodel.h>
#include <qt/platformstyle.h>
#include <qt/styleSheet.h>
#include <qt/delegationlistwidget.h>
#include <qt/guiutil.h>
#include <qt/editsuperstakerdialog.h>

#include <QPainter>
#include <QAbstractItemDelegate>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QSizePolicy>
#include <QMenu>
#include <QMessageBox>

DelegationPage::DelegationPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DelegationPage),
    m_model(0),
    m_clientModel(0)
{
    ui->setupUi(this);

    m_platformStyle = platformStyle;

    m_removeDelegationPage = new RemoveDelegationPage(this);
    m_addDelegationPage = new AddDelegationPage(this);
    m_splitUtxoPage = new SplitUTXOPage(this, SplitUTXOPage::Delegation);

    m_removeDelegationPage->setEnabled(false);
    m_splitUtxoPage->setEnabled(false);

    QAction *copyStakerNameAction = new QAction(tr("Copy staker name"), this);
    QAction *copyStakerAddressAction = new QAction(tr("Copy staker address"), this);
    QAction *copyStekerFeeAction = new QAction(tr("Copy staker fee"), this);
    QAction *copyDelegateAddressAction = new QAction(tr("Copy delegate address"), this);
    QAction *copyDelegateWeightAction = new QAction(tr("Copy delegate weight"), this);
    QAction *editStakerNameAction = new QAction(tr("Edit staker name"), this);
    QAction *removeDelegationAction = new QAction(tr("Remove delegation"), this);

    m_delegationList = new DelegationListWidget(platformStyle, this);
    m_delegationList->setContextMenuPolicy(Qt::CustomContextMenu);
    new QVBoxLayout(ui->scrollArea);
    ui->scrollArea->setWidget(m_delegationList);
    ui->scrollArea->setWidgetResizable(true);
    connect(m_delegationList, &DelegationListWidget::removeDelegation, this, &DelegationPage::on_removeDelegation);
    connect(m_delegationList, &DelegationListWidget::addDelegation, this, &DelegationPage::on_addDelegation);
    connect(m_delegationList, &DelegationListWidget::splitCoins, this, &DelegationPage::on_splitCoins);
    connect(m_delegationList, &DelegationListWidget::restoreDelegations, this, &DelegationPage::on_restoreDelegations);

    contextMenu = new QMenu(m_delegationList);
    contextMenu->addAction(copyStakerNameAction);
    contextMenu->addAction(copyStakerAddressAction);
    contextMenu->addAction(copyStekerFeeAction);
    contextMenu->addAction(copyDelegateAddressAction);
    contextMenu->addAction(copyDelegateWeightAction);
    contextMenu->addAction(editStakerNameAction);
    contextMenu->addAction(removeDelegationAction);

    connect(copyDelegateAddressAction, &QAction::triggered, this, &DelegationPage::copyDelegateAddress);
    connect(copyStekerFeeAction, &QAction::triggered, this, &DelegationPage::copyStekerFee);
    connect(copyStakerNameAction, &QAction::triggered, this, &DelegationPage::copyStakerName);
    connect(copyStakerAddressAction, &QAction::triggered, this, &DelegationPage::copyStakerAddress);
    connect(copyDelegateWeightAction, &QAction::triggered, this, &DelegationPage::copyDelegateWeight);
    connect(editStakerNameAction, &QAction::triggered, this, &DelegationPage::editStakerName);
    connect(removeDelegationAction, &QAction::triggered, this, &DelegationPage::removeDelegation);

    connect(m_delegationList, &DelegationListWidget::customContextMenuRequested, this, &DelegationPage::contextualMenu);
}

DelegationPage::~DelegationPage()
{
    delete ui;
}

void DelegationPage::setModel(WalletModel *_model)
{
    m_model = _model;
    m_addDelegationPage->setModel(m_model);
    m_removeDelegationPage->setModel(m_model);
    m_delegationList->setModel(m_model);
    m_splitUtxoPage->setModel(m_model);
    if(m_model && m_model->getDelegationItemModel())
    {
        // Set current delegation
        connect(m_delegationList->delegationModel(), &QAbstractItemModel::dataChanged, this, &DelegationPage::on_dataChanged);
        connect(m_delegationList->delegationModel(), &QAbstractItemModel::rowsInserted, this, &DelegationPage::on_rowsInserted);
        if(m_delegationList->delegationModel()->rowCount() > 0)
        {
            QModelIndex currentDelegation(m_delegationList->delegationModel()->index(0, 0));
            on_currentDelegationChanged(currentDelegation);
        }
    }
}

void DelegationPage::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;
    m_addDelegationPage->setClientModel(_clientModel);
    m_removeDelegationPage->setClientModel(_clientModel);
}

void DelegationPage::on_goToRemoveDelegationPage()
{
    m_removeDelegationPage->show();
}

void DelegationPage::on_goToAddDelegationPage()
{
    m_addDelegationPage->show();
}

void DelegationPage::on_currentDelegationChanged(QModelIndex index)
{
    if(m_delegationList->delegationModel())
    {
        if(index.isValid())
        {
            m_selectedDelegationHash = m_delegationList->delegationModel()->data(index, DelegationItemModel::HashRole).toString();
            QString address = m_delegationList->delegationModel()->data(index, DelegationItemModel::AddressRole).toString();
            QString hash = m_delegationList->delegationModel()->data(index, DelegationItemModel::HashRole).toString();
            m_removeDelegationPage->setDelegationData(address, hash);
            m_splitUtxoPage->setAddress(address);

            if(!m_removeDelegationPage->isEnabled())
                m_removeDelegationPage->setEnabled(true);
            if(!m_splitUtxoPage->isEnabled())
                m_splitUtxoPage->setEnabled(true);
        }
        else
        {
            m_removeDelegationPage->setEnabled(false);
            m_removeDelegationPage->setDelegationData("", "");
            m_splitUtxoPage->setAddress("");
            m_selectedDelegationHash = "";
        }
    }
}

void DelegationPage::on_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    Q_UNUSED(bottomRight);
    Q_UNUSED(roles);

    if(m_delegationList->delegationModel())
    {
        QString delegationHash = m_delegationList->delegationModel()->data(topLeft, DelegationItemModel::HashRole).toString();
        if(m_selectedDelegationHash.isEmpty() ||
                delegationHash == m_selectedDelegationHash)
        {
            on_currentDelegationChanged(topLeft);
        }
    }
}

void DelegationPage::on_currentChanged(QModelIndex current, QModelIndex previous)
{
    Q_UNUSED(previous);

    on_currentDelegationChanged(current);
}

void DelegationPage::on_rowsInserted(QModelIndex index, int first, int last)
{
    Q_UNUSED(index);
    Q_UNUSED(first);
    Q_UNUSED(last);

    if(m_delegationList->delegationModel()->rowCount() == 1)
    {
        QModelIndex currentDelegation(m_delegationList->delegationModel()->index(0, 0));
        on_currentDelegationChanged(currentDelegation);
    }
}

void DelegationPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = m_delegationList->indexAt(point);
    if(index.isValid())
    {
        indexMenu = index;
        contextMenu->exec(QCursor::pos());
    }
}

void DelegationPage::copyDelegateAddress()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(DelegationItemModel::AddressRole).toString());
        indexMenu = QModelIndex();
    }
}

void DelegationPage::copyDelegateWeight()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(DelegationItemModel::FormattedWeightRole).toString());
        indexMenu = QModelIndex();
    }
}

void DelegationPage::copyStekerFee()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(DelegationItemModel::FormattedFeeRole).toString());
        indexMenu = QModelIndex();
    }
}

void DelegationPage::copyStakerName()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(DelegationItemModel::StakerNameRole).toString());
        indexMenu = QModelIndex();
    }
}

void DelegationPage::copyStakerAddress()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(DelegationItemModel::StakerAddressRole).toString());
        indexMenu = QModelIndex();
    }
}

void DelegationPage::editStakerName()
{
    if(indexMenu.isValid())
    {
        QString stakerName = indexMenu.data(DelegationItemModel::StakerNameRole).toString();
        QString stakerAddress = indexMenu.data(DelegationItemModel::StakerAddressRole).toString();
        QString sHash = indexMenu.data(DelegationItemModel::HashRole).toString();
        uint256 hash;
        hash.SetHex(sHash.toStdString());

        EditSuperStakerDialog dlg;
        dlg.setData(stakerName, stakerAddress);

        if(dlg.exec())
        {
            interfaces::DelegationInfo delegation = m_model->wallet().getDelegation(hash);
            if(delegation.hash == hash)
            {
                delegation.staker_name = dlg.getSuperStakerName().toStdString();
                m_model->wallet().removeDelegationEntry(sHash.toStdString());
                m_model->wallet().addDelegationEntry(delegation);
            }
        }
    }
}

void DelegationPage::removeDelegation()
{
    if(indexMenu.isValid())
    {
        on_removeDelegation(indexMenu);
        indexMenu = QModelIndex();
    }
}

void DelegationPage::on_removeDelegation(const QModelIndex &index)
{
    on_currentDelegationChanged(index);
    on_goToRemoveDelegationPage();
}

void DelegationPage::on_addDelegation()
{
    on_goToAddDelegationPage();
}

void DelegationPage::on_splitCoins(const QModelIndex &index)
{
    on_currentDelegationChanged(index);
    on_goToSplitCoinsPage();
}

void DelegationPage::on_goToSplitCoinsPage()
{
    m_splitUtxoPage->show();
}

void DelegationPage::on_restoreDelegations()
{
    if(m_model)
    {
        QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, tr("Confirm delegations restoration"), tr("Are you sure you wish to restore your delegations?"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

        if(btnRetVal == QMessageBox::Yes)
        {
            if(m_model->wallet().restoreDelegations() == 0)
            {
                QMessageBox::information(this, tr("Delegations not found"), tr("No delegations found to restore."), QMessageBox::Ok);
            }
        }
    }
}
