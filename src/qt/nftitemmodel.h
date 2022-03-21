#ifndef NFTITEMMODEL_H
#define NFTITEMMODEL_H

#include <QAbstractItemModel>
#include <QStringList>
#include <QThread>

#include <memory>

namespace interfaces {
class Handler;
}

class WalletModel;
class Nft;
class NftItemPriv;
class NftTxWorker;
class NftItemEntry;

class NftItemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum ColumnIndex {
        Name = 0,
        Balance = 1,
    };

    enum DataRole{
        HashRole = Qt::UserRole + 1,
        NameRole = Qt::UserRole + 2,
        OwnerRole = Qt::UserRole + 3,
        BalanceRole = Qt::UserRole + 4,
        RawBalanceRole = Qt::UserRole + 5,
        IdRole = Qt::UserRole + 6,
        UrlRole = Qt::UserRole + 7,
        DescRole = Qt::UserRole + 8,
        NftIdRole = Qt::UserRole + 9,
    };

    NftItemModel(WalletModel *parent = 0);
    ~NftItemModel();

    /** @name Methods overridden from QAbstractItemModel
        @{*/
    QModelIndex index(int row, int column,
                              const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    /*@}*/

    void join();

public Q_SLOTS:
    void checkNftBalanceChanged();

private Q_SLOTS:
    void updateNft(const QString &hash, int status, bool showNft);

private:
    /** Notify listeners that data changed. */
    void emitDataChanged(int index);
    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();

    Nft *nftAbi;
    QStringList columns;
    WalletModel *walletModel;
    NftItemPriv* priv;
    NftTxWorker* worker;
    QThread t;
    std::unique_ptr<interfaces::Handler> m_handler_nft_changed;
    bool nftTxCleaned;

    friend class NftItemPriv;
};

#endif // NFTITEMMODEL_H
