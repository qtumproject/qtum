#ifndef SUPERSTAKERLISTWIDGET_H
#define SUPERSTAKERLISTWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QList>

class PlatformStyle;
class WalletModel;
class ClientModel;
class QAbstractItemModel;
class SuperStakerItemWidget;

/**
 * @brief The SuperStakerListWidget class List of super stakers
 */
class SuperStakerListWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief SuperStakerListWidget Constructor
     * @param parent Parent windows of the GUI control
     */
    explicit SuperStakerListWidget(const PlatformStyle *platformStyle, QWidget *parent = 0);

    void setModel(WalletModel *_model);

    void setClientModel(ClientModel *clientModel);

    QAbstractItemModel *superStakerModel() const;

    QModelIndex indexAt(const QPoint &p) const;

Q_SIGNALS:
    void splitCoins(const QModelIndex& index);
    void configSuperStaker(const QModelIndex& index);
    void delegationsSuperStaker(const QModelIndex& index);
    void removeSuperStaker(const QModelIndex& index);
    void addSuperStaker();
    void restoreSuperStakers();
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
    SuperStakerItemWidget *removeRow(int position);
    void insertItem(int position, SuperStakerItemWidget* item);
    QModelIndex indexAt(int position) const;

private:
    QVBoxLayout *m_mainLayout;
    const PlatformStyle *m_platfromStyle;
    WalletModel* m_model;
    ClientModel* m_clientModel;
    QAbstractItemModel *m_superStakerModel;
    QList<SuperStakerItemWidget*> m_rows;
};

#endif // SUPERSTAKERLISTWIDGET_H
