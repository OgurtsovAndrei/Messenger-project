#include "../../include/interface/register.h"
#include "./ui_register.h"
#include "../../include/interface/mainwindow.h"
#include "../../src/general/NetTest/netClient.hpp"
#include "../../include/interface/welcWindow.h"
#include "../../include/database/DataBaseInterface.hpp"

Net::Client::Client client("localhost", "12345");

Register::Register(QWidget *parent) : QWidget(parent), ui(new Ui::Register) {
  ui->setupUi(this);
  this->setWindowTitle("Log in");
}

Register::~Register() { delete ui; }

void Register::delRegInfo() {
  ui->nameInput->close();
  ui->nameLabel->close();
  ui->snameInput->close();
  ui->snameLabel->close();
  regVersion = false;
}

void Register::on_cancelButton_clicked() {
  auto *welc = new WelcWindow();
  welc->show();
  this->close();
}

void Register::on_readyButton_clicked() {
  // TODO check if password okay
//  Net::Client::Client client("localhost", "12345");
//  client.make_secure_connection();
  Status status;
  std::string login = ui->logInput->text().toStdString();
  if (regVersion) {
    status = client.sing_up(ui->nameInput->text().toStdString(),
                            ui->snameInput->text().toStdString(),
                            login,
                            ui->pasInput->text().toStdString());
  } else {
    status = client.log_in(login,ui->pasInput->text().toStdString());
  }
  if (status) {
    std::cout << "Logged in -->>" + status.message() + "\n";
//    client.set_user_id(std::stoi(status.message()));
    auto *win = new MainWindow();
    win->set_client_id(client.get_user_id_by_login(login).second.m_user_id);
    win->show();
    this->close();
  } else {
    std::cout << "Log in failed -->>" + status.message() + "\n";
    std::cout << "log in isn't correct. Please try again or register" << "\n";
  }
}
