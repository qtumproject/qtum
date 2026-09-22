#include <qt/waitmessagebox.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>

WaitMessageBox::WaitMessageBox(const QString &title, const QString &content, std::function<void()> _run, QWidget *parent) :
    QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setWindowTitle(title);

    QVBoxLayout *mainLay = new QVBoxLayout(this);
    QLabel *lbl = new QLabel(content, this);

    mainLay->addWidget(lbl);
    run = _run;

    QTimer::singleShot(100, this, SLOT(timeout()));
}

void WaitMessageBox::timeout()
{
    if(run) run();
    accept();
}
