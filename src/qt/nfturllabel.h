#ifndef NFTURLLABEL_H
#define NFTURLLABEL_H

#include <QLabel>
#include <QWidget>
#include <Qt>

class NftUrlLabel : public QLabel { 
    Q_OBJECT 

public:
    explicit NftUrlLabel(QWidget* parent = Q_NULLPTR);
    ~NftUrlLabel();

    QString getNftUrl() const;
    void setNftUrl(const QString &value);

Q_SIGNALS:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    QString nftUrl;

};

#endif // NFTURLLABEL_H
