#include "addsuperstakerpage.h"
#include "qt/forms/ui_addsuperstakerpage.h"

#include <wallet/wallet.h>
#include <qt/clientmodel.h>

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
    ui->lineEditStaker->setWalletModel(m_model);
}

void AddSuperStakerPage::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;

    if (m_clientModel)
    {
        connect(m_clientModel, SIGNAL(gasInfoChanged(quint64, quint64, quint64)), this, SLOT(on_gasInfoChanged(quint64, quint64, quint64)));
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
