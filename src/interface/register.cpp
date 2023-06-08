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
  QString login = ui->logInput->text();
  QString pas = ui->pasInput->text();
  if (incorrect_log_or_pas(login, pas)) {
    return;
  }
  if (regVersion && !sign_up()) {
      return ;
  }
  auto [log_status, cl_user] = client.log_in(login.toStdString(), pas.toStdString());
  if (log_status) {
    std::cout << "Logged in -->>" + log_status.message() + "\n";
    auto *win = new MainWindow();
    win->set_client_info(cl_user);
    win->show();
    this->close();
  } else {
      show_popUp("Incorrect login or password. Please try again.\n");
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
    if (pas.contains(" ")) {
        popUp_msg += "Password can't contain spaces.\n";
    }
    if (log.contains('\\') || log.contains('/')) {
        popUp_msg += "'\\' and '/' characters are not allowed in the login.\n";
    }
    if (log.contains(" ")) {
        popUp_msg += "Login can't contain spaces.\n";
    }
    if (pas.isEmpty() || log.isEmpty()) {
        popUp_msg = "password and login are not allowed to be empty.\n";
    }
    if (!popUp_msg.empty()) {
        popUp_msg += "Please try again.\n";
        show_popUp(popUp_msg);
        return true;
    }
    return false;
}

bool Register::incorrect_name_or_surname(const QString &name, const QString &sname) {
    std::string popUp_msg;
    if (name.isEmpty() || sname.isEmpty()) {
        popUp_msg += "Name and surname are not allowed to be empty.\n";
    }
    if (name.contains(" ") || sname.contains(" ")) {
        popUp_msg += "Name and Surname can't contain spaces.\n";
    }
    if (!popUp_msg.empty()) {
        popUp_msg += "Please try again.\n";
        show_popUp(popUp_msg);
        return true;
    }
    return false;
}

bool Register::sign_up() {
    QString name = ui->nameInput->text();
    QString sname = ui->snameInput->text();
    QString login = ui->logInput->text();
    QString pas = ui->pasInput->text();
    if (incorrect_name_or_surname(name, sname)) {
        return false;
    }
    auto sign_status = client.sign_up(name.toStdString(),
                                 sname.toStdString(),
                                 login.toStdString(),
                                 pas.toStdString());
    if (!sign_status) {
        std::string err = "There were problems with sign up.\n";
        std::cout << sign_status.message() << "\n";
        if (sign_status.message() == "Problem in MAKE User.\nMessage: login is already taken\n") {
            err = "This login is already in use.\nPlease try again.\n";
        }
        show_popUp(err);
        return false;
    }
    return true;
}
