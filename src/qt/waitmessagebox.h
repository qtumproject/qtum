#ifndef WAITMESSAGEBOX_H
#define WAITMESSAGEBOX_H

#include <QDialog>
#include <functional>
#include <utility>

class WaitMessageBox : public QDialog
{
    Q_OBJECT
public:
    WaitMessageBox(const QString &title, const QString &content, std::function<void()> run, QWidget *parent = nullptr);

public Q_SLOTS:
    void timeout();

private:
    std::function<void()> run;
};

#endif // WAITMESSAGEBOX_H
