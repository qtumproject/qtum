#ifndef NFTURLLABEL_H
#define NFTURLLABEL_H

#include <QLabel>
#include <QWidget>
#include <Qt>
#include <QMouseEvent>
#include <QPixmap>

class NftUrlLabel : public QLabel { 
    Q_OBJECT

public:
    explicit NftUrlLabel(QWidget* parent = Q_NULLPTR);
    ~NftUrlLabel();

    QString getNftUrl() const;
    void setNftUrl(const QString &value);

    QString getThumbnail() const;
    void setThumbnail(const QString &thumbnail);

    QString getNftName() const;
    void setNftName(const QString &nftName);

Q_SIGNALS:
    void clicked();

public Q_SLOTS:
    void on_clicked();

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QString m_nftUrl;
    QString m_thumbnail;
    QPixmap m_defPixmap;
    QString m_nftName;

};

#endif // NFTURLLABEL_H
