#ifndef STAKERDELEGATIONVIEW_H
#define STAKERDELEGATIONVIEW_H

#include <qt/guiutil.h>

#include <QWidget>

class DelegationFilterProxy;
class WalletModel;

QT_BEGIN_NAMESPACE
class QComboBox;
class QDateTimeEdit;
class QFrame;
class QLineEdit;
class QMenu;
class QSpinBox;
class QTableView;
QT_END_NAMESPACE

class StakerDelegationView : public QWidget
{
    Q_OBJECT
public:
    explicit StakerDelegationView(QWidget *parent = nullptr);

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
        DATE_COLUMN_WIDTH = 130,
        FEE_COLUMN_WIDTH = 80,
        POD_COLUMN_WIDTH = 150,
        POD_MINIMUM_COLUMN_WIDTH = 100,
        MINIMUM_COLUMN_WIDTH = 23
    };

private:
    WalletModel *model;
    DelegationFilterProxy *delegationProxyModel;
    QTableView *delegationView;

    QComboBox *dateWidget;
    QLineEdit *addressWidget;
    QSpinBox *feeWidget;
    QLineEdit *podWidget;

    QMenu *contextMenu;

    QFrame *dateRangeWidget;
    QDateTimeEdit *dateFrom;
    QDateTimeEdit *dateTo;

    QWidget *createDateRangeWidget();

    GUIUtil::TableViewLastColumnResizingFixer *columnResizingFixer;

    virtual void resizeEvent(QResizeEvent* event);

private Q_SLOTS:
    void contextualMenu(const QPoint &);
    void dateRangeChanged();
    void copyAddress();
    void copyFee();
    void copyPoD();

public Q_SLOTS:
    void chooseDate(int idx);
    void changedPrefix(const QString &prefix);
    void changedFee(int fee);
    void changedPoD(const QString &prefix);
};

#endif // STAKERDELEGATIONVIEW_H
