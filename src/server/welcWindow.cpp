#include "../../include/interface/welcWindow.h"
#include <QDesktopServices>
#include "../../include/interface/register.h"
#include "./ui_welcWindow.h"

WelcWindow::WelcWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WelcWindow)
{
    ui->setupUi(this);

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
    Register *reg = new Register();
    if (ui->logInButton->isDown()) {
        reg->delRegInfo();
    }
    reg->show();
    this->close();
}

