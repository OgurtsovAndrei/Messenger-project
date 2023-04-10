#include "welcWindow.h"
#include "mainwindow.h"
#include "register.h"
#include "./ui_register.h"
#include "netClient.hpp"

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
    regVersion = false;
}

void Register::on_cancelButton_clicked()
{
    auto *welc = new WelcWindow();
    welc->show();
    this->close();
}


void Register::on_readyButton_clicked()
{
    //TODO check if password okay
    Net::Client::Client client("localhost", "12345");
    client.make_secure_connection();
    Status status;
    if (regVersion) {
        status = client.sing_up(ui->nameInput->text().toStdString(), ui->snameInput->text().toStdString(), ui->logInput->text().toStdString(), ui->pasInput->text().toStdString());
    } else {
        status = client.log_in(ui->logInput->text().toStdString(), ui->pasInput->text().toStdString());
    }
    if (status) {
        std::cout << "Logged in -->>" + status.message() + "\n";
        auto *win = new MainWindow();
        win->show();
        this->close();
    } else {
        std::cout << "Log in failed -->>" + status.message() + "\n";
        std::cout << "log in isn't correct. Please try again or register";
    }
}

