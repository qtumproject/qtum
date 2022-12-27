#ifndef DELEGATIONLISTWIDGET_H
#define DELEGATIONLISTWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QList>

class PlatformStyle;
class WalletModel;
class ClientModel;
class QAbstractItemModel;
class DelegationItemWidget;

/**
 * @brief The DelegationListWidget class List of delegations
 */
class DelegationListWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief DelegationListWidget Constructor
     * @param parent Parent windows of the GUI control
     */
    explicit DelegationListWidget(const PlatformStyle *platformStyle, QWidget *parent = 0);

    void setModel(WalletModel *_model);

    void setClientModel(ClientModel *clientModel);

    QAbstractItemModel *delegationModel() const;

    QModelIndex indexAt(const QPoint &p) const;

Q_SIGNALS:
    void splitCoins(const QModelIndex& index);
    void removeDelegation(const QModelIndex& index);
    void addDelegation();
    void restoreDelegations();
    void clicked(const QModelIndex& index);

public Q_SLOTS:
    void on_rowsInserted(const QModelIndex& parent, int start, int end);
    void on_rowsRemoved(const QModelIndex& parent, int start, int end);
    void on_rowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);
    void on_modelReset();
    void on_layoutChanged();
    void on_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void on_clicked(int position, int button);

private:
    void insertRow(const QModelIndex &index, int position);
    void updateRow(const QModelIndex &index, int position);
    DelegationItemWidget *removeRow(int position);
    void insertItem(int position, DelegationItemWidget* item);
    QModelIndex indexAt(int position) const;

private:
    QVBoxLayout *m_mainLayout;
    const PlatformStyle *m_platfromStyle;
    WalletModel* m_model;
    ClientModel* m_clientModel;
    QAbstractItemModel *m_delegationModel;
    QList<DelegationItemWidget*> m_rows;
};

#endif // DELEGATIONLISTWIDGET_H
