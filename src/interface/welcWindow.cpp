#include "interface/welcWindow.h"
#include <QDesktopServices>
#include "./ui_welcWindow.h"
#include "Net/NetClient.hpp"
#include "interface/register.h"

WelcWindow::WelcWindow(Net::Client::Client *client_, QWidget *parent)
    : client(client_), QWidget(parent), ui(new Ui::WelcWindow) {
    ui->setupUi(this);

    this->setWindowTitle("ИШО");

    connect(ui->logInButton, &QPushButton::clicked, this, [&]() {
        ui->logInButton->setDown(true);
        on_regButton_clicked();
    });
}

WelcWindow::~WelcWindow() { delete ui; }

void WelcWindow::on_regButton_clicked() {
    auto *reg = new Register(client);
    if (ui->logInButton->isDown()) {
        reg->delRegInfo();
    }
    reg->show();
    this->close();
}
