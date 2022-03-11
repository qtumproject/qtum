#ifndef NFTLISTWIDGET_H
#define NFTLISTWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QList>

class PlatformStyle;
class WalletModel;
class QAbstractItemModel;
class NftItemWidget;

/**
 * @brief The NftListWidget class List of nfts
 */
class NftListWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief NftListWidget Constructor
     * @param parent Parent windows of the GUI control
     */
    explicit NftListWidget(const PlatformStyle *platformStyle, QWidget *parent = 0);

    void setModel(WalletModel *_model);

    QAbstractItemModel *nftModel() const;

    QModelIndex indexAt(const QPoint &p) const;

Q_SIGNALS:
    void sendNft(const QModelIndex& index);
    void createNft();
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
    NftItemWidget *removeRow(int position);
    void insertItem(int position, NftItemWidget* item);
    QModelIndex indexAt(int position) const;

private:
    QVBoxLayout *m_mainLayout;
    const PlatformStyle *m_platfromStyle;
    WalletModel* m_model;
    QAbstractItemModel *m_nftModel;
    QList<NftItemWidget*> m_rows;
};

#endif // NFTLISTWIDGET_H
