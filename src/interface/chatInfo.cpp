#include "interface/chatInfo.h"
#include "interface/welcWindow.h"
#include "ui_chatInfo.h"
#include "database/Dialog.hpp"

ChatInfo::ChatInfo(int dialog_id, QWidget *parent) : dialog_id(dialog_id),
    QDialog(parent),
    ui(new Ui::ChatInfo)
{
    ui->setupUi(this);
    this->setWindowTitle("Chat Info");
    auto [status, chats] = client.get_last_n_dialogs(100);
    database_interface::Dialog *dialog;
    for  (auto &chat : chats) {
        if (chat.m_dialog_id == dialog_id) {
            dialog = &chat;
        }
    }
//    auto [status, dialog] = client.get_dialog_by_id(dialog_id);
//    if (!status) {
//        return ; /// TODO fail message
//    }
    ui->userNameLabel->setText(QString::fromStdString(dialog->m_name));
    ui->userNameLabel->setWordWrap(true);
    if (!dialog->m_is_group) {
        ui->addMemButton->hide();
        ui->memList->hide();
        ui->addMemLine->hide();
        ui->memLabel->hide();
        setFixedSize(400, 75);

    }
    else {
//        get_users_in_dialog
//        ui->memList->addItem(client_name)
    }
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
        if (add_st) {
            ui->memList->addItem(add_mem);
        }
        else {}
//        TODO message
    }
}

