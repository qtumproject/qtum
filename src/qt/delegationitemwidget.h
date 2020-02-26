#ifndef DELEGATIONITEMWIDGET_H
#define DELEGATIONITEMWIDGET_H

#include <QWidget>

class PlatformStyle;

namespace Ui {
class DelegationItemWidget;
}

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
        Remove = 1
    };

    explicit DelegationItemWidget(const PlatformStyle *platformStyle, QWidget *parent = 0, ItemType type = Record);
    ~DelegationItemWidget();

    void setData(const QString& fee, const QString& staker, const QString& address, const QString& filename);

    void setPosition(int position);

    int position() const;

Q_SIGNALS:
    void clicked(int position, int button);

private Q_SLOTS:
    void on_buttonAdd_clicked();

    void on_buttonRemove_clicked();

private:
    void updateLogo();

private:
    Ui::DelegationItemWidget *ui;
    const PlatformStyle *m_platfromStyle;
    ItemType m_type;
    int m_position;
    QString m_filename;
};

#endif // DELEGATIONITEMWIDGET_H
