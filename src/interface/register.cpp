#include "interface/register.h"
#include "./ui_register.h"
#include "interface/mainwindow.h"
#include "Net/NetClient.hpp"
#include "interface/welcWindow.h"
#include "interface/popUp.h"

Net::Client::Client client("localhost", "12345");

Register::Register(QWidget *parent) : QWidget(parent), ui(new Ui::Register) {
  ui->setupUi(this);
  this->setWindowTitle("Log in");
  client.make_secure_connection();
}

Register::~Register() { delete ui; }

void Register::delRegInfo() {
  ui->nameInput->close();
  ui->nameLabel->close();
  ui->snameInput->close();
  ui->snameLabel->close();
  setFixedHeight(200);
  regVersion = false;
}

void Register::on_cancelButton_clicked() {
    auto *welc = new WelcWindow();
    welc->show();
    client.close_connection();
    this->close();
}

void Register::on_readyButton_clicked() {
  Status status;
  QString login = ui->logInput->text();
  QString pas = ui->pasInput->text();
  if (incorrect_log_or_pas(login, pas)) {
    return;
  }
  if (regVersion) {
    QString name = ui->nameInput->text();
    QString sname = ui->snameInput->text();
    if (incorrect_name_or_surname(name, sname)) {
      return;
    }
    status = client.sing_up(name.toStdString(),
                            sname.toStdString(),
                            login.toStdString(),
                            pas.toStdString());
  }
  if ((regVersion && status) || (!regVersion)) {
    status = client.log_in(login.toStdString(), pas.toStdString());
  }
  if (status) {
    std::cout << "Logged in -->>" + status.message() + "\n";
    auto *win = new MainWindow();
    auto [cl_status, cl_info] = client.get_user_id_by_login(login.toStdString());
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

bool Register::incorrect_log_or_pas(const QString &log, const QString &pas) {
    std::string popUp_msg;
//    if (0 < pas.size() && pas.size() < 8) {
//        popUp_msg += "Password must contain at least 8 characters.\n";
//    }
    if (pas.contains('\\') || pas.contains('/')) {
        popUp_msg += "'\\' and '/' characters are not allowed in the password.\n";
    }
    if (log.contains('\\') || log.contains('/')) {
        popUp_msg += "'\\' and '/' characters are not allowed in the login.\n";
    }
    if (pas.isEmpty() || log.isEmpty()) {
        popUp_msg = "password and login are not allowed to be empty.\n";
    }
    if (!popUp_msg.empty()) {
        popUp_msg += "Please try again";
        auto *popUp = new PopUp(popUp_msg);
        popUp->adjustSize();
        popUp->show();
        return true;
    }
    return false;
}

bool Register::incorrect_name_or_surname(const QString &name, const QString &sname) {
    std::string popUp_msg;
    if (name.isEmpty() || sname.isEmpty()) {
        popUp_msg = "name and surname are not allowed to be empty.\n";
        popUp_msg += "Please try again";
        auto *popUp = new PopUp(popUp_msg);
        popUp->adjustSize();
        popUp->show();
        return true;
    }
    return false;
}
