#ifndef SUPERSTAKERITEMWIDGET_H
#define SUPERSTAKERITEMWIDGET_H

#include <QWidget>

class PlatformStyle;
class WalletModel;

namespace Ui {
class SuperStakerItemWidget;
}

class SuperStakerItemWidgetPriv;

class SuperStakerItemWidget : public QWidget
{
    Q_OBJECT

public:
    enum ItemType
    {
        Record = 0,
        New = 1
    };

    enum Buttons{
        Add = 0,
        Remove = 1,
        Config = 2,
        Delegations = 3,
        Split = 4,
        Restore = 5
    };

    explicit SuperStakerItemWidget(const PlatformStyle *platformStyle, QWidget *parent = 0, ItemType type = Record);
    ~SuperStakerItemWidget();

    void setData(const QString& fee, const QString& staker, const QString& address, const bool& staking_on, const int64_t& balance, const int64_t& stake, const int64_t& weight, const int64_t& delegationsWeight);

    void setPosition(int position);

    int position() const;

    void setModel(WalletModel *_model);

Q_SIGNALS:
    void clicked(int position, int button);

private Q_SLOTS:
    void updateDisplayUnit();

    void on_buttonAdd_clicked();

    void on_buttonRemove_clicked();

    void on_buttonConfig_clicked();

    void on_buttonDelegations_clicked();

    void on_buttonSplit_clicked();

    void on_buttonRestore_clicked();

private:
    void updateLogo();
    void updateBalance();
    void updateLabelStaker();

private:
    Ui::SuperStakerItemWidget *ui;
    const PlatformStyle *m_platfromStyle;
    ItemType m_type;
    int m_position;
    QString m_filename;
    WalletModel* m_model;
    SuperStakerItemWidgetPriv* d;
};

#endif // SUPERSTAKERITEMWIDGET_H
