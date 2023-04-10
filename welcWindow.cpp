#include "welcWindow.h"
#include "./ui_welcWindow.h"
#include "register.h"
#include <QDesktopServices>

WelcWindow::WelcWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WelcWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("ИШО");

    connect(ui->logInButton, &QPushButton::clicked, [&](){
        ui->logInButton->setDown(true);
        on_regButton_clicked();
    });
}

WelcWindow::~WelcWindow()
{
    delete ui;
}




void WelcWindow::on_regButton_clicked()
{
    auto *reg = new Register();
    if (ui->logInButton->isDown()) {
        reg->delRegInfo();
    }
    reg->show();
    this->close();
}

