#ifndef NFTITEMWIDGET_H
#define NFTITEMWIDGET_H

#include <QWidget>

class PlatformStyle;

namespace Ui {
class NftItemWidget;
}

class NftItemWidget : public QWidget
{
    Q_OBJECT

public:
    enum ItemType
    {
        Record = 0,
        New = 1
    };

    enum Buttons{
        Create = 0,
        Send = 1
    };

    explicit NftItemWidget(const PlatformStyle *platformStyle, QWidget *parent = 0, ItemType type = Record);
    ~NftItemWidget();

    void setData(const QString& nftName, const QString& nftBalance, const QString& nftOwner, const QString& nftDesc, const QString& nftUrl);

    void setPosition(int position);

    int position() const;

Q_SIGNALS:
    void clicked(int position, int button);

private Q_SLOTS:
    void on_buttonCreate_clicked();

    void on_buttonSend_clicked();

private:
    void updateLogo();
    void getLabelText(const QString& itemName, QString& itemText, QString& itemTooltip);

private:
    Ui::NftItemWidget *ui;
    const PlatformStyle *m_platfromStyle;
    ItemType m_type;
    int m_position;
};

#endif // NFTITEMWIDGET_H
