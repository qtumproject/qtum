#ifndef TOKENLISTWIDGET_H
#define TOKENLISTWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QList>

class PlatformStyle;
class WalletModel;
class QAbstractItemModel;
class TokenItemWidget;

/**
 * @brief The TokenListWidget class List of tokens
 */
class TokenListWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief TokenListWidget Constructor
     * @param parent Parent windows of the GUI control
     */
    explicit TokenListWidget(const PlatformStyle *platformStyle, QWidget *parent = 0);

    void setModel(WalletModel *_model);

    QAbstractItemModel *tokenModel() const;

    QModelIndex indexAt(const QPoint &p) const;

Q_SIGNALS:
    void sendToken(const QModelIndex& index);
    void receiveToken(const QModelIndex& index);
    void addToken();
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
    TokenItemWidget *removeRow(int position);
    void insertItem(int position, TokenItemWidget* item);
    QModelIndex indexAt(int position) const;

private:
    QVBoxLayout *m_mainLayout;
    const PlatformStyle *m_platfromStyle;
    WalletModel* m_model;
    QAbstractItemModel *m_tokenModel;
    QList<TokenItemWidget*> m_rows;
};

#endif // TOKENLISTWIDGET_H
