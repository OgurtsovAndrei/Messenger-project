#include "../../include/interface/register.h"
#include "../../include/interface/welcWindow.h"
#include "./ui_register.h"

Register::Register(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Register)
{
    ui->setupUi(this);
}

Register::~Register()
{
    delete ui;
}

void Register::delRegInfo() {
    ui->nameInput->close();
    ui->nameLabel->close();
    ui->snameInput->close();
    ui->snameLabel->close();
}

void Register::on_cancelButton_clicked()
{
    WelcWindow *welc = new WelcWindow();
    welc->show();
    this->close();
}


void Register::on_readyButton_clicked()
{
    //TODO check if password okay
    std::string a = ui->logInput->text().toStdString();
}

