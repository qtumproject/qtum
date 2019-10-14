#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/tokenitemwidget.h>
#include <qt/forms/ui_tokenitemwidget.h>

TokenItemWidget::TokenItemWidget(QWidget *parent, ItemType type) :
    QWidget(parent),
    ui(new Ui::TokenItemWidget),
    m_type(type),
    m_position(-1)

{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(type);
}

TokenItemWidget::~TokenItemWidget()
{
    delete ui;
}

void TokenItemWidget::setData(const QString &tokenName, const QString &tokenBalance, const QString &receiveAddress, const QString &filename)
{
    ui->tokenName->setText(tokenName);
    ui->tokenBalance->setText(tokenBalance);
    ui->receiveAddress->setText(receiveAddress);
}

void TokenItemWidget::setPosition(int position)
{
    m_position = position;
}

void TokenItemWidget::on_buttonAdd_clicked()
{
    Q_EMIT clicked(m_position, Buttons::Add);
}

void TokenItemWidget::on_buttonSend_clicked()
{
    Q_EMIT clicked(m_position, Buttons::Send);
}

void TokenItemWidget::on_buttonReceive_clicked()
{
    Q_EMIT clicked(m_position, Buttons::Receive);
}
