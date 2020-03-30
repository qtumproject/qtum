#ifndef SUPERSTAKERITEMWIDGET_H
#define SUPERSTAKERITEMWIDGET_H

#include <QWidget>

class PlatformStyle;

namespace Ui {
class SuperStakerItemWidget;
}

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
        Config = 2
    };

    explicit SuperStakerItemWidget(const PlatformStyle *platformStyle, QWidget *parent = 0, ItemType type = Record);
    ~SuperStakerItemWidget();

    void setData(const QString& fee, const QString& staker, const bool& staking_on);

    void setPosition(int position);

    int position() const;

Q_SIGNALS:
    void clicked(int position, int button);

private Q_SLOTS:
    void on_buttonAdd_clicked();

    void on_buttonRemove_clicked();

    void on_buttonConfig_clicked();

private:
    void updateLogo();

private:
    Ui::SuperStakerItemWidget *ui;
    const PlatformStyle *m_platfromStyle;
    ItemType m_type;
    int m_position;
    QString m_filename;
};

#endif // SUPERSTAKERITEMWIDGET_H
