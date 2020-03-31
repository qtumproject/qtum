#ifndef SUPERSTAKERPAGE_H
#define SUPERSTAKERPAGE_H

#include <qt/superstakerconfigdialog.h>
#include <qt/addsuperstakerpage.h>

#include <QWidget>
#include <QModelIndex>
#include <QAbstractItemModel>

class WalletModel;
class ClientModel;
class SuperStakerListWidget;
class PlatformStyle;
class QMenu;

namespace Ui {
class SuperStakerPage;
}

class SuperStakerPage : public QWidget
{
    Q_OBJECT

public:
    explicit SuperStakerPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~SuperStakerPage();

    void setModel(WalletModel *_model);
    void setClientModel(ClientModel *clientModel);

Q_SIGNALS:

public Q_SLOTS:
    void on_goToConfigSuperStakerPage();
    void on_goToAddSuperStakerPage();
    void on_currentSuperStakerChanged(QModelIndex index);
    void on_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());
    void on_currentChanged(QModelIndex current, QModelIndex previous);
    void on_rowsInserted(QModelIndex index, int first, int last);
    void contextualMenu(const QPoint &);
    void copyStekerFee();
    void copyStakerAddress();
    void configSuperStaker();
    void on_configSuperStaker(const QModelIndex& index);
    void on_addSuperStaker();

private:
    Ui::SuperStakerPage *ui;
    SuperStakerConfigDialog* m_configSuperStakerPage;
    AddSuperStakerPage* m_addSuperStakerPage;
    WalletModel* m_model;
    ClientModel* m_clientModel;
    QAction *m_sendAction;
    QAction *m_configAction;
    QAction *m_addSuperStakerAction;
    QString m_selectedSuperStakerHash;
    const PlatformStyle *m_platformStyle;
    QMenu *contextMenu;
    QModelIndex indexMenu;
    SuperStakerListWidget* m_superStakerList;
};

#endif // SUPERSTAKERPAGE_H
