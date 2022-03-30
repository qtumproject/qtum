#ifndef NFTTRANSACTIONVIEW_H
#define NFTTRANSACTIONVIEW_H

#include <qt/guiutil.h>

#include <QWidget>
#include <QKeyEvent>

class PlatformStyle;
class NftTxFilterProxy;
class WalletModel;

QT_BEGIN_NAMESPACE
class QComboBox;
class QDateTimeEdit;
class QLineEdit;
class QMenu;
class QTableView;
QT_END_NAMESPACE

class NftTransactionView : public QWidget
{
    Q_OBJECT
public:
    explicit NftTransactionView(const PlatformStyle *platformStyle, QWidget *parent = 0);

    void setModel(WalletModel *model);

    // Date ranges for filter
    enum DateEnum
    {
        All,
        Today,
        ThisWeek,
        ThisMonth,
        LastMonth,
        ThisYear,
        Range
    };

    enum ColumnWidths {
        STATUS_COLUMN_WIDTH = 30,
        DATE_COLUMN_WIDTH = 130,
        TYPE_COLUMN_WIDTH = 150,
        NAME_COLUMN_WIDTH = 230,
        AMOUNT_COLUMN_WIDTH = 100,
        AMOUNT_MINIMUM_COLUMN_WIDTH = 80,
        MINIMUM_COLUMN_WIDTH = 23
    };

private:
    WalletModel *model;
    NftTxFilterProxy *nftTxProxyModel;
    QTableView *nftView;

    QComboBox *dateWidget;
    QComboBox *typeWidget;
    QComboBox *nameWidget;
    QLineEdit *addressWidget;
    QLineEdit *amountWidget;

    QMenu *contextMenu;

    QFrame *dateRangeWidget;
    QDateTimeEdit *dateFrom;
    QDateTimeEdit *dateTo;

    QWidget *createDateRangeWidget();

    GUIUtil::TableViewLastColumnResizingFixer *columnResizingFixer;

    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private Q_SLOTS:
    void contextualMenu(const QPoint &);
    void dateRangeChanged();
    void showDetails();
    void copyAddress();
    void copyAmount();
    void copyTxID();
    void copyTxPlainText();

Q_SIGNALS:

public Q_SLOTS:
    void chooseDate(int idx);
    void chooseType(int idx);
    void chooseName(int idx);
    void changedPrefix(const QString &prefix);
    void changedAmount(const QString &amount);
    void addToNameWidget(const QModelIndex& parent, int start, int /*end*/);
    void removeFromNameWidget(const QModelIndex& parent, int start, int /*end*/);
    void refreshNameWidget();
};

#endif // NFTTRANSACTIONVIEW_H
