#include "addsuperstakerpage.h"
#include "qt/forms/ui_addsuperstakerpage.h"

#include <miner.h>
#include <wallet/wallet.h>
#include <qt/walletmodel.h>
#include <QMessageBox>

AddSuperStakerPage::AddSuperStakerPage(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddSuperStakerPage),
    m_model(nullptr)
{
    ui->setupUi(this);

    setWindowTitle(tr("Add super staker"));

    // Set defaults
    ui->spinBoxFee->setMinimum(0);
    ui->spinBoxFee->setMaximum(100);
    ui->spinBoxFee->setValue(DEFAULT_STAKING_MIN_FEE);

    connect(ui->lineEditStaker, &QComboBox::currentTextChanged, this, &AddSuperStakerPage::on_updateAddStakerButton);
}

AddSuperStakerPage::~AddSuperStakerPage()
{
    delete ui;
}

void AddSuperStakerPage::setModel(WalletModel *_model)
{
    m_model = _model;
    if(m_model)
    {
        ui->lineEditStaker->setWalletModel(m_model);
    }
}

void AddSuperStakerPage::clearAll()
{
    ui->spinBoxFee->setValue(DEFAULT_STAKING_MIN_FEE);
    ui->lineEditStaker->setCurrentIndex(-1);
}

void AddSuperStakerPage::accept()
{
    clearAll();
    QDialog::accept();
}

void AddSuperStakerPage::reject()
{
    clearAll();
    QDialog::reject();
}

void AddSuperStakerPage::show()
{
    ui->lineEditStaker->setFocus();
    QDialog::show();
}

void AddSuperStakerPage::on_cancelButton_clicked()
{
    reject();
}

void AddSuperStakerPage::on_updateAddStakerButton()
{
    bool enabled = true;
    QString staker = ui->lineEditStaker->currentText();
    if(staker.isEmpty() || !ui->lineEditStaker->isValidAddress())
    {
        enabled = false;
    }

    ui->addSuperStakerButton->setEnabled(enabled);
}

void AddSuperStakerPage::on_addSuperStakerButton_clicked()
{
    if(m_model)
    {
        bool fSuperStake = gArgs.GetBoolArg("-superstaking", DEFAULT_SUPER_STAKE);
        if(!fSuperStake)
        {
            QMessageBox::information(this, tr("Super staking"), tr("Enable super staking from the option menu in order to start the super staker."));
        }

        QString stakerAddress = ui->lineEditStaker->currentText();
        int stakerFee = ui->spinBoxFee->value();
        interfaces::SuperStakerInfo superStaker;
        superStaker.staker_address = stakerAddress.toStdString();
        superStaker.min_fee = stakerFee;
        m_model->wallet().addSuperStakerEntry(superStaker);
        accept();
    }
}
