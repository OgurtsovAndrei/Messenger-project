#include "interface/chatInfo.h"
#include <QListWidgetItem>
#include <QTimer>
#include "database/Dialog.hpp"
#include "interface/chatSetting.h"
#include "interface/popUp.h"
#include "interface/register.h"
#include "interface/welcWindow.h"
#include "ui_chatInfo.h"

ChatInfo::ChatInfo(int dialog_id_, MainWindow *mainWin, QWidget *parent)
    : dialog_id(dialog_id_),
      mainWin(mainWin),
      QDialog(parent),
      ui(new Ui::ChatInfo) {
    ui->setupUi(this);
    ui->encrLabel->close();
    ui->encrOptions->close();
    ui->changeLogButton->close();
    ui->changeNameButton->close();
    ui->changeSnameButton->close();
    this->setWindowTitle("Chat Info");
    auto [status, dialog] = mainWin->get_client()->get_dialog_by_id(dialog_id);
    if (!status) {
        auto *popUp = new PopUp(
            "This dialog is corrupted.\nFor help, please contact at.\nWe will "
            "try to help you"
        );
        popUp->show();
        return;
    }
    owner_id = dialog.m_owner_id;
    ui->userNameLabel->setWordWrap(true);
    if (dialog.m_is_group) {
        ui->userNameLabel->setText(QString::fromStdString(dialog.m_name));
        update_dialog();
    } else {
        ui->userNameLabel->setText(
            mainWin->get_second_user_name_surname(dialog_id)
        );
        close_group_buttons();
        setMinimumSize(400, 46);
        resize(QDialog::minimumSizeHint());
    }
}

ChatInfo::ChatInfo(MainWindow *mainWin, QWidget *parent)
    : mainWin(mainWin),
      sure_change(new SureDo()),
      QDialog(parent),
      ui(new Ui::ChatInfo) {
    ui->setupUi(this);
    setMinimumWidth(400);
    QString cl_name_surname = mainWin->get_client_name_surname();
    setWindowTitle(cl_name_surname);
    auto [status, all_encr] = mainWin->get_client()->get_all_encryption();
    if (!status) {
        show_popUp("There were problems with new type of encryption.\n");
        return;
    }
    std::sort(all_encr.begin(), all_encr.end());
    for (const auto &[i, encr_name] : all_encr) {
        ui->encrOptions->addItem(QString::fromStdString(encr_name));
    }
    int encr_index = mainWin->get_cl_encryption_id() - 1;
    if (ui->encrOptions->count() <= encr_index) {
        show_popUp("Extra number of encryption.\n");
        return;
    }
    ui->encrOptions->setCurrentIndex(encr_index);
    ui->userNameLabel->setText(cl_name_surname);
    ui->userNameLabel->setWordWrap(true);
    close_group_buttons();
    resize(QDialog::minimumSizeHint());

    connect(sure_change, &QDialog::accepted, this, [=]() {
        QString new_parametr = sure_change->get_line();
        if (incorrect_name_or_surname(new_parametr)) {
            return;
        }
        std::pair<Status, database_interface::User> change_res;
        switch (last_mod) {
            case LAST_MODIFIED::NAME:
                change_res = mainWin->get_client()->change_user(
                    mainWin->get_client_id(), "name",
                    new_parametr.toStdString(), -1
                );
                break;
            case LAST_MODIFIED::SURNAME:
                change_res = mainWin->get_client()->change_user(
                    mainWin->get_client_id(), "surname",
                    new_parametr.toStdString(), -1
                );
                break;
            case LAST_MODIFIED::LOGIN:
                if (new_parametr.contains("\\") || new_parametr.contains("/") ||
                    new_parametr.contains("\'")) {
                    show_popUp(
                        "'\\', '/' and '\'' characters are not allowed in the "
                        "password.\n"
                    );
                    return;
                }
                change_res = mainWin->get_client()->change_user(
                    mainWin->get_client_id(), "login",
                    new_parametr.toStdString(), -1
                );
                break;
            default:
                return;
        }
        last_mod = LAST_MODIFIED::NONE;
        if (!change_res.first) {
            show_popUp("There were problems with changing user info.\n");
            return;
        }
        mainWin->set_client_info(change_res.second);
        QString cl_name_surname = mainWin->get_client_name_surname();
        setWindowTitle(cl_name_surname);
        ui->userNameLabel->setText(cl_name_surname);
    });
}

ChatInfo::~ChatInfo() { delete ui; }

void ChatInfo::on_addMemButton_clicked() {
    QString add_mem = ui->addMemLine->text();
    if (add_mem.isEmpty()) {
        return;
    }
    auto [status, sec_client] =
        mainWin->get_client()->get_user_id_by_login(add_mem.toStdString());
    if (!status) {
        show_popUp(
            "User with login '" + sec_client.m_login + "' doesn't exist"
        );
        return;
    }
    auto add_st = mainWin->get_client()->add_user_to_dialog(
        sec_client.m_user_id, dialog_id
    );
    if (add_st) {
        ui->memList->addItem(QString::fromStdString(
            sec_client.m_name + " " + sec_client.m_surname
        ));
        ui->addMemLine->clear();
    } else {
        show_popUp("There were problems with add new member.\n");
    }
}

void ChatInfo::close_group_buttons() {
    ui->addMemButton->close();
    ui->memList->close();
    ui->addMemLine->close();
    ui->memLabel->close();
}

void ChatInfo::on_encrOptions_activated(int index) {
    auto [status, new_cl_info] = mainWin->get_client()->change_user(
        mainWin->get_client_id(), "encryption", "", index + 1
    );
    if (!status) {
        show_popUp("There were problems with changing encryption.\n");
        return;
    }
    ui->encrOptions->setCurrentIndex(mainWin->get_cl_encryption_id() - 1);
    show_success_popUp(
        "Your encryption was changed successful!\n For finish, please reopen "
        "'ИШО'.\n"
    );
}

void ChatInfo::on_memList_itemDoubleClicked(QListWidgetItem *item) {
    std::string dialog_name = "d_id:" + std::to_string(dialog_id) + " f_id:" +
                              std::to_string(mainWin->get_client_id()) +
                              " s_id:" + std::to_string(item->type());
    if (mainWin->get_client_id() == owner_id) {
        auto chat_set = new ChatSetting(
            mainWin->get_client(), item->type(), item->text(), dialog_name, this
        );
        chat_set->show();
    } else {
        auto chat_set = new ChatSetting(
            mainWin->get_client(), mainWin->get_client_id(), item->type(),
            dialog_name
        );
        chat_set->show();
    }
}

void ChatInfo::on_changeNameButton_clicked() {
    sure_change->set_text("Enter new name, please.");
    last_mod = LAST_MODIFIED::NAME;
    sure_change->clear_line();
    sure_change->show();
}

void ChatInfo::on_changeSnameButton_clicked() {
    sure_change->set_text("Enter new surname, please.");
    last_mod = LAST_MODIFIED::SURNAME;
    sure_change->clear_line();
    sure_change->show();
}

void ChatInfo::on_changeLogButton_clicked() {
    sure_change->set_text("Enter new login, please.");
    last_mod = LAST_MODIFIED::LOGIN;
    sure_change->clear_line();
    sure_change->show();
}

int ChatInfo::get_owner_id() const { return owner_id; }

int ChatInfo::get_dialog_id() const { return dialog_id; }

void ChatInfo::update_dialog() {
    auto [st, users] = mainWin->get_client()->get_users_in_dialog(dialog_id);
    if (!st) {
        show_popUp("Problems with showing group members.\n");
        return;
    }
    ui->memList->clear();
    for (const auto &user : users) {
        auto user_name =
            QString::fromStdString(user.m_name + " " + user.m_surname);
        auto mem_item = new QListWidgetItem(user_name, nullptr, user.m_user_id);
        ui->memList->addItem(mem_item);
    }
}
