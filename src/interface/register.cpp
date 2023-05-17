#include "interface/register.h"
#include "./ui_register.h"
#include "interface/mainwindow.h"
#include "Net/NetClient.hpp"
#include "interface/welcWindow.h"

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
  setFixedSize(430, 200);
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
  }
  if ((regVersion && status) || (!regVersion && !status)) {
    status = client.log_in(login,ui->pasInput->text().toStdString());
  }
  std::cout << bool(status) << " " << status.message() << "\n";
  if (status) {
    std::cout << "Logged in -->>" + status.message() + "\n";
//    client.set_user_id(std::stoi(status.message()));
    auto *win = new MainWindow();
    auto [cl_status, cl_info] = client.get_user_id_by_login(login);
    if (cl_status) {
        win->set_client_info(cl_info);
        win->show();
    }
    this->close();
  } else {
    std::cout << "Log in failed -->>" + status.message() + "\n";
    std::cout << "log in isn't correct. Please try again or register" << "\n";
  }
}
