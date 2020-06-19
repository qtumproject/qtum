#ifndef DELEGATIONITEMWIDGET_H
#define DELEGATIONITEMWIDGET_H

#include <QWidget>

class PlatformStyle;
class WalletModel;

namespace Ui {
class DelegationItemWidget;
}
class DelegationItemWidgetPriv;

class DelegationItemWidget : public QWidget
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
        Split = 2,
        Restore = 3
    };

    enum LightType{
        Transparent = 0,
        Red = 1,
        Orange = 2,
        Green = 3
    };

    explicit DelegationItemWidget(const PlatformStyle *platformStyle, QWidget *parent = 0, ItemType type = Record);
    ~DelegationItemWidget();

    void setData(const QString& fee, const QString& staker, const QString& address, const int32_t& blockHight, const int64_t& balance, const int64_t& stake, const int64_t& weight, const int32_t& status);

    void setPosition(int position);

    int position() const;

    void setModel(WalletModel *_model);

    void setLight(LightType type);

Q_SIGNALS:
    void clicked(int position, int button);

private Q_SLOTS:
    void updateDisplayUnit();

    void on_buttonAdd_clicked();

    void on_buttonRemove_clicked();

    void on_buttonSplit_clicked();

    void on_buttonRestore_clicked();

private:
    void updateLogo();
    void updateBalance();
    void updateLabelStaker();

private:
    Ui::DelegationItemWidget *ui;
    const PlatformStyle *m_platfromStyle;
    ItemType m_type;
    int m_position;
    QString m_filename;
    WalletModel* m_model;
    DelegationItemWidgetPriv* d;
};

#endif // DELEGATIONITEMWIDGET_H
