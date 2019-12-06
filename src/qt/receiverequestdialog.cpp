// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/receiverequestdialog.h>
#include <qt/forms/ui_receiverequestdialog.h>

#include <qt/bitcoinunits.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/styleSheet.h>

#include <QClipboard>
#include <QPixmap>

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h> /* for USE_QRCODE */
#endif

ReceiveRequestDialog::ReceiveRequestDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReceiveRequestDialog),
    model(nullptr)
{
    ui->setupUi(this);

    SetObjectStyleSheet(ui->btnCopyURI, StyleSheetNames::ButtonWhite);
    SetObjectStyleSheet(ui->btnSaveAs, StyleSheetNames::ButtonWhite);
    SetObjectStyleSheet(ui->btnCopyAddress, StyleSheetNames::ButtonWhite);

#ifndef USE_QRCODE
    ui->btnSaveAs->setVisible(false);
    ui->lblQRCode->setVisible(false);
#endif

    connect(ui->btnSaveAs, &QPushButton::clicked, ui->lblQRCode, &QRImageWidget::saveImage);
}

ReceiveRequestDialog::~ReceiveRequestDialog()
{
    delete ui;
}

void ReceiveRequestDialog::setModel(WalletModel *_model)
{
    this->model = _model;

    if (_model)
        connect(_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &ReceiveRequestDialog::update);

    // update the display unit if necessary
    update();
}

void ReceiveRequestDialog::setInfo(const SendCoinsRecipient &_info)
{
    this->info = _info;
    update();
}

void ReceiveRequestDialog::update()
{
    if(!model)
        return;
    QString target = info.label;
    if(target.isEmpty())
        target = info.address;
    setWindowTitle(tr("Request payment to %1").arg(target));

    QString uri = GUIUtil::formatBitcoinURI(info);
    ui->btnSaveAs->setEnabled(false);
    QString html;
    html += "<html><font face='verdana, arial, helvetica, sans-serif'>";
    html += "<font color='#ffffff'>" + tr("PAYMENT INFORMATION")+"</font><br><br>";
    html += tr("URI")+": ";
    html += "<a href=\""+uri+"\">" + GUIUtil::HtmlEscape(uri) + "</a><br>";
    html += tr("Address")+": <font color='#ffffff'>" + GUIUtil::HtmlEscape(info.address) + "</font><br>";
    if(info.amount)
        html += tr("Amount")+": <font color='#ffffff'>" + BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), info.amount) + "</font><br>";
    if(!info.label.isEmpty())
        html += tr("Label")+": <font color='#ffffff'>" + GUIUtil::HtmlEscape(info.label) + "</font><br>";
    if(!info.message.isEmpty())
        html += tr("Message")+": <font color='#ffffff'>" + GUIUtil::HtmlEscape(info.message) + "</font><br>";
    if(model->isMultiwallet()) {
        html += tr("Wallet")+": <font color='#ffffff'>" + GUIUtil::HtmlEscape(model->getWalletName()) + "</font><br>";
    }
    ui->outUri->setText(html);

    if (ui->lblQRCode->setQR(uri)) {
        ui->lblQRCode->setScaledContents(true);
        ui->btnSaveAs->setEnabled(true);
    }
}

void ReceiveRequestDialog::on_btnCopyURI_clicked()
{
    GUIUtil::setClipboard(GUIUtil::formatBitcoinURI(info));
}

void ReceiveRequestDialog::on_btnCopyAddress_clicked()
{
    GUIUtil::setClipboard(info.address);
}
