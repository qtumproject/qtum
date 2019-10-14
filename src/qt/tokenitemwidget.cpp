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

void TokenItemWidget::setData(const QString &tokenName, const QString &tokenBalance, const QString &senderAddress, const QString &filename)
{
    if(tokenName != ui->tokenName->text())
        ui->tokenName->setText(tokenName);
    if(tokenBalance != ui->tokenBalance->text())
        ui->tokenBalance->setText(tokenBalance);
    if(senderAddress != ui->senderAddress->text())
        ui->senderAddress->setText(senderAddress);
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

int TokenItemWidget::position() const
{
    return m_position;
}
