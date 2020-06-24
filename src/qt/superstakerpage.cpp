#include <qt/superstakerpage.h>
#include <qt/forms/ui_superstakerpage.h>
#include <qt/superstakeritemmodel.h>
#include <qt/walletmodel.h>
#include <qt/platformstyle.h>
#include <qt/styleSheet.h>
#include <qt/superstakerlistwidget.h>
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

SuperStakerPage::SuperStakerPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SuperStakerPage),
    m_model(0),
    m_clientModel(0)
{
    ui->setupUi(this);

    m_platformStyle = platformStyle;

    m_configSuperStakerPage = new SuperStakerConfigDialog(this);
    m_addSuperStakerPage = new AddSuperStakerPage(this);
    m_delegationsSuperStakerPage = new DelegationsStakerDialog(this);
    m_splitUtxoPage = new SplitUTXOPage(this, SplitUTXOPage::SuperStaker);

    m_configSuperStakerPage->setEnabled(false);
    m_delegationsSuperStakerPage->setEnabled(false);
    m_splitUtxoPage->setEnabled(false);

    QAction *copyStakerNameAction = new QAction(tr("Copy staker name"), this);
    QAction *copyStakerAddressAction = new QAction(tr("Copy staker address"), this);
    QAction *copyStekerMinFeeAction = new QAction(tr("Copy staker minimum fee"), this);
    QAction *copyStekerWeightAction = new QAction(tr("Copy staker weight"), this);
    QAction *copyDelegationsWeightAction = new QAction(tr("Copy delegations weight"), this);
    QAction *configSuperStakerAction = new QAction(tr("Configure super staker"), this);
    QAction *editStakerNameAction = new QAction(tr("Edit staker name"), this);
    QAction *removeSuperStakerAction = new QAction(tr("Remove super staker"), this);

    m_superStakerList = new SuperStakerListWidget(platformStyle, this);
    m_superStakerList->setContextMenuPolicy(Qt::CustomContextMenu);
    new QVBoxLayout(ui->scrollArea);
    ui->scrollArea->setWidget(m_superStakerList);
    ui->scrollArea->setWidgetResizable(true);
    connect(m_superStakerList, &SuperStakerListWidget::configSuperStaker, this, &SuperStakerPage::on_configSuperStaker);
    connect(m_superStakerList, &SuperStakerListWidget::addSuperStaker, this, &SuperStakerPage::on_addSuperStaker);
    connect(m_superStakerList, &SuperStakerListWidget::removeSuperStaker, this, &SuperStakerPage::on_removeSuperStaker);
    connect(m_superStakerList, &SuperStakerListWidget::delegationsSuperStaker, this, &SuperStakerPage::on_delegationsSuperStaker);
    connect(m_superStakerList, &SuperStakerListWidget::splitCoins, this, &SuperStakerPage::on_splitCoins);
    connect(m_superStakerList, &SuperStakerListWidget::restoreSuperStakers, this, &SuperStakerPage::on_restoreSuperStakers);

    contextMenu = new QMenu(m_superStakerList);
    contextMenu->addAction(copyStakerNameAction);
    contextMenu->addAction(copyStakerAddressAction);
    contextMenu->addAction(copyStekerWeightAction);
    contextMenu->addAction(copyDelegationsWeightAction);
    contextMenu->addAction(copyStekerMinFeeAction);
    contextMenu->addAction(configSuperStakerAction);
    contextMenu->addAction(editStakerNameAction);
    contextMenu->addAction(removeSuperStakerAction);

    connect(copyStakerNameAction, &QAction::triggered, this, &SuperStakerPage::copyStakerName);
    connect(copyStakerAddressAction, &QAction::triggered, this, &SuperStakerPage::copyStakerAddress);
    connect(copyStekerMinFeeAction, &QAction::triggered, this, &SuperStakerPage::copyStekerMinFee);
    connect(copyStekerWeightAction, &QAction::triggered, this, &SuperStakerPage::copyStakerWeight);
    connect(copyDelegationsWeightAction, &QAction::triggered, this, &SuperStakerPage::copyDelegationsWeight);
    connect(configSuperStakerAction, &QAction::triggered, this, &SuperStakerPage::configSuperStaker);
    connect(editStakerNameAction, &QAction::triggered, this, &SuperStakerPage::editStakerName);
    connect(removeSuperStakerAction, &QAction::triggered, this, &SuperStakerPage::removeSuperStaker);

    connect(m_superStakerList, &SuperStakerListWidget::customContextMenuRequested, this, &SuperStakerPage::contextualMenu);
}

SuperStakerPage::~SuperStakerPage()
{
    delete ui;
}

void SuperStakerPage::setModel(WalletModel *_model)
{
    m_model = _model;
    m_addSuperStakerPage->setModel(m_model);
    m_configSuperStakerPage->setModel(m_model);
    m_delegationsSuperStakerPage->setModel(m_model);
    m_superStakerList->setModel(m_model);
    m_splitUtxoPage->setModel(m_model);
    if(m_model && m_model->getSuperStakerItemModel())
    {
        // Set current super staker
        connect(m_superStakerList->superStakerModel(), &QAbstractItemModel::dataChanged, this, &SuperStakerPage::on_dataChanged);
        connect(m_superStakerList->superStakerModel(), &QAbstractItemModel::rowsInserted, this, &SuperStakerPage::on_rowsInserted);
        if(m_superStakerList->superStakerModel()->rowCount() > 0)
        {
            QModelIndex currentSuperStaker(m_superStakerList->superStakerModel()->index(0, 0));
            on_currentSuperStakerChanged(currentSuperStaker);
        }
    }
}

void SuperStakerPage::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;
    m_configSuperStakerPage->setClientModel(_clientModel);
}

void SuperStakerPage::on_goToConfigSuperStakerPage()
{
    m_configSuperStakerPage->show();
}

void SuperStakerPage::on_goToAddSuperStakerPage()
{
    m_addSuperStakerPage->show();
}

void SuperStakerPage::on_currentSuperStakerChanged(QModelIndex index)
{
    if(m_superStakerList->superStakerModel())
    {
        if(index.isValid())
        {
            QString hash = m_superStakerList->superStakerModel()->data(index, SuperStakerItemModel::HashRole).toString();
            m_selectedSuperStakerHash = hash;
            QString address = m_superStakerList->superStakerModel()->data(index, SuperStakerItemModel::StakerAddressRole).toString();
            QString name = m_superStakerList->superStakerModel()->data(index, SuperStakerItemModel::StakerNameRole).toString();
            int minFee = m_superStakerList->superStakerModel()->data(index, SuperStakerItemModel::MinFeeRole).toInt();
            m_configSuperStakerPage->setSuperStakerData(hash);
            m_delegationsSuperStakerPage->setSuperStakerData(name, address, minFee, hash);
            m_splitUtxoPage->setAddress(address);

            if(!m_configSuperStakerPage->isEnabled())
                m_configSuperStakerPage->setEnabled(true);
            if(!m_delegationsSuperStakerPage->isEnabled())
                m_delegationsSuperStakerPage->setEnabled(true);
            if(!m_splitUtxoPage->isEnabled())
                m_splitUtxoPage->setEnabled(true);
        }
        else
        {
            m_configSuperStakerPage->setEnabled(false);
            m_configSuperStakerPage->setSuperStakerData("");
            m_delegationsSuperStakerPage->setEnabled(false);
            m_delegationsSuperStakerPage->setSuperStakerData("", "", 0, "");
            m_splitUtxoPage->setEnabled(false);
            m_splitUtxoPage->setAddress("");
            m_selectedSuperStakerHash = "";
        }
    }
}

void SuperStakerPage::on_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    Q_UNUSED(bottomRight);
    Q_UNUSED(roles);

    if(m_superStakerList->superStakerModel())
    {
        QString superStakerHash = m_superStakerList->superStakerModel()->data(topLeft, SuperStakerItemModel::HashRole).toString();
        if(m_selectedSuperStakerHash.isEmpty() ||
                superStakerHash == m_selectedSuperStakerHash)
        {
            on_currentSuperStakerChanged(topLeft);
        }
    }
}

void SuperStakerPage::on_currentChanged(QModelIndex current, QModelIndex previous)
{
    Q_UNUSED(previous);

    on_currentSuperStakerChanged(current);
}

void SuperStakerPage::on_rowsInserted(QModelIndex index, int first, int last)
{
    Q_UNUSED(index);
    Q_UNUSED(first);
    Q_UNUSED(last);

    if(m_superStakerList->superStakerModel()->rowCount() == 1)
    {
        QModelIndex currentSuperStaker(m_superStakerList->superStakerModel()->index(0, 0));
        on_currentSuperStakerChanged(currentSuperStaker);
    }
}

void SuperStakerPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = m_superStakerList->indexAt(point);
    if(index.isValid())
    {
        indexMenu = index;
        contextMenu->exec(QCursor::pos());
    }
}

void SuperStakerPage::copyStekerMinFee()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(SuperStakerItemModel::FormattedMinFeeRole).toString());
        indexMenu = QModelIndex();
    }
}

void SuperStakerPage::copyStakerName()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(SuperStakerItemModel::StakerNameRole).toString());
        indexMenu = QModelIndex();
    }
}

void SuperStakerPage::copyStakerAddress()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(SuperStakerItemModel::StakerAddressRole).toString());
        indexMenu = QModelIndex();
    }
}

void SuperStakerPage::copyStakerWeight()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(SuperStakerItemModel::FormattedWeightRole).toString());
        indexMenu = QModelIndex();
    }
}

void SuperStakerPage::copyDelegationsWeight()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(SuperStakerItemModel::FormattedDelegationsWeightRole).toString());
        indexMenu = QModelIndex();
    }
}

void SuperStakerPage::configSuperStaker()
{
    if(indexMenu.isValid())
    {
        on_configSuperStaker(indexMenu);
        indexMenu = QModelIndex();
    }
}

void SuperStakerPage::editStakerName()
{
    if(indexMenu.isValid())
    {
        QString stakerName = indexMenu.data(SuperStakerItemModel::StakerNameRole).toString();
        QString stakerAddress = indexMenu.data(SuperStakerItemModel::StakerAddressRole).toString();
        QString sHash = indexMenu.data(SuperStakerItemModel::HashRole).toString();
        uint256 hash;
        hash.SetHex(sHash.toStdString());

        EditSuperStakerDialog dlg;
        dlg.setData(stakerName, stakerAddress);

        if(dlg.exec())
        {
            interfaces::SuperStakerInfo staker = m_model->wallet().getSuperStaker(hash);
            if(staker.hash == hash)
            {
                staker.staker_name = dlg.getSuperStakerName().toStdString();
                m_model->wallet().removeSuperStakerEntry(sHash.toStdString());
                m_model->wallet().addSuperStakerEntry(staker);
            }
        }
    }
}

void SuperStakerPage::on_configSuperStaker(const QModelIndex &index)
{
    m_configSuperStakerPage->clearAll();
    on_currentSuperStakerChanged(index);
    on_goToConfigSuperStakerPage();
}

void SuperStakerPage::on_addSuperStaker()
{
    on_goToAddSuperStakerPage();
}

void SuperStakerPage::removeSuperStaker()
{
    if(indexMenu.isValid())
    {
        on_removeSuperStaker(indexMenu);
        indexMenu = QModelIndex();
    }
}

void SuperStakerPage::on_removeSuperStaker(const QModelIndex &index)
{
    if(index.isValid() && m_model)
    {
        QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, tr("Confirm super staker removal"), tr("The selected super staker will be removed from the list. Are you sure?"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

        if(btnRetVal == QMessageBox::Yes)
        {
            QString hash = m_superStakerList->superStakerModel()->data(index, SuperStakerItemModel::HashRole).toString();
            m_model->wallet().removeSuperStakerEntry(hash.toStdString());
        }
    }
}

void SuperStakerPage::on_delegationsSuperStaker(const QModelIndex &index)
{
    on_currentSuperStakerChanged(index);
    on_goToDelegationsSuperStakerPage();
}

void SuperStakerPage::on_restoreSuperStakers()
{
    if(m_model)
    {
        bool fSuperStake = m_model->wallet().getEnabledSuperStaking();
        if(!fSuperStake)
        {
            QMessageBox::information(this, tr("Super staking"), tr("Enable super staking from the option menu in order to start the restoration."));
        }
        else
        {
            QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, tr("Confirm super stakers restoration"), tr("Are you sure you wish to restore your super stakers?"),
                QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

            if(btnRetVal == QMessageBox::Yes)
            {
                if(m_model->wallet().restoreSuperStakers() == 0)
                {
                    QMessageBox::information(this, tr("Super stakers not found"), tr("No super stakers found to restore."), QMessageBox::Ok);
                }
            }
        }
    }
}

void SuperStakerPage::on_goToDelegationsSuperStakerPage()
{
    m_delegationsSuperStakerPage->show();
}

void SuperStakerPage::on_splitCoins(const QModelIndex &index)
{
    on_currentSuperStakerChanged(index);
    on_goToSplitCoinsPage();
}

void SuperStakerPage::on_goToSplitCoinsPage()
{
    m_splitUtxoPage->show();
}
