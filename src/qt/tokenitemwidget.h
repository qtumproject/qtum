#ifndef TOKENITEMWIDGET_H
#define TOKENITEMWIDGET_H

#include <QWidget>

class PlatformStyle;

namespace Ui {
class TokenItemWidget;
}

class TokenItemWidget : public QWidget
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
        Send = 1,
        Receive = 2
    };

    explicit TokenItemWidget(const PlatformStyle *platformStyle, QWidget *parent = 0, ItemType type = Record);
    ~TokenItemWidget();

    void setData(const QString& tokenName, const QString& tokenBalance, const QString& senderAddress, const QString& filename);

    void setPosition(int position);

    int position() const;

Q_SIGNALS:
    void clicked(int position, int button);

private Q_SLOTS:
    void on_buttonAdd_clicked();

    void on_buttonSend_clicked();

    void on_buttonReceive_clicked();

private:
    void updateLogo();

private:
    Ui::TokenItemWidget *ui;
    const PlatformStyle *m_platfromStyle;
    ItemType m_type;
    int m_position;
    QString m_filename;
};

#endif // TOKENITEMWIDGET_H
