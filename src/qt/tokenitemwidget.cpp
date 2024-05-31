#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/tokenitemwidget.h>
#include <qt/platformstyle.h>
#include <qt/forms/ui_tokenitemwidget.h>

#include <QFile>

#define TOKEN_ITEM_ICONSIZE 24
TokenItemWidget::TokenItemWidget(const PlatformStyle *platformStyle, QWidget *parent, ItemType type) :
    QWidget(parent),
    ui(new Ui::TokenItemWidget),
    m_platfromStyle(platformStyle),
    m_type(type),
    m_position(-1)

{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(type);
    ui->buttonSend->setIcon(m_platfromStyle->MultiStatesIcon(":/icons/send", PlatformStyle::PushButton));
    ui->buttonReceive->setIcon(platformStyle->MultiStatesIcon(":/icons/receiving_addresses", PlatformStyle::PushButton));
    ui->buttonAdd->setIcon(platformStyle->MultiStatesIcon(":/icons/plus_full", PlatformStyle::PushButtonIcon));
    ui->tokenLogo->setPixmap(platformStyle->MultiStatesIcon(m_type == New ? ":/icons/add_token" : ":/icons/token").pixmap(TOKEN_ITEM_ICONSIZE, TOKEN_ITEM_ICONSIZE));
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
    if(m_filename != filename)
    {
        m_filename = filename;
        updateLogo();
    }
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

void TokenItemWidget::updateLogo()
{
    QPixmap pixmap;
    if(QFile::exists(m_filename))
    {
        QIcon icon(m_filename);
        pixmap = icon.pixmap(ui->tokenLogo->width(), ui->tokenLogo->height());
    }
    else
    {
        pixmap = m_platfromStyle->MultiStatesIcon(":/icons/token").pixmap(TOKEN_ITEM_ICONSIZE, TOKEN_ITEM_ICONSIZE);
    }
    ui->tokenLogo->setPixmap(pixmap);
}
