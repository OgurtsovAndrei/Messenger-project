#include "interface/chatInfo.h"
#include "interface/welcWindow.h"
#include "ui_chatInfo.h"
#include "database/Dialog.hpp"
#include "interface/popUp.h"
#include "algorithm"

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
        ui->userNameLabel->setText(mainWin->get_second_user_name_surname(dialog_id));
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
    auto [status, all_encr] = client.get_all_encryption();
    if (!status) {
        show_popUp("There were problems with new type of encryption.\n");
        return ;
    }
    std::sort(all_encr.begin(), all_encr.end());
    for (auto [i, encr_name] : all_encr) {
        ui->encrOptions->addItem(QString::fromStdString(encr_name));
    }
    int encr_index = mainWin->get_cl_encryption_id() - 1;
    if (ui->encrOptions->count() <= encr_index) {
        show_popUp("Extra number of encryption.\n");
        return ;
    }
    ui->encrOptions->setCurrentIndex(encr_index);
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
    if (!status) {
        show_popUp("User with login '" + sec_client.m_login + "' doesn't exist");
        return ;
    }
    auto add_st = client.add_user_to_dialog(sec_client.m_user_id, dialog_id);
    if (add_st) {
        ui->memList->addItem(QString::fromStdString(sec_client.m_name + " " + sec_client.m_surname));
    }
    else {
        show_popUp("There were problems with add new member.\n");
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
    auto [status, new_cl_info] = client.change_user(mainWin->get_client_id(), "encryption", "", index + 1);
    if (!status) {
        show_popUp("There were problems with changing encryption.\n");
        return ;
    }
    mainWin->set_client_info(new_cl_info);
}

