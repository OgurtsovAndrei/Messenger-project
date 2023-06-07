#include "interface/chatInfo.h"
#include "interface/welcWindow.h"
#include "ui_chatInfo.h"
#include "database/Dialog.hpp"
#include "interface/popUp.h"

ChatInfo::ChatInfo(int dialog_id, MainWindow *mainWin, QWidget *parent) :
    dialog_id(dialog_id),
    mainWin(mainWin),
    QDialog(parent),
    ui(new Ui::ChatInfo)
{
    ui->setupUi(this);
    ui->encrLabel->close();
    ui->encrOptions->close();
    this->setWindowTitle("Chat Info");
    auto [status, dialog] = client.get_dialog_by_id(dialog_id);
    if (!status) {
        auto *popUp = new PopUp("This dialog is corrupted.\nFor help, please contact at.\nWe will try to help you");
        popUp->show();
        return ;
    }
    ui->userNameLabel->setWordWrap(true);
    if (dialog.m_is_group) {
        ui->userNameLabel->setText(QString::fromStdString(dialog.m_name));
        auto [st, users] = client.get_users_in_dialog(dialog_id);
        for (const auto &user : users) {
            auto user_name = new QListWidgetItem(QString::fromStdString(user.m_name + " " + user.m_surname), nullptr);
            ui->memList->addItem(user_name);
        }
    }
    else {
        ui->userNameLabel->setText(mainWin->get_sec_user_name_surname(dialog_id));
        close_group_buttons();
        setFixedHeight(75);
    }
}


ChatInfo::ChatInfo(MainWindow *mainWin, QWidget *parent) :
    mainWin(mainWin),
    QDialog(parent), ui(new Ui::ChatInfo)
{
    ui->setupUi(this);
    QString cl_name_surname = mainWin->get_client_name_surname();
    this->setWindowTitle(cl_name_surname);
    ui->userNameLabel->setText(cl_name_surname);
    ui->userNameLabel->setWordWrap(true);
    close_group_buttons();
    setFixedHeight(125);
}


ChatInfo::~ChatInfo()
{
    delete ui;
}

void ChatInfo::on_addMemButton_clicked()
{
    QString add_mem = ui->addMemLine->text();
    if (add_mem.isEmpty()) {
        return;
    }
    auto [status, sec_client] = client.get_user_id_by_login(add_mem.toStdString());
    if (status) {
        auto add_st = client.add_user_to_dialog(sec_client.m_user_id, dialog_id);
//        if (add_st) {
//            ui->memList->addItem(add_mem);
//        }
//        else {
//        }
////        TODO message
    }
}

void ChatInfo::close_group_buttons() {
    ui->addMemButton->close();
    ui->memList->close();
    ui->addMemLine->close();
    ui->memLabel->close();
}



void ChatInfo::on_encrOptions_activated(int index)
{
    ///TODO set_encryption_from(
    ///
}

