#ifndef NFTPAGE_H
#define NFTPAGE_H

#include <qt/sendnftdialog.h>
#include <qt/createnftdialog.h>

#include <QWidget>
#include <QModelIndex>
#include <QAbstractItemModel>

class WalletModel;
class ClientModel;
class NftTransactionView;
class NftListWidget;
class PlatformStyle;
class QMenu;

namespace Ui {
class NftPage;
}

class NftPage : public QWidget
{
    Q_OBJECT

public:
    explicit NftPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~NftPage();

    void setModel(WalletModel *_model);
    void setClientModel(ClientModel *clientModel);

Q_SIGNALS:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);

public Q_SLOTS:
    void on_goToSendNftDialog();
    void on_goToCreateNftDialog();
    void on_currentNftChanged(QModelIndex index);
    void on_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());
    void on_currentChanged(QModelIndex current, QModelIndex previous);
    void on_rowsInserted(QModelIndex index, int first, int last);
    void on_sendNft(const QModelIndex& index);
    void on_createNft();
    void contextualMenu(const QPoint &);
    void copyBalance();
    void copyName();
    void copyOwnerAddress();
    void copyUrl();
    void copyDesc();

private:
    bool checkLogEvents();

private:
    Ui::NftPage *ui;
    SendNftDialog* m_sendNftDialog;
    CreateNftDialog* m_createNftDialog;
    WalletModel* m_model;
    ClientModel* m_clientModel;
    QAction *m_sendAction;
    QAction *m_createAction;
    QString m_selectedNftHash;
    NftTransactionView *m_nftTransactionView;
    const PlatformStyle *m_platformStyle;
    QMenu *contextMenu;
    QModelIndex indexMenu;
    NftListWidget* m_nftList;
};

#endif // NFTPAGE_H
